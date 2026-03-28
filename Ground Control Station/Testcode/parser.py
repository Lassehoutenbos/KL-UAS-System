import customtkinter as ctk
import serial
from PIL import Image, ImageTk
import cairosvg
import io

START_BYTE = 0xAA


def checksum(t, l, p):
    c = t ^ l
    for b in p:
        c ^= b
    return c


def pkt(t, payload):
    l = len(payload)
    return bytes([START_BYTE, t, l]) + payload + bytes([checksum(t, l, payload)])


class App(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.geometry("1400x900")
        self.title("GCS Panel (SVG)")

        self.ser = None

        self.build()

    # ================= UI =================
    def build(self):
        top = ctk.CTkFrame(self)
        top.pack(fill="x")

        self.port = ctk.CTkEntry(top, width=120)
        self.port.insert(0, "COM5")
        self.port.pack(side="left", padx=10)

        ctk.CTkButton(top, text="Connect", command=self.connect).pack(side="left")

        self.status = ctk.CTkLabel(top, text="OFFLINE", text_color="red")
        self.status.pack(side="left", padx=20)

        # ===== SVG LOAD =====
        png_data = cairosvg.svg2png(url="KL-GCS-PAN01.svg")
        image = Image.open(io.BytesIO(png_data))
        self.img = ImageTk.PhotoImage(image)

        canvas = ctk.CTkCanvas(self, width=image.width, height=image.height)
        canvas.pack()

        canvas.create_image(0, 0, anchor="nw", image=self.img)

        self.canvas = canvas

        # ===== HOTSPOTS =====
        self.add_hotspots()

    # ================= HOTSPOTS =================
    def add_hotspots(self):
        # 🔴 FUSE SWITCHES (pas coords aan indien nodig)
        self.make_button(600, 250, 650, 320, lambda: self.send(0x09, bytes([0x10, 1])))
        self.make_button(670, 250, 720, 320, lambda: self.send(0x09, bytes([0x11, 1])))
        self.make_button(740, 250, 790, 320, lambda: self.send(0x09, bytes([0x12, 1])))

        # 🟢 FUNCTION BUTTONS
        self.make_button(600, 340, 650, 380, lambda: self.send(0x09, bytes([0x20, 1])))
        self.make_button(660, 340, 710, 380, lambda: self.send(0x09, bytes([0x21, 1])))
        self.make_button(720, 340, 770, 380, lambda: self.send(0x09, bytes([0x22, 1])))

        # 🔵 SOURCE
        self.make_button(500, 400, 580, 460, lambda: self.send(0x09, bytes([0x40, 1])))

        # 🟡 SWITCHES
        self.make_button(850, 250, 950, 350, lambda: self.send(0x09, bytes([0x30, 1])))
        self.make_button(960, 250, 1060, 350, lambda: self.send(0x09, bytes([0x31, 1])))

        # 🟣 FAN
        self.make_button(750, 400, 820, 470, lambda: self.send(0x09, bytes([0x50, 1])))

    def make_button(self, x1, y1, x2, y2, cmd):
        rect = self.canvas.create_rectangle(
            x1, y1, x2, y2,
            outline="", fill=""
        )
        self.canvas.tag_bind(rect, "<Button-1>", lambda e: cmd())

    # ================= SERIAL =================
    def connect(self):
        try:
            self.ser = serial.Serial(self.port.get(), 115200)
            self.status.configure(text="ONLINE", text_color="green")
        except:
            self.status.configure(text="FAIL", text_color="red")

    def send(self, t, payload):
        if self.ser:
            self.ser.write(pkt(t, payload))


if __name__ == "__main__":
    App().mainloop()