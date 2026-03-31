# GCS All-in-One CDC Test GUI
# Tests all TX commands and visualises every RX message for the RP2040 GCS firmware.
#
# Dependencies:  pip install customtkinter pyserial
#
# Usage:  python gcs_tester.py

import customtkinter as ctk
import tkinter as tk
import serial
import serial.tools.list_ports
import threading
import struct
import time
import queue


# ---------------------------------------------------------------------------
# Protocol layer
# ---------------------------------------------------------------------------

class GCSProtocol:
    SOF = 0xAA

    # Message types
    TYPE_ADC        = 0x01
    TYPE_DIGITAL    = 0x02
    TYPE_LED        = 0x03
    TYPE_SCREEN     = 0x04
    TYPE_HEARTBEAT  = 0x05
    TYPE_EVENT      = 0x06
    TYPE_ERROR      = 0x07
    TYPE_BRIGHTNESS = 0x08
    TYPE_MODE       = 0x09
    TYPE_WARNING    = 0x0A
    TYPE_ALS        = 0x0B

    # LED chain IDs (first byte of LED payload)
    CHAIN_SK6812    = 0x00
    CHAIN_WS2811    = 0x01
    CHAIN_INDICATOR = 0x02

    # WS2811 animation modes
    ANIM_OFF        = 0
    ANIM_ON         = 1
    ANIM_BLINK_SLOW = 2
    ANIM_BLINK_FAST = 3
    ANIM_PULSE      = 4
    ANIM_NAMES      = ["OFF", "ON", "BLINK_SLOW", "BLINK_FAST", "PULSE"]

    # System state names  (index == state value)
    STATE_NAMES = ["BOOT", "INIT", "WAITING_FOR_PI", "CONNECTED", "LOCKED", "ACTIVE", "ERROR"]

    # Screen mode names  (index == mode value)
    SCREEN_NAMES = ["AUTO", "MAIN", "WARNING", "LOCK", "BATWARNING"]

    # Warning icon names  (index == payload byte position)
    WARN_NAMES = [
        "TEMP", "SIGNAL", "AIRCRAFT", "DRONE_LINK", "MAIN",
        "GPS_GCS", "NETWORK_GCS", "LOCKED", "DRONE_STATUS",
    ]
    WARN_SEVERITY_NAMES = ["OK", "WARNING", "CRITICAL"]

    # Firmware max payload = 256 bytes.
    # SK6812 chain: payload = [chain(1), num_px(1), GRBW*n] → max n = (256-2)/4 = 63
    SK6812_MAX_PIXELS_PER_SEND = 63

    @staticmethod
    def _checksum(msg_type: int, length: int, payload: bytes) -> int:
        c = msg_type ^ length
        for b in payload:
            c ^= b
        return c & 0xFF

    @staticmethod
    def build_packet(msg_type: int, payload: bytes) -> bytes:
        length = len(payload)
        cksum = GCSProtocol._checksum(msg_type, length, payload)
        return bytes([GCSProtocol.SOF, msg_type, length]) + payload + bytes([cksum])

    @staticmethod
    def parse_stream(buffer: bytes) -> tuple:
        """
        Scan buffer for complete, valid packets.
        Returns (list_of_(type, payload_bytes), remaining_unprocessed_bytes).
        Skips garbage bytes before each SOF.  Leaves partial packets buffered.
        """
        packets = []
        i = 0
        while i < len(buffer):
            if buffer[i] != GCSProtocol.SOF:
                i += 1
                continue
            # Need SOF + TYPE + LEN + CKSUM = 4 bytes minimum
            if i + 3 >= len(buffer):
                break
            msg_type = buffer[i + 1]
            length   = buffer[i + 2]
            # Need full payload + checksum byte
            if i + 3 + length >= len(buffer):
                break
            payload   = buffer[i + 3 : i + 3 + length]
            cksum_rx  = buffer[i + 3 + length]
            cksum_exp = GCSProtocol._checksum(msg_type, length, payload)
            if cksum_rx == cksum_exp:
                packets.append((msg_type, bytes(payload)))
                i += 4 + length
            else:
                i += 1  # bad checksum, skip this SOF byte and keep scanning
        return packets, buffer[i:]


# ---------------------------------------------------------------------------
# Serial driver
# ---------------------------------------------------------------------------

class SerialDriver:
    def __init__(self, rx_callback):
        """
        rx_callback(msg_type, payload) is called from the RX thread.
        Sentinel: rx_callback(None, None) on serial error / unexpected disconnect.
        """
        self._ser: serial.Serial | None = None
        self._thread: threading.Thread | None = None
        self._running = False
        self._rx_callback = rx_callback
        self._rx_buf = b""
        self._lock = threading.Lock()  # guards _ser.write access

    def connect(self, port: str, baud: int = 115200) -> bool:
        try:
            self._ser = serial.Serial(port, baud, timeout=0.05)
            self._running = True
            self._rx_buf = b""
            self._thread = threading.Thread(target=self._rx_loop, daemon=True)
            self._thread.start()
            return True
        except serial.SerialException as e:
            self._ser = None
            return False

    def disconnect(self) -> None:
        self._running = False
        if self._thread:
            self._thread.join(timeout=1.0)
            self._thread = None
        if self._ser and self._ser.is_open:
            try:
                self._ser.close()
            except Exception:
                pass
        self._ser = None

    def send(self, data: bytes) -> bool:
        if not self.is_connected():
            return False
        try:
            with self._lock:
                self._ser.write(data)
            return True
        except (serial.SerialException, OSError):
            return False

    def is_connected(self) -> bool:
        return self._ser is not None and self._ser.is_open

    def _rx_loop(self) -> None:
        while self._running:
            try:
                chunk = self._ser.read(256)
                if chunk:
                    self._rx_buf += chunk
                    # Cap buffer to prevent unbounded growth on garbage data
                    if len(self._rx_buf) > 1024:
                        self._rx_buf = self._rx_buf[-512:]
                    packets, self._rx_buf = GCSProtocol.parse_stream(self._rx_buf)
                    for msg_type, payload in packets:
                        self._rx_callback(msg_type, payload)
            except (serial.SerialException, OSError):
                self._running = False
                self._rx_callback(None, None)
                return

    @staticmethod
    def list_ports() -> list:
        return sorted(p.device for p in serial.tools.list_ports.comports())


# ---------------------------------------------------------------------------
# Main application
# ---------------------------------------------------------------------------

class GCSTesterApp(ctk.CTk):

    def __init__(self):
        super().__init__()
        self.title("GCS All-in-One Tester")
        self.geometry("1640x980")
        self.minsize(1400, 860)
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self._driver = SerialDriver(rx_callback=self._on_rx_packet)
        self._hb_seq = 0
        self._hb_auto = False
        self._hb_after_id = None
        self._log_queue: queue.SimpleQueue = queue.SimpleQueue()

        self._build_ui()
        self._refresh_ports()
        self._process_log_queue()
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    # -----------------------------------------------------------------------
    # UI construction
    # -----------------------------------------------------------------------

    def _build_ui(self):
        self.grid_columnconfigure(0, weight=0)
        self.grid_columnconfigure(1, weight=0)
        self.grid_columnconfigure(2, weight=0)
        self.grid_rowconfigure(1, weight=1)
        self.grid_rowconfigure(2, weight=0)

        self._build_topbar()

        # Three fixed-width panels in a row
        self._left_frame   = ctk.CTkScrollableFrame(self, width=340, corner_radius=0)
        self._center_frame = ctk.CTkScrollableFrame(self, width=400, corner_radius=0)
        self._right_frame  = ctk.CTkScrollableFrame(self, width=420, corner_radius=0)

        self._left_frame.grid  (row=1, column=0, sticky="nsew", padx=(6,3), pady=(4,4))
        self._center_frame.grid(row=1, column=1, sticky="nsew", padx=(3,3), pady=(4,4))
        self._right_frame.grid (row=1, column=2, sticky="nsew", padx=(3,6), pady=(4,4))

        self._build_left()
        self._build_center()
        self._build_right()
        self._build_log()

    # --- Top bar ------------------------------------------------------------

    def _build_topbar(self):
        bar = ctk.CTkFrame(self, height=46, corner_radius=0)
        bar.grid(row=0, column=0, columnspan=3, sticky="ew", padx=6, pady=(6, 0))

        ctk.CTkLabel(bar, text="Port:").pack(side="left", padx=(10, 2))
        self._port_combo = ctk.CTkComboBox(bar, width=110, values=[])
        self._port_combo.pack(side="left", padx=2)

        ctk.CTkButton(bar, text="Refresh", width=68,
                      command=self._refresh_ports).pack(side="left", padx=4)

        ctk.CTkLabel(bar, text="Baud:").pack(side="left", padx=(10, 2))
        self._baud_entry = ctk.CTkEntry(bar, width=75)
        self._baud_entry.insert(0, "115200")
        self._baud_entry.pack(side="left", padx=2)

        self._connect_btn = ctk.CTkButton(bar, text="Connect", width=90,
                                          fg_color="#1a7a1a", hover_color="#247a24",
                                          command=self._on_connect)
        self._connect_btn.pack(side="left", padx=(10, 4))

        self._disconnect_btn = ctk.CTkButton(bar, text="Disconnect", width=100,
                                              fg_color="#7a1a1a", hover_color="#a02020",
                                              state="disabled",
                                              command=self._on_disconnect)
        self._disconnect_btn.pack(side="left", padx=4)

        self._status_label = ctk.CTkLabel(bar, text="● OFFLINE",
                                          text_color="#FF4444",
                                          font=ctk.CTkFont(size=13, weight="bold"))
        self._status_label.pack(side="left", padx=(14, 0))

        # RX packet rate counter
        self._pkt_rate_label = ctk.CTkLabel(bar, text="0 pkt/s",
                                             font=ctk.CTkFont(size=11),
                                             text_color="#888888")
        self._pkt_rate_label.pack(side="right", padx=12)
        self._pkt_count = 0
        self._pkt_rate_last = time.time()
        self.after(1000, self._update_pkt_rate)

    # --- Left panel: Digital Inputs + ADC -----------------------------------

    def _build_left(self):
        p = self._left_frame
        self._section_label(p, "DIGITAL INPUTS")

        self._dig_indicators: dict = {}

        # KEY
        self._add_digital_row(p, "KEY", "Master key switch")

        # Switch groups — labels reflect physical hardware layout
        for grp_label, contacts in [
            ("SW1  Guard switches",          ["SW1_1", "SW1_2", "SW1_3"]),
            ("SW2  Momentary + LED buttons", ["SW2_1", "SW2_2", "SW2_3"]),
            ("SW3  Toggles (above WS2811)",  ["SW3_1", "SW3_2", "SW3_3", "SW3_4"]),
        ]:
            grp_frame = ctk.CTkFrame(p, fg_color="transparent")
            grp_frame.pack(fill="x", padx=8, pady=2)
            ctk.CTkLabel(grp_frame, text=grp_label,
                         font=ctk.CTkFont(size=11, weight="bold"),
                         text_color="#cccccc",
                         anchor="w").pack(side="left")
            for name in contacts:
                self._add_indicator_inline(grp_frame, name)

        # Timestamps
        self._dig_ts_label = ctk.CTkLabel(p, text="ts: —",
                                           font=ctk.CTkFont(size=10),
                                           text_color="#666666")
        self._dig_ts_label.pack(anchor="w", padx=10, pady=(0, 6))

        self._section_label(p, "ADC READINGS")

        ch_names = ["CH0  BAT_VIN", "CH1  EXT_VIN",
                    "CH2  SENS2",   "CH3  SENS3",
                    "CH4  SENS4",   "CH5  SENS5"]

        self._adc_raw  = []
        self._adc_volt = []
        self._adc_bar  = []

        for i, name in enumerate(ch_names):
            row = ctk.CTkFrame(p, fg_color="transparent")
            row.pack(fill="x", padx=8, pady=2)

            ctk.CTkLabel(row, text=name,
                         font=ctk.CTkFont(size=11), width=110, anchor="w").pack(side="left")

            raw_lbl = ctk.CTkLabel(row, text="0",
                                   font=ctk.CTkFont(family="Courier New", size=11),
                                   width=44, anchor="e")
            raw_lbl.pack(side="left", padx=(2, 4))
            self._adc_raw.append(raw_lbl)

            volt_lbl = ctk.CTkLabel(row, text="0.000 V",
                                    font=ctk.CTkFont(family="Courier New", size=11),
                                    width=66, anchor="e")
            volt_lbl.pack(side="left", padx=(0, 6))
            self._adc_volt.append(volt_lbl)

            bar = ctk.CTkProgressBar(row, width=100, height=10)
            bar.set(0)
            bar.pack(side="left")
            self._adc_bar.append(bar)

        self._adc_ts_label = ctk.CTkLabel(p, text="ts: —",
                                           font=ctk.CTkFont(size=10),
                                           text_color="#666666")
        self._adc_ts_label.pack(anchor="w", padx=10, pady=(0, 8))

        self._section_label(p, "AMBIENT LIGHT  (VEML7700 · 0x0B)")

        als_box = ctk.CTkFrame(p, corner_radius=6)
        als_box.pack(fill="x", padx=8, pady=(0, 8))

        als_row1 = ctk.CTkFrame(als_box, fg_color="transparent")
        als_row1.pack(fill="x", padx=8, pady=(8, 2))

        ctk.CTkLabel(als_row1, text="Lux:", width=40, anchor="w",
                     font=ctk.CTkFont(size=11)).pack(side="left")
        self._als_lux_label = ctk.CTkLabel(
            als_row1, text="—",
            font=ctk.CTkFont(family="Courier New", size=14, weight="bold"),
            text_color="#FFD700", width=100, anchor="w")
        self._als_lux_label.pack(side="left", padx=(4, 0))

        self._als_bar = ctk.CTkProgressBar(als_row1, width=100, height=10)
        self._als_bar.set(0)
        self._als_bar.pack(side="left", padx=(8, 0))

        als_row2 = ctk.CTkFrame(als_box, fg_color="transparent")
        als_row2.pack(fill="x", padx=8, pady=(2, 2))
        ctk.CTkLabel(als_row2, text="ALS raw:", width=68, anchor="w",
                     font=ctk.CTkFont(size=10),
                     text_color="#888888").pack(side="left")
        self._als_raw_label = ctk.CTkLabel(als_row2, text="—",
                                            font=ctk.CTkFont(family="Courier New", size=10),
                                            text_color="#888888", width=50, anchor="w")
        self._als_raw_label.pack(side="left")
        ctk.CTkLabel(als_row2, text="WHITE raw:", width=80, anchor="w",
                     font=ctk.CTkFont(size=10),
                     text_color="#888888").pack(side="left", padx=(8, 0))
        self._als_white_label = ctk.CTkLabel(als_row2, text="—",
                                              font=ctk.CTkFont(family="Courier New", size=10),
                                              text_color="#888888", width=50, anchor="w")
        self._als_white_label.pack(side="left")

        self._als_ts_label = ctk.CTkLabel(als_box, text="ts: —",
                                           font=ctk.CTkFont(size=10),
                                           text_color="#666666")
        self._als_ts_label.pack(anchor="w", padx=10, pady=(2, 8))

    def _add_digital_row(self, parent, name: str, tooltip: str = ""):
        row = ctk.CTkFrame(parent, fg_color="transparent")
        row.pack(fill="x", padx=8, pady=2)
        canvas = tk.Canvas(row, width=14, height=14,
                           bg="#1c1c1c", highlightthickness=0)
        canvas.create_oval(2, 2, 12, 12, fill="#444444", outline="", tags="dot")
        canvas.pack(side="left", padx=(0, 6))
        ctk.CTkLabel(row, text=name,
                     font=ctk.CTkFont(size=11), anchor="w").pack(side="left")
        if tooltip:
            ctk.CTkLabel(row, text=f"  ({tooltip})",
                         font=ctk.CTkFont(size=10),
                         text_color="#666666", anchor="w").pack(side="left")
        self._dig_indicators[name] = canvas

    def _add_indicator_inline(self, parent, name: str):
        canvas = tk.Canvas(parent, width=14, height=14,
                           bg="#1c1c1c", highlightthickness=0)
        canvas.create_oval(2, 2, 12, 12, fill="#444444", outline="", tags="dot")
        canvas.pack(side="left", padx=(4, 0))
        ctk.CTkLabel(parent, text=name[-1],
                     font=ctk.CTkFont(size=10),
                     text_color="#aaaaaa").pack(side="left", padx=(1, 2))
        self._dig_indicators[name] = canvas

    # --- Center panel: Commands ---------------------------------------------

    def _build_center(self):
        p = self._center_frame

        # --- System State ---------------------------------------------------
        self._section_label(p, "SYSTEM STATE")
        state_box = ctk.CTkFrame(p, corner_radius=6)
        state_box.pack(fill="x", padx=8, pady=(0, 8))

        row = ctk.CTkFrame(state_box, fg_color="transparent")
        row.pack(fill="x", padx=8, pady=(8, 4))
        ctk.CTkLabel(row, text="Current:", width=60, anchor="w").pack(side="left")
        self._state_label = ctk.CTkLabel(row, text="UNKNOWN",
                                          font=ctk.CTkFont(size=13, weight="bold"),
                                          text_color="#FFAA00")
        self._state_label.pack(side="left")

        row2 = ctk.CTkFrame(state_box, fg_color="transparent")
        row2.pack(fill="x", padx=8, pady=(0, 8))
        ctk.CTkLabel(row2, text="Override:", width=60, anchor="w").pack(side="left")
        self._state_combo = ctk.CTkComboBox(row2, values=GCSProtocol.STATE_NAMES, width=150)
        self._state_combo.set("ACTIVE")
        self._state_combo.pack(side="left", padx=(0, 8))
        ctk.CTkButton(row2, text="Send", width=70,
                      command=self._send_state_override).pack(side="left")

        # --- Screen Mode ----------------------------------------------------
        self._section_label(p, "SCREEN MODE")
        screen_box = ctk.CTkFrame(p, corner_radius=6)
        screen_box.pack(fill="x", padx=8, pady=(0, 8))

        row3 = ctk.CTkFrame(screen_box, fg_color="transparent")
        row3.pack(fill="x", padx=8, pady=8)
        ctk.CTkLabel(row3, text="Mode:", width=50, anchor="w").pack(side="left")
        self._screen_combo = ctk.CTkComboBox(row3, values=GCSProtocol.SCREEN_NAMES, width=130)
        self._screen_combo.set("AUTO")
        self._screen_combo.pack(side="left", padx=(0, 8))
        ctk.CTkButton(row3, text="Send", width=70,
                      command=self._send_screen_mode).pack(side="left")

        # --- Heartbeat ------------------------------------------------------
        self._section_label(p, "HEARTBEAT")
        hb_box = ctk.CTkFrame(p, corner_radius=6)
        hb_box.pack(fill="x", padx=8, pady=(0, 8))

        hb_row1 = ctk.CTkFrame(hb_box, fg_color="transparent")
        hb_row1.pack(fill="x", padx=8, pady=(8, 2))
        self._hb_seq_label = ctk.CTkLabel(hb_row1, text="SEQ: 0",
                                           font=ctk.CTkFont(family="Courier New", size=12))
        self._hb_seq_label.pack(side="left", padx=(0, 16))
        ctk.CTkLabel(hb_row1, text="Interval (ms):").pack(side="left")
        self._hb_interval_entry = ctk.CTkEntry(hb_row1, width=60)
        self._hb_interval_entry.insert(0, "1000")
        self._hb_interval_entry.pack(side="left", padx=(4, 0))

        hb_row2 = ctk.CTkFrame(hb_box, fg_color="transparent")
        hb_row2.pack(fill="x", padx=8, pady=(2, 8))
        self._hb_auto_switch = ctk.CTkSwitch(hb_row2, text="Auto-Send",
                                              command=self._toggle_hb_auto)
        self._hb_auto_switch.pack(side="left", padx=(0, 16))
        ctk.CTkButton(hb_row2, text="Send Once", width=90,
                      command=self._send_heartbeat_once).pack(side="left")

        # --- Warning Panel --------------------------------------------------
        self._section_label(p, "WARNING PANEL  (0x0A)")
        warn_box = ctk.CTkFrame(p, corner_radius=6)
        warn_box.pack(fill="x", padx=8, pady=(0, 8))

        self._warn_combos = []
        self._warn_rows   = []

        for i, name in enumerate(GCSProtocol.WARN_NAMES):
            row = ctk.CTkFrame(warn_box, corner_radius=4, fg_color="#222222")
            row.pack(fill="x", padx=6, pady=2)
            ctk.CTkLabel(row, text=f"{i}  {name}",
                         font=ctk.CTkFont(size=11), width=140, anchor="w").pack(side="left", padx=6)
            combo = ctk.CTkComboBox(row, values=GCSProtocol.WARN_SEVERITY_NAMES,
                                    width=110,
                                    command=lambda val, r=row, idx=i: self._on_warn_changed(r, val))
            combo.set("OK")
            combo.pack(side="left", padx=4, pady=4)
            self._warn_combos.append(combo)
            self._warn_rows.append(row)

        ctk.CTkButton(warn_box, text="Send All Warnings",
                      command=self._send_warnings).pack(padx=8, pady=(4, 8))

    # --- Right panel: LED Controls ------------------------------------------

    def _build_right(self):
        p = self._right_frame

        # --- Indicator LEDs (MCP23017 / Chain 0x02) -------------------------
        self._section_label(p, "INDICATOR LEDs  (Chain 0x02)")
        ind_box = ctk.CTkFrame(p, corner_radius=6)
        ind_box.pack(fill="x", padx=8, pady=(0, 8))

        ind_row = ctk.CTkFrame(ind_box, fg_color="transparent")
        ind_row.pack(fill="x", padx=8, pady=8)

        self._ind_checks = []
        for i, name in enumerate(["LED2  (PA0)", "LED3  (PA1)", "LED4  (PA2)"]):
            chk = ctk.CTkCheckBox(ind_row, text=name, width=110,
                                  command=self._send_indicator_leds)
            chk.pack(side="left", padx=6)
            self._ind_checks.append(chk)

        # --- Button LEDs (WS2811 / Chain 0x01) ------------------------------
        self._section_label(p, "BUTTON LEDs  (Chain 0x01 · WS2811)")
        btn_led_box = ctk.CTkFrame(p, corner_radius=6)
        btn_led_box.pack(fill="x", padx=8, pady=(0, 8))

        self._ws_r    = []
        self._ws_g    = []
        self._ws_b    = []
        self._ws_anim = []
        self._ws_preview = []

        for idx, label in enumerate(["LED5  (id=0)", "LED6  (id=1)"]):
            sub = ctk.CTkFrame(btn_led_box, corner_radius=4, fg_color="#222222")
            sub.pack(fill="x", padx=6, pady=(6, 2))

            header = ctk.CTkFrame(sub, fg_color="transparent")
            header.pack(fill="x", padx=6, pady=(4, 2))
            ctk.CTkLabel(header, text=label,
                         font=ctk.CTkFont(size=12, weight="bold")).pack(side="left")

            # Color preview canvas
            preview = tk.Canvas(header, width=40, height=20,
                                bg="#000000", highlightthickness=1,
                                highlightbackground="#555555")
            preview.create_rectangle(0, 0, 40, 20, fill="#000000",
                                     outline="", tags="fill")
            preview.pack(side="right", padx=6)
            self._ws_preview.append(preview)

            def make_slider_row(parent, ch_label, store_list, btn_idx):
                r = ctk.CTkFrame(parent, fg_color="transparent")
                r.pack(fill="x", padx=6, pady=1)
                ctk.CTkLabel(r, text=ch_label, width=18, anchor="w",
                             font=ctk.CTkFont(size=11)).pack(side="left")
                val_lbl = ctk.CTkLabel(r, text="000", width=32,
                                       font=ctk.CTkFont(family="Courier New", size=11))
                val_lbl.pack(side="left", padx=(2, 4))
                sl = ctk.CTkSlider(r, from_=0, to=255, width=160,
                                   command=lambda v, lbl=val_lbl, bi=btn_idx: (
                                       lbl.configure(text=f"{int(v):03d}"),
                                       self._update_ws_preview(bi)
                                   ))
                sl.set(0)
                sl.pack(side="left")
                store_list.append(sl)

            make_slider_row(sub, "R", self._ws_r, idx)
            make_slider_row(sub, "G", self._ws_g, idx)
            make_slider_row(sub, "B", self._ws_b, idx)

            anim_row = ctk.CTkFrame(sub, fg_color="transparent")
            anim_row.pack(fill="x", padx=6, pady=(2, 4))
            ctk.CTkLabel(anim_row, text="Anim:", width=40, anchor="w").pack(side="left")
            anim_combo = ctk.CTkComboBox(anim_row, values=GCSProtocol.ANIM_NAMES, width=130)
            anim_combo.set("ON")
            anim_combo.pack(side="left", padx=(0, 8))
            self._ws_anim.append(anim_combo)

            btn_idx_capture = idx
            ctk.CTkButton(anim_row, text=f"Send {label.split()[0]}", width=100,
                          command=lambda bi=btn_idx_capture: self._send_ws2811(bi)
                          ).pack(side="left")

        # --- SK6812 Strip (Chain 0x00) --------------------------------------
        self._section_label(p, "SK6812 STRIP  (Chain 0x00 · GRBW)")
        sk_box = ctk.CTkFrame(p, corner_radius=6)
        sk_box.pack(fill="x", padx=8, pady=(0, 8))

        range_row = ctk.CTkFrame(sk_box, fg_color="transparent")
        range_row.pack(fill="x", padx=8, pady=(8, 4))
        ctk.CTkLabel(range_row, text="Start px:", width=60, anchor="w").pack(side="left")
        self._sk_start = ctk.CTkEntry(range_row, width=50)
        self._sk_start.insert(0, "0")
        self._sk_start.pack(side="left", padx=(0, 12))
        ctk.CTkLabel(range_row, text="End px:", width=55, anchor="w").pack(side="left")
        self._sk_end = ctk.CTkEntry(range_row, width=50)
        self._sk_end.insert(0, "60")
        self._sk_end.pack(side="left")

        note = ctk.CTkLabel(sk_box, text=f"Max {GCSProtocol.SK6812_MAX_PIXELS_PER_SEND} px/send (protocol limit)",
                             font=ctk.CTkFont(size=10), text_color="#888888")
        note.pack(anchor="w", padx=10, pady=(0, 4))

        self._sk_g = self._sk_r = self._sk_b = self._sk_w = None  # will be set below
        self._sk_preview = None

        sk_color_frame = ctk.CTkFrame(sk_box, fg_color="transparent")
        sk_color_frame.pack(fill="x", padx=8, pady=2)

        sk_preview_canvas = tk.Canvas(sk_color_frame, width=50, height=28,
                                       bg="#000000", highlightthickness=1,
                                       highlightbackground="#555555")
        sk_preview_canvas.create_rectangle(0, 0, 50, 28, fill="#000000",
                                            outline="", tags="fill")
        sk_preview_canvas.pack(side="right", padx=6)
        self._sk_preview = sk_preview_canvas

        def sk_slider_row(parent, ch_label):
            r = ctk.CTkFrame(parent, fg_color="transparent")
            r.pack(fill="x", padx=0, pady=1)
            ctk.CTkLabel(r, text=ch_label, width=18, anchor="w",
                         font=ctk.CTkFont(size=11)).pack(side="left")
            val_lbl = ctk.CTkLabel(r, text="000", width=32,
                                   font=ctk.CTkFont(family="Courier New", size=11))
            val_lbl.pack(side="left", padx=(2, 4))
            sl = ctk.CTkSlider(r, from_=0, to=255, width=180,
                               command=lambda v, lbl=val_lbl: (
                                   lbl.configure(text=f"{int(v):03d}"),
                                   self._update_sk_preview()
                               ))
            sl.set(0)
            sl.pack(side="left")
            return sl

        self._sk_g = sk_slider_row(sk_color_frame, "G")
        self._sk_r = sk_slider_row(sk_color_frame, "R")
        self._sk_b = sk_slider_row(sk_color_frame, "B")
        self._sk_w = sk_slider_row(sk_color_frame, "W")

        btn_row = ctk.CTkFrame(sk_box, fg_color="transparent")
        btn_row.pack(fill="x", padx=8, pady=(4, 8))
        ctk.CTkButton(btn_row, text="Fill Range", width=110,
                      command=self._send_sk6812_fill).pack(side="left", padx=(0, 8))
        ctk.CTkButton(btn_row, text="Clear All (Black)", width=130,
                      fg_color="#7a1a1a", hover_color="#a02020",
                      command=self._send_sk6812_clear).pack(side="left")

        # --- Brightness -----------------------------------------------------
        self._section_label(p, "BRIGHTNESS  (0x08)")
        bright_box = ctk.CTkFrame(p, corner_radius=6)
        bright_box.pack(fill="x", padx=8, pady=(0, 8))

        br_row = ctk.CTkFrame(bright_box, fg_color="transparent")
        br_row.pack(fill="x", padx=8, pady=8)
        ctk.CTkLabel(br_row, text="Target:", width=50, anchor="w").pack(side="left")
        self._bright_target = ctk.CTkComboBox(br_row,
                                               values=["SK6812 Strip (0)", "WS2811 Buttons (1)"],
                                               width=160)
        self._bright_target.set("SK6812 Strip (0)")
        self._bright_target.pack(side="left", padx=(0, 10))

        br_row2 = ctk.CTkFrame(bright_box, fg_color="transparent")
        br_row2.pack(fill="x", padx=8, pady=(0, 8))
        ctk.CTkLabel(br_row2, text="Level:", width=50, anchor="w").pack(side="left")
        self._bright_val_label = ctk.CTkLabel(br_row2, text="128",
                                               font=ctk.CTkFont(family="Courier New", size=11),
                                               width=36)
        self._bright_val_label.pack(side="left")
        self._bright_slider = ctk.CTkSlider(
            br_row2, from_=0, to=255, width=160,
            command=lambda v: self._bright_val_label.configure(text=f"{int(v):3d}"))
        self._bright_slider.set(128)
        self._bright_slider.pack(side="left", padx=(4, 10))
        ctk.CTkButton(br_row2, text="Send", width=70,
                      command=self._send_brightness).pack(side="left")

    # --- Log panel ----------------------------------------------------------

    def _build_log(self):
        log_frame = ctk.CTkFrame(self, height=185, corner_radius=0)
        log_frame.grid(row=2, column=0, columnspan=3, sticky="ew", padx=6, pady=(0, 6))
        log_frame.grid_columnconfigure(0, weight=1)
        log_frame.grid_rowconfigure(1, weight=1)

        hdr = ctk.CTkFrame(log_frame, height=22, fg_color="transparent")
        hdr.grid(row=0, column=0, sticky="ew", padx=4, pady=(4, 0))
        ctk.CTkLabel(hdr, text="PACKET LOG",
                     font=ctk.CTkFont(size=11, weight="bold")).pack(side="left", padx=6)
        ctk.CTkButton(hdr, text="Clear", width=60, height=20,
                      command=self._clear_log).pack(side="right", padx=6)

        self._log_box = ctk.CTkTextbox(log_frame, height=155,
                                        font=ctk.CTkFont(family="Courier New", size=11),
                                        state="disabled")
        self._log_box.grid(row=1, column=0, sticky="nsew", padx=4, pady=(2, 4))

        tb = self._log_box._textbox
        tb.tag_config("TX",    foreground="#4488FF")
        tb.tag_config("RX",    foreground="#44CC44")
        tb.tag_config("ERR",   foreground="#FF4444")
        tb.tag_config("EVENT", foreground="#FFAA00")
        tb.tag_config("INFO",  foreground="#AAAAAA")

    # -----------------------------------------------------------------------
    # Helper widgets
    # -----------------------------------------------------------------------

    def _section_label(self, parent, text: str):
        ctk.CTkLabel(parent, text=text,
                     font=ctk.CTkFont(size=12, weight="bold"),
                     text_color="#88AAFF",
                     anchor="w").pack(fill="x", padx=8, pady=(10, 3))
        # Thin divider
        div = tk.Frame(parent, height=1, bg="#333366")
        div.pack(fill="x", padx=8, pady=(0, 4))

    # -----------------------------------------------------------------------
    # TX senders
    # -----------------------------------------------------------------------

    def _send_state_override(self):
        idx = GCSProtocol.STATE_NAMES.index(self._state_combo.get()) \
              if self._state_combo.get() in GCSProtocol.STATE_NAMES else 0
        payload = bytes([idx])
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_MODE, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_MODE, payload,
                         f"→ {GCSProtocol.STATE_NAMES[idx]}")

    def _send_screen_mode(self):
        idx = GCSProtocol.SCREEN_NAMES.index(self._screen_combo.get()) \
              if self._screen_combo.get() in GCSProtocol.SCREEN_NAMES else 0
        payload = bytes([idx])
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_SCREEN, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_SCREEN, payload,
                         f"→ {GCSProtocol.SCREEN_NAMES[idx]}")

    def _send_heartbeat_once(self):
        seq = self._hb_seq & 0xFF
        payload = bytes([seq])
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_HEARTBEAT, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_HEARTBEAT, payload, f"seq={seq}")
            self._hb_seq = (self._hb_seq + 1) & 0xFF

    def _send_warnings(self):
        payload = bytes(
            GCSProtocol.WARN_SEVERITY_NAMES.index(self._warn_combos[i].get())
            if self._warn_combos[i].get() in GCSProtocol.WARN_SEVERITY_NAMES else 0
            for i in range(9)
        )
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_WARNING, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_WARNING, payload)

    def _send_indicator_leds(self):
        mask  = 0b111
        state = 0
        for i, chk in enumerate(self._ind_checks):
            if chk.get():
                state |= (1 << i)
        payload = bytes([GCSProtocol.CHAIN_INDICATOR, mask, state])
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_LED, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_LED, payload,
                         f"indicators mask={mask:#04x} state={state:#04x}")

    def _send_ws2811(self, button_id: int):
        r    = int(self._ws_r[button_id].get())
        g    = int(self._ws_g[button_id].get())
        b    = int(self._ws_b[button_id].get())
        anim = GCSProtocol.ANIM_NAMES.index(self._ws_anim[button_id].get()) \
               if self._ws_anim[button_id].get() in GCSProtocol.ANIM_NAMES else 1
        payload = bytes([GCSProtocol.CHAIN_WS2811, button_id, r, g, b, anim])
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_LED, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_LED, payload,
                         f"WS2811 id={button_id} R={r} G={g} B={b} anim={GCSProtocol.ANIM_NAMES[anim]}")

    def _send_sk6812_fill(self):
        try:
            start = max(0, min(127, int(self._sk_start.get())))
            end   = max(start, min(127, int(self._sk_end.get())))
        except ValueError:
            self._log_info("Invalid start/end pixel value")
            return

        g_val = int(self._sk_g.get())
        r_val = int(self._sk_r.get())
        b_val = int(self._sk_b.get())
        w_val = int(self._sk_w.get())

        cap = GCSProtocol.SK6812_MAX_PIXELS_PER_SEND
        # Send in chunks of at most cap pixels
        px = start
        while px <= end:
            chunk_end  = min(end, px + cap - 1)
            # Payload: all pixels from 0 to chunk_end (zeros before 'px')
            num_pixels = chunk_end + 1
            pixel_data = bytearray(num_pixels * 4)
            for p in range(px, chunk_end + 1):
                base = p * 4
                pixel_data[base]     = g_val
                pixel_data[base + 1] = r_val
                pixel_data[base + 2] = b_val
                pixel_data[base + 3] = w_val
            payload = bytes([GCSProtocol.CHAIN_SK6812, num_pixels]) + bytes(pixel_data)
            pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_LED, payload)
            if self._driver.send(pkt):
                self._log_tx(GCSProtocol.TYPE_LED, payload,
                             f"SK6812 px {px}-{chunk_end} G={g_val} R={r_val} B={b_val} W={w_val}")
            px = chunk_end + 1

    def _send_sk6812_clear(self):
        cap = GCSProtocol.SK6812_MAX_PIXELS_PER_SEND
        for start in range(0, 128, cap):
            num = min(cap, 128 - start)
            pixel_data = bytes(num * 4)
            payload = bytes([GCSProtocol.CHAIN_SK6812, num]) + pixel_data
            pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_LED, payload)
            self._driver.send(pkt)
        self._log_tx(GCSProtocol.TYPE_LED, b"", "SK6812 clear all (black)")

    def _send_brightness(self):
        target = 0 if "SK6812" in self._bright_target.get() else 1
        level  = int(self._bright_slider.get())
        payload = bytes([target, level])
        pkt = GCSProtocol.build_packet(GCSProtocol.TYPE_BRIGHTNESS, payload)
        if self._driver.send(pkt):
            self._log_tx(GCSProtocol.TYPE_BRIGHTNESS, payload,
                         f"target={target} level={level}")

    # -----------------------------------------------------------------------
    # RX handlers (called via self.after() — main thread only)
    # -----------------------------------------------------------------------

    def _update_adc(self, payload: bytes):
        try:
            channels = struct.unpack_from("<6H", payload, 0)
            ts = struct.unpack_from("<H", payload, 12)[0]
        except struct.error:
            return
        for i, raw in enumerate(channels):
            v = (raw / 4095.0) * 3.3
            if i < 2:
                v *= 4.0
            self._adc_raw[i].configure(text=str(raw))
            self._adc_volt[i].configure(text=f"{v:6.3f}V")
            self._adc_bar[i].set(raw / 4095.0)
        self._adc_ts_label.configure(text=f"ts: {ts} ms")
        self._pkt_count += 1

    def _update_als(self, payload: bytes):
        # als_packet_t: als_raw(u16) + white_raw(u16) + lux_milli(u32) + ts_ms(u16) = 10 bytes
        try:
            als_raw, white_raw, lux_milli, ts = struct.unpack_from("<HHIH", payload, 0)
        except struct.error:
            return
        lux = lux_milli / 1000.0
        self._als_lux_label.configure(text=f"{lux:,.2f} lx")
        # Bar: 0–120000 lux range (VEML7700 max with gain 1/8)
        self._als_bar.set(min(1.0, lux / 120000.0))
        self._als_raw_label.configure(text=str(als_raw))
        self._als_white_label.configure(text=str(white_raw))
        self._als_ts_label.configure(text=f"ts: {ts} ms")
        self._pkt_count += 1

    def _update_digital(self, port_a: int, port_b: int, ts: int):
        mapping = {
            "KEY":   (port_a, 5),
            "SW3_3": (port_a, 6),
            "SW3_4": (port_a, 7),
            "SW1_1": (port_b, 0), "SW1_2": (port_b, 1), "SW1_3": (port_b, 2),
            "SW2_1": (port_b, 3), "SW2_2": (port_b, 4), "SW2_3": (port_b, 5),
            "SW3_1": (port_b, 6), "SW3_2": (port_b, 7),
        }
        for name, (reg, bit) in mapping.items():
            active = not bool((reg >> bit) & 1)  # active-low
            color = "#00CC44" if active else "#444444"
            canvas = self._dig_indicators.get(name)
            if canvas:
                canvas.itemconfig("dot", fill=color)
        self._dig_ts_label.configure(text=f"ts: {ts} ms")
        self._pkt_count += 1

    def _handle_event(self, evt_id: int, value: int):
        evt_map = {
            0x01: lambda v: f"SWITCH_CHANGED  port_a={v >> 8:#04x}  port_b={v & 0xFF:#04x}",
            0x02: lambda v: f"BUTTON_PRESSED  id={v}",
            0x03: lambda v: f"BUTTON_RELEASED id={v}",
            0x04: lambda v: f"KEY_LOCK_CHANGED  {'LOCKED' if v == 0 else 'UNLOCKED'}",
            0x05: lambda v: f"SOURCE_DETECTED mask={v:#04x}",
            0x06: lambda v: "USB_CONNECTED",
            0x07: lambda v: "USB_DISCONNECTED",
        }
        desc = evt_map.get(evt_id, lambda v: f"UNKNOWN evt={evt_id:#04x} val={v}")(value)
        self._log_queue.put(("EVENT", f"EVENT  {desc}"))

    def _on_rx_packet(self, msg_type, payload):
        """Called from RX thread — must NOT touch widgets directly."""
        if msg_type is None:
            self._log_queue.put(("ERR", "Serial read error / unexpected disconnect"))
            self.after(0, self._handle_unexpected_disconnect)
            return

        self._log_rx(msg_type, payload)

        if msg_type == GCSProtocol.TYPE_ADC:
            if len(payload) >= 14:
                self.after(0, self._update_adc, payload)

        elif msg_type == GCSProtocol.TYPE_DIGITAL:
            if len(payload) >= 4:
                port_a = payload[0]
                port_b = payload[1]
                ts     = struct.unpack_from("<H", payload, 2)[0]
                self.after(0, self._update_digital, port_a, port_b, ts)

        elif msg_type == GCSProtocol.TYPE_HEARTBEAT:
            seq = payload[0] if payload else 0
            self.after(0, lambda s=seq: self._hb_seq_label.configure(text=f"SEQ: {s}"))

        elif msg_type == GCSProtocol.TYPE_EVENT:
            if len(payload) >= 3:
                evt_id, value = payload[0], struct.unpack_from("<H", payload, 1)[0]
                self.after(0, self._handle_event, evt_id, value)

        elif msg_type == GCSProtocol.TYPE_ALS:
            if len(payload) >= 10:
                self.after(0, self._update_als, payload)

        elif msg_type == GCSProtocol.TYPE_ERROR:
            code = payload[0] if payload else 0
            err_names = {1: "WATCHDOG_RESET", 2: "STACK_OVERFLOW", 3: "MALLOC_FAILED"}
            name = err_names.get(code, f"UNKNOWN({code:#04x})")
            self._log_queue.put(("ERR", f"DEVICE ERROR: {name}"))

    # -----------------------------------------------------------------------
    # Connection management
    # -----------------------------------------------------------------------

    def _refresh_ports(self):
        ports = SerialDriver.list_ports()
        self._port_combo.configure(values=ports if ports else [""])
        if ports:
            self._port_combo.set(ports[0])

    def _on_connect(self):
        port = self._port_combo.get().strip()
        if not port:
            self._log_info("No port selected")
            return
        try:
            baud = int(self._baud_entry.get())
        except ValueError:
            baud = 115200

        if self._driver.connect(port, baud):
            self._status_label.configure(text=f"● ONLINE  {port}", text_color="#44CC44")
            self._connect_btn.configure(state="disabled")
            self._disconnect_btn.configure(state="normal")
            self._log_info(f"Connected to {port} @ {baud}")
        else:
            self._status_label.configure(text="● FAILED", text_color="#FF4444")
            self._log_info(f"Failed to open {port}")

    def _on_disconnect(self):
        self._stop_hb_auto()
        self._driver.disconnect()
        self._status_label.configure(text="● OFFLINE", text_color="#FF4444")
        self._connect_btn.configure(state="normal")
        self._disconnect_btn.configure(state="disabled")
        self._log_info("Disconnected")

    def _handle_unexpected_disconnect(self):
        if not self._driver.is_connected():
            self._stop_hb_auto()
            self._status_label.configure(text="● LOST", text_color="#FF4444")
            self._connect_btn.configure(state="normal")
            self._disconnect_btn.configure(state="disabled")

    # -----------------------------------------------------------------------
    # Heartbeat helpers
    # -----------------------------------------------------------------------

    def _toggle_hb_auto(self):
        self._hb_auto = bool(self._hb_auto_switch.get())
        if self._hb_auto:
            self._schedule_next_hb()
        else:
            self._stop_hb_auto()

    def _stop_hb_auto(self):
        self._hb_auto = False
        if self._hb_after_id:
            try:
                self.after_cancel(self._hb_after_id)
            except Exception:
                pass
            self._hb_after_id = None

    def _schedule_next_hb(self):
        if not self._hb_auto or not self._driver.is_connected():
            return
        self._send_heartbeat_once()
        try:
            interval = max(100, int(self._hb_interval_entry.get()))
        except ValueError:
            interval = 1000
        self._hb_after_id = self.after(interval, self._schedule_next_hb)

    # -----------------------------------------------------------------------
    # Warning panel colour feedback
    # -----------------------------------------------------------------------

    def _on_warn_changed(self, row_frame, value: str):
        colors = {"OK": "#0d2e0d", "WARNING": "#3a2800", "CRITICAL": "#3a0000"}
        row_frame.configure(fg_color=colors.get(value, "#222222"))

    # -----------------------------------------------------------------------
    # LED preview helpers
    # -----------------------------------------------------------------------

    def _update_ws_preview(self, idx: int):
        r = int(self._ws_r[idx].get())
        g = int(self._ws_g[idx].get())
        b = int(self._ws_b[idx].get())
        color = f"#{r:02x}{g:02x}{b:02x}"
        self._ws_preview[idx].itemconfig("fill", fill=color)

    def _update_sk_preview(self):
        g = int(self._sk_g.get())
        r = int(self._sk_r.get())
        b = int(self._sk_b.get())
        w = int(self._sk_w.get())
        # Blend white channel additively for preview approximation
        r_eff = min(255, r + w)
        g_eff = min(255, g + w)
        b_eff = min(255, b + w)
        color = f"#{r_eff:02x}{g_eff:02x}{b_eff:02x}"
        self._sk_preview.itemconfig("fill", fill=color)

    # -----------------------------------------------------------------------
    # Packet log
    # -----------------------------------------------------------------------

    def _log_tx(self, msg_type: int, payload: bytes, note: str = ""):
        hex_str = payload.hex(" ").upper() if payload else "(empty)"
        tag_str = f"  {note}" if note else ""
        self._log_queue.put(("TX", f"TX  [{msg_type:#04x}]  {hex_str}{tag_str}"))

    def _log_rx(self, msg_type: int, payload: bytes):
        hex_str = payload.hex(" ").upper() if payload else "(empty)"
        self._log_queue.put(("RX", f"RX  [{msg_type:#04x}]  {hex_str}"))

    def _log_info(self, text: str):
        self._log_queue.put(("INFO", text))

    def _clear_log(self):
        self._log_box.configure(state="normal")
        self._log_box.delete("1.0", "end")
        self._log_box.configure(state="disabled")

    def _process_log_queue(self):
        try:
            while True:
                tag, msg = self._log_queue.get_nowait()
                ts   = time.strftime("%H:%M:%S")
                line = f"[{ts}]  {msg}\n"
                tb   = self._log_box._textbox
                self._log_box.configure(state="normal")
                tb.insert("end", line, tag)
                self._log_box.configure(state="disabled")
                tb.see("end")
        except queue.Empty:
            pass
        self.after(50, self._process_log_queue)

    # -----------------------------------------------------------------------
    # Packet rate counter
    # -----------------------------------------------------------------------

    def _update_pkt_rate(self):
        now     = time.time()
        elapsed = now - self._pkt_rate_last
        if elapsed > 0:
            rate = self._pkt_count / elapsed
            self._pkt_rate_label.configure(text=f"{rate:.0f} pkt/s")
        self._pkt_count = 0
        self._pkt_rate_last = now
        self.after(1000, self._update_pkt_rate)

    # -----------------------------------------------------------------------
    # Window close
    # -----------------------------------------------------------------------

    def _on_close(self):
        self._stop_hb_auto()
        self._driver.disconnect()
        self.destroy()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    app = GCSTesterApp()
    app.mainloop()
