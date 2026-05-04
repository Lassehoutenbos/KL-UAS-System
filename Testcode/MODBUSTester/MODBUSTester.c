/*
 * RS-485 Peripheral Bus Emulator for Raspberry Pi Pico W
 * ------------------------------------------------------
 * Emulates the four peripherals defined in docs/RS485_PERIPHERAL_BUS.md
 *   0x01  Searchlight
 *   0x02  Radar / rangefinder
 *   0x03  Pan-tilt unit
 *   0x04  External lighting bar
 *
 * The Pico is wired as a single RS-485 slave node carrying multiple virtual
 * peripheral addresses. The GCS Pico (master) drives the bus; this tester
 * answers any frame addressed to one of the enabled virtual peripherals.
 *
 * Emulated values (brightness, temp, distance, pan/tilt, etc.) can be poked
 * live from a host PC over USB CDC. Type "help" in the USB serial console.
 *
 * --- Wiring (SP3485 / MAX485 transceiver) -------------------------------
 *
 *    Pico GP4  (UART1 TX) ──── transceiver pin 4 (DI)
 *    Pico GP5  (UART1 RX) ──── transceiver pin 1 (RO)
 *    Pico GP3             ──┬─ transceiver pin 3 (DE)
 *                           └─ transceiver pin 2 (/RE)   (DE & /RE tied)
 *    Pico GP6  (/INT out) ──── 8-pin connector pin 6     (open-drain,
 *                                pull-up to 3V3 already present on master)
 *    Pico 3V3             ──── transceiver pin 8 (VCC) + 100 nF to GND
 *    Pico GND             ──── transceiver pin 5 (GND)
 *
 *    transceiver pin 6 (A) ── bus A   (yellow)
 *    transceiver pin 7 (B) ── bus B   (green)
 *
 *    120 Ohm termination across A/B if this Pico sits at a bus end.
 *    560 Ohm A->3V3 / B->GND bias only if no other node biases the bus.
 *
 *    Power: feed the Pico from VBUS via USB while bench-testing, or from
 *    the +5V pin of the GX16 connector through a 5V->VSYS path.
 *
 * Build: standard Pico SDK, pico_w board, USB stdio enabled.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "web.h"

/* ------------------------------------------------------------------ */
/* Wi-Fi AP credentials (change to taste)                             */
/* ------------------------------------------------------------------ */
#ifndef WEB_AP_SSID
#define WEB_AP_SSID     "MODBUSTester"
#endif
#ifndef WEB_AP_PASSWORD
#define WEB_AP_PASSWORD "modbus1234"        /* >=8 chars for WPA2     */
#endif

/* ------------------------------------------------------------------ */
/* Pin / bus configuration                                            */
/* ------------------------------------------------------------------ */
#define RS485_UART        uart1
#define RS485_BAUD        115200
#define PIN_RS485_TX      4     /* GP4  -> SP3485 DI                   */
#define PIN_RS485_RX      5     /* GP5  <- SP3485 RO                   */
#define PIN_RS485_DE      3     /* GP3  -> SP3485 DE & /RE             */
#define PIN_RS485_INT     6     /* GP6  -> /INT (open-drain to bus)    */
#define PIN_LED           25    /* onboard LED (Pico, not Pico W WL)   */

/* ------------------------------------------------------------------ */
/* Protocol constants (must match docs/RS485_PERIPHERAL_BUS.md)       */
/* ------------------------------------------------------------------ */
#define RS485_SOF              0xAB
#define RS485_ADDR_BROADCAST   0xFF

#define CMD_PING        0x01
#define CMD_PONG        0x02
#define CMD_SET_OUTPUT  0x10
#define CMD_GET_STATUS  0x11
#define CMD_STATUS      0x12
#define CMD_SET_PARAM   0x20
#define CMD_GET_PARAM   0x21
#define CMD_PARAM_VAL   0x22
#define CMD_STREAM_ON   0x30
#define CMD_STREAM_OFF  0x31
#define CMD_STREAM_DATA 0x32
#define CMD_ERROR       0xF0
#define CMD_SYNC        0xFF

#define FW_VERSION      0x07

#define MAX_FRAME       260     /* SOF + ADDR + CMD + LEN + 255 + CRC  */

/* ------------------------------------------------------------------ */
/* Virtual peripheral state                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  addr;
    bool     enabled;
    bool     stream_on;
    uint16_t stream_interval_ms;
    uint32_t next_stream_at;
    bool     int_pending;       /* will assert /INT on next service    */
    /* Searchlight */
    uint8_t  sl_brightness;     /* 0..255 PWM                          */
    int8_t   sl_temp_c;         /* deg C                               */
    uint8_t  sl_faults;         /* fault flags bitmap                  */
    uint16_t sl_p_thermal_lim;  /* param 0x01 thermal shutoff enable   */
    /* Radar */
    uint16_t rd_distance_mm;
    uint8_t  rd_signal;         /* 0..255                              */
    uint8_t  rd_status;         /* 0=ok                                */
    uint8_t  rd_mode;
    uint8_t  rd_range_max_m;
    uint8_t  rd_sample_rate_hz;
    uint16_t rd_min_threshold_mm;
    /* Pan-tilt */
    uint8_t  pt_pan_deg;
    uint8_t  pt_tilt_deg;
    uint8_t  pt_moving;
    uint16_t pt_slew_dps;
    uint16_t pt_soft_limits;
    /* Light bar */
    uint8_t  lb_brightness;
    uint16_t lb_mode;           /* 0=solid, 1=flash, 2=breathe         */
    uint16_t lb_color_rgb565;
} periph_t;

#define N_PERIPH 4
static periph_t g_dev[N_PERIPH] = {
    { .addr = 0x01, .enabled = true,
      .sl_brightness = 0, .sl_temp_c = 25, .sl_faults = 0 },
    { .addr = 0x02, .enabled = true,
      .rd_distance_mm = 5000, .rd_signal = 200, .rd_status = 0,
      .rd_mode = 1, .rd_range_max_m = 12, .rd_sample_rate_hz = 10 },
    { .addr = 0x03, .enabled = true,
      .pt_pan_deg = 90, .pt_tilt_deg = 45, .pt_moving = 0,
      .pt_slew_dps = 60 },
    { .addr = 0x04, .enabled = true,
      .lb_brightness = 0, .lb_mode = 0, .lb_color_rgb565 = 0xFFFF },
};

static bool g_trace = false;       /* dump every RX/TX frame to USB    */

static periph_t *find_dev(uint8_t addr) {
    for (int i = 0; i < N_PERIPH; i++)
        if (g_dev[i].addr == addr) return &g_dev[i];
    return NULL;
}

/* ------------------------------------------------------------------ */
/* CRC-8/MAXIM (poly 0x31), Dallas/Maxim                              */
/* ------------------------------------------------------------------ */
static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31)
                               : (uint8_t)(crc << 1);
    }
    return crc;
}

/* ------------------------------------------------------------------ */
/* Frame TX                                                           */
/* ------------------------------------------------------------------ */
static void rs485_tx(uint8_t addr, uint8_t cmd,
                     const uint8_t *payload, uint8_t plen)
{
    uint8_t f[MAX_FRAME];
    f[0] = RS485_SOF;
    f[1] = addr;
    f[2] = cmd;
    f[3] = plen;
    if (plen) memcpy(&f[4], payload, plen);
    f[4 + plen] = crc8(&f[1], 3 + plen);

    gpio_put(PIN_RS485_DE, 1);
    /* small turnaround delay so the master's transceiver releases */
    sleep_us(50);
    uart_write_blocking(RS485_UART, f, 5 + plen);
    uart_tx_wait_blocking(RS485_UART);
    /* drop DE quickly so the master can speak again — last bit is already
       on the wire by the time uart_tx_wait_blocking returns */
    sleep_us(5);
    gpio_put(PIN_RS485_DE, 0);

    if (g_trace) {
        printf("[TX] addr=0x%02X cmd=0x%02X len=%u :", addr, cmd, plen);
        for (uint8_t i = 0; i < plen; i++) printf(" %02X", payload[i]);
        printf("  crc=%02X\n", f[4 + plen]);
    }
}

/* ------------------------------------------------------------------ */
/* /INT line — emulate open-drain by toggling pin direction           */
/* ------------------------------------------------------------------ */
static void int_assert(bool assert) {
    if (assert) {
        gpio_set_dir(PIN_RS485_INT, GPIO_OUT);
        gpio_put(PIN_RS485_INT, 0);
    } else {
        gpio_set_dir(PIN_RS485_INT, GPIO_IN); /* high-Z, master pulls up */
    }
}

static bool any_int_pending(void) {
    for (int i = 0; i < N_PERIPH; i++)
        if (g_dev[i].enabled && g_dev[i].int_pending) return true;
    return false;
}

/* ------------------------------------------------------------------ */
/* Per-peripheral response builders                                   */
/* ------------------------------------------------------------------ */
static void send_status(const periph_t *d) {
    uint8_t p[8]; uint8_t n = 0;
    switch (d->addr) {
    case 0x01: /* Searchlight: brightness u8, temp i8, faults u8 */
        p[n++] = d->sl_brightness;
        p[n++] = (uint8_t)d->sl_temp_c;
        p[n++] = d->sl_faults;
        break;
    case 0x02: /* Radar status: mode, range_max, sample_rate */
        p[n++] = d->rd_mode;
        p[n++] = d->rd_range_max_m;
        p[n++] = d->rd_sample_rate_hz;
        break;
    case 0x03: /* Pan-tilt: pan, tilt, moving */
        p[n++] = d->pt_pan_deg;
        p[n++] = d->pt_tilt_deg;
        p[n++] = d->pt_moving;
        break;
    case 0x04: /* Light bar: brightness, mode lo, mode hi (compact) */
        p[n++] = d->lb_brightness;
        p[n++] = (uint8_t)(d->lb_mode & 0xFF);
        p[n++] = (uint8_t)(d->lb_color_rgb565 & 0xFF);
        p[n++] = (uint8_t)(d->lb_color_rgb565 >> 8);
        break;
    }
    rs485_tx(d->addr, CMD_STATUS, p, n);
}

static void send_stream_data(const periph_t *d) {
    uint8_t p[8]; uint8_t n = 0;
    switch (d->addr) {
    case 0x02:
        p[n++] = (uint8_t)(d->rd_distance_mm & 0xFF);
        p[n++] = (uint8_t)(d->rd_distance_mm >> 8);
        p[n++] = d->rd_signal;
        p[n++] = d->rd_status;
        break;
    case 0x04: /* fake power draw estimate */
        {
            uint16_t pw = (uint16_t)d->lb_brightness * 47; /* mA */
            p[n++] = (uint8_t)(pw & 0xFF);
            p[n++] = (uint8_t)(pw >> 8);
        }
        break;
    default:
        send_status(d);
        return;
    }
    rs485_tx(d->addr, CMD_STREAM_DATA, p, n);
}

static void handle_set_output(periph_t *d, const uint8_t *p, uint8_t n) {
    if (n < 2) return;
    uint8_t ch = p[0], v = p[1];
    switch (d->addr) {
    case 0x01: if (ch == 0) d->sl_brightness = v; break;
    case 0x03:
        if (ch == 0) d->pt_pan_deg = v;
        else if (ch == 1) d->pt_tilt_deg = v;
        d->pt_moving = 1;
        break;
    case 0x04: if (ch == 0) d->lb_brightness = v; break;
    }
}

static void handle_set_param(periph_t *d, uint8_t pid, uint16_t v) {
    switch (d->addr) {
    case 0x01: if (pid == 0x01) d->sl_p_thermal_lim = v; break;
    case 0x02: if (pid == 0x01) d->rd_min_threshold_mm = v; break;
    case 0x03:
        if (pid == 0x01) d->pt_slew_dps = v;
        else if (pid == 0x02) d->pt_soft_limits = v;
        break;
    case 0x04:
        if (pid == 0x01) d->lb_mode = v;
        else if (pid == 0x02) d->lb_color_rgb565 = v;
        break;
    }
}

static uint16_t handle_get_param(const periph_t *d, uint8_t pid) {
    switch (d->addr) {
    case 0x01: if (pid == 0x01) return d->sl_p_thermal_lim; break;
    case 0x02: if (pid == 0x01) return d->rd_min_threshold_mm; break;
    case 0x03:
        if (pid == 0x01) return d->pt_slew_dps;
        if (pid == 0x02) return d->pt_soft_limits;
        break;
    case 0x04:
        if (pid == 0x01) return d->lb_mode;
        if (pid == 0x02) return d->lb_color_rgb565;
        break;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Frame dispatch                                                     */
/* ------------------------------------------------------------------ */
static void dispatch(uint8_t addr, uint8_t cmd,
                     const uint8_t *p, uint8_t n)
{
    if (g_trace) {
        printf("[RX] addr=0x%02X cmd=0x%02X len=%u :", addr, cmd, n);
        for (uint8_t i = 0; i < n; i++) printf(" %02X", p[i]);
        printf("\n");
    }

    /* Broadcast — no response */
    if (addr == RS485_ADDR_BROADCAST) {
        if (cmd == CMD_SYNC && g_trace) printf("[BUS] SYNC received\n");
        return;
    }

    periph_t *d = find_dev(addr);
    if (!d || !d->enabled) return;       /* silently ignore */

    switch (cmd) {
    case CMD_PING: {
        uint8_t v = FW_VERSION;
        rs485_tx(addr, CMD_PONG, &v, 1);
        break;
    }
    case CMD_GET_STATUS:
        send_status(d);
        break;
    case CMD_SET_OUTPUT:
        handle_set_output(d, p, n);
        send_status(d);
        break;
    case CMD_SET_PARAM:
        if (n >= 3) {
            uint8_t  pid = p[0];
            uint16_t v   = (uint16_t)p[1] | ((uint16_t)p[2] << 8);
            handle_set_param(d, pid, v);
            uint8_t r[3] = { pid, (uint8_t)(v & 0xFF), (uint8_t)(v >> 8) };
            rs485_tx(addr, CMD_PARAM_VAL, r, 3);
        }
        break;
    case CMD_GET_PARAM:
        if (n >= 1) {
            uint8_t  pid = p[0];
            uint16_t v   = handle_get_param(d, pid);
            uint8_t r[3] = { pid, (uint8_t)(v & 0xFF), (uint8_t)(v >> 8) };
            rs485_tx(addr, CMD_PARAM_VAL, r, 3);
        }
        break;
    case CMD_STREAM_ON:
        if (n >= 2) {
            d->stream_interval_ms = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
            if (d->stream_interval_ms < 10) d->stream_interval_ms = 10;
            d->stream_on = true;
            d->next_stream_at = to_ms_since_boot(get_absolute_time())
                              + d->stream_interval_ms;
        }
        send_status(d);
        break;
    case CMD_STREAM_OFF:
        d->stream_on = false;
        send_status(d);
        break;
    default: {
        uint8_t err = 0x01; /* unknown cmd */
        rs485_tx(addr, CMD_ERROR, &err, 1);
        break;
    }
    }
}

/* ------------------------------------------------------------------ */
/* RS-485 byte-state-machine receiver                                 */
/* ------------------------------------------------------------------ */
static void rs485_rx_poll(void) {
    static enum { S_SOF, S_ADDR, S_CMD, S_LEN, S_PAY, S_CRC } st = S_SOF;
    static uint8_t addr, cmd, len, idx;
    static uint8_t buf[256];
    static uint32_t last_byte_ms = 0;

    /* inter-frame timeout — if mid-frame and silent for >5 ms, reset */
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (st != S_SOF && (now - last_byte_ms) > 5) {
        st = S_SOF;
    }

    while (uart_is_readable(RS485_UART)) {
        uint8_t b = uart_getc(RS485_UART);
        last_byte_ms = now;
        switch (st) {
        case S_SOF:  if (b == RS485_SOF) st = S_ADDR; break;
        case S_ADDR: addr = b; st = S_CMD;            break;
        case S_CMD:  cmd  = b; st = S_LEN;            break;
        case S_LEN:
            len = b; idx = 0;
            st = (len == 0) ? S_CRC : S_PAY;
            break;
        case S_PAY:
            buf[idx++] = b;
            if (idx >= len) st = S_CRC;
            break;
        case S_CRC: {
            /* recompute over ADDR..end-of-payload */
            uint8_t tmp[3 + 255];
            tmp[0] = addr; tmp[1] = cmd; tmp[2] = len;
            if (len) memcpy(&tmp[3], buf, len);
            uint8_t calc = crc8(tmp, 3 + len);
            if (calc == b) {
                dispatch(addr, cmd, buf, len);
            } else if (g_trace) {
                printf("[RX] CRC fail (got %02X want %02X)\n", b, calc);
            }
            st = S_SOF;
            break;
        }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Periodic streaming + /INT servicing                                */
/* ------------------------------------------------------------------ */
static void periodic_tick(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    int_assert(any_int_pending());

    for (int i = 0; i < N_PERIPH; i++) {
        periph_t *d = &g_dev[i];
        if (!d->enabled || !d->stream_on) continue;
        if ((int32_t)(now - d->next_stream_at) >= 0) {
            d->next_stream_at = now + d->stream_interval_ms;
            send_stream_data(d);
        }
    }
}

/* ------------------------------------------------------------------ */
/* USB CDC command-line                                               */
/* ------------------------------------------------------------------ */
static void cli_help(void) {
    printf(
      "\nRS-485 peripheral emulator -- commands:\n"
      "  help                          this text\n"
      "  list                          dump all virtual peripherals\n"
      "  enable <addr>                 enable an addr (hex or dec)\n"
      "  disable <addr>                disable an addr\n"
      "  addr <old> <new>              renumber a peripheral\n"
      "  trace on|off                  log every RS-485 frame\n"
      "  int <addr>                    raise /INT for one peripheral\n"
      "  clrint <addr>                 clear pending /INT\n"
      "  set <addr> <field> <value>    poke an emulated value\n"
      "    Searchlight (0x01): brightness | temp | faults\n"
      "    Radar       (0x02): distance | signal | status |\n"
      "                        mode | range | rate\n"
      "    PanTilt     (0x03): pan | tilt | moving | slew\n"
      "    LightBar    (0x04): brightness | mode | color\n"
      "Examples:\n"
      "  set 0x01 brightness 200\n"
      "  set 0x02 distance 1750\n"
      "  set 0x03 tilt 30\n"
      "  int 0x01\n"
    );
}

static void cli_list(void) {
    static const char *names[] = {
        "Searchlight", "Radar", "PanTilt", "LightBar"
    };
    printf("\n  addr  name         on   stream   key values\n");
    for (int i = 0; i < N_PERIPH; i++) {
        periph_t *d = &g_dev[i];
        printf("  0x%02X  %-11s  %s   %4u ms  ",
               d->addr,
               (i < 4 ? names[i] : "?"),
               d->enabled ? "Y" : "n",
               d->stream_on ? d->stream_interval_ms : 0);
        switch (d->addr) {
        case 0x01:
            printf("bright=%u temp=%dC faults=0x%02X\n",
                   d->sl_brightness, d->sl_temp_c, d->sl_faults);
            break;
        case 0x02:
            printf("dist=%u mm sig=%u stat=%u\n",
                   d->rd_distance_mm, d->rd_signal, d->rd_status);
            break;
        case 0x03:
            printf("pan=%u tilt=%u moving=%u\n",
                   d->pt_pan_deg, d->pt_tilt_deg, d->pt_moving);
            break;
        case 0x04:
            printf("bright=%u mode=%u color=0x%04X\n",
                   d->lb_brightness, d->lb_mode, d->lb_color_rgb565);
            break;
        }
    }
    printf("\n");
}

static long parse_num(const char *s) {
    if (!s) return 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return strtol(s + 2, NULL, 16);
    return strtol(s, NULL, 0);
}

static void cli_set(int argc, char **argv) {
    if (argc < 4) { printf("usage: set <addr> <field> <value>\n"); return; }
    periph_t *d = find_dev((uint8_t)parse_num(argv[1]));
    if (!d) { printf("no such addr\n"); return; }
    const char *f = argv[2];
    long v = parse_num(argv[3]);
    bool ok = true;
    switch (d->addr) {
    case 0x01:
        if      (!strcmp(f, "brightness")) d->sl_brightness = (uint8_t)v;
        else if (!strcmp(f, "temp"))       d->sl_temp_c     = (int8_t)v;
        else if (!strcmp(f, "faults"))     d->sl_faults     = (uint8_t)v;
        else ok = false;
        break;
    case 0x02:
        if      (!strcmp(f, "distance")) d->rd_distance_mm     = (uint16_t)v;
        else if (!strcmp(f, "signal"))   d->rd_signal          = (uint8_t)v;
        else if (!strcmp(f, "status"))   d->rd_status          = (uint8_t)v;
        else if (!strcmp(f, "mode"))     d->rd_mode            = (uint8_t)v;
        else if (!strcmp(f, "range"))    d->rd_range_max_m     = (uint8_t)v;
        else if (!strcmp(f, "rate"))     d->rd_sample_rate_hz  = (uint8_t)v;
        else ok = false;
        break;
    case 0x03:
        if      (!strcmp(f, "pan"))    d->pt_pan_deg  = (uint8_t)v;
        else if (!strcmp(f, "tilt"))   d->pt_tilt_deg = (uint8_t)v;
        else if (!strcmp(f, "moving")) d->pt_moving   = (uint8_t)v;
        else if (!strcmp(f, "slew"))   d->pt_slew_dps = (uint16_t)v;
        else ok = false;
        break;
    case 0x04:
        if      (!strcmp(f, "brightness")) d->lb_brightness    = (uint8_t)v;
        else if (!strcmp(f, "mode"))       d->lb_mode          = (uint16_t)v;
        else if (!strcmp(f, "color"))      d->lb_color_rgb565  = (uint16_t)v;
        else ok = false;
        break;
    }
    printf(ok ? "ok\n" : "unknown field\n");
}

static void cli_dispatch(char *line) {
    char *argv[8]; int argc = 0;
    char *tok = strtok(line, " \t");
    while (tok && argc < 8) { argv[argc++] = tok; tok = strtok(NULL, " \t"); }
    if (!argc) return;
    const char *c = argv[0];

    if      (!strcmp(c, "help"))    cli_help();
    else if (!strcmp(c, "list"))    cli_list();
    else if (!strcmp(c, "enable") && argc >= 2) {
        periph_t *d = find_dev((uint8_t)parse_num(argv[1]));
        if (d) { d->enabled = true; printf("ok\n"); } else printf("no such addr\n");
    }
    else if (!strcmp(c, "disable") && argc >= 2) {
        periph_t *d = find_dev((uint8_t)parse_num(argv[1]));
        if (d) { d->enabled = false; printf("ok\n"); } else printf("no such addr\n");
    }
    else if (!strcmp(c, "addr") && argc >= 3) {
        periph_t *d = find_dev((uint8_t)parse_num(argv[1]));
        if (d) { d->addr = (uint8_t)parse_num(argv[2]); printf("ok\n"); }
        else printf("no such addr\n");
    }
    else if (!strcmp(c, "trace") && argc >= 2) {
        g_trace = !strcmp(argv[1], "on"); printf("trace %s\n", g_trace ? "on" : "off");
    }
    else if (!strcmp(c, "int") && argc >= 2) {
        periph_t *d = find_dev((uint8_t)parse_num(argv[1]));
        if (d) { d->int_pending = true; printf("/INT asserted for 0x%02X\n", d->addr); }
        else printf("no such addr\n");
    }
    else if (!strcmp(c, "clrint") && argc >= 2) {
        periph_t *d = find_dev((uint8_t)parse_num(argv[1]));
        if (d) { d->int_pending = false; printf("ok\n"); }
        else printf("no such addr\n");
    }
    else if (!strcmp(c, "set"))  cli_set(argc, argv);
    else printf("? type 'help'\n");
}

static void cli_poll(void) {
    static char  line[96];
    static size_t len = 0;
    int ch = getchar_timeout_us(0);
    while (ch != PICO_ERROR_TIMEOUT) {
        if (ch == '\r' || ch == '\n') {
            if (len) { putchar('\n'); line[len] = 0; cli_dispatch(line); len = 0; }
            printf("> "); fflush(stdout);
        } else if (ch == 0x08 || ch == 0x7F) {
            if (len) { len--; printf("\b \b"); fflush(stdout); }
        } else if (len < sizeof(line) - 1 && ch >= 0x20 && ch < 0x7F) {
            line[len++] = (char)ch;
            putchar(ch); fflush(stdout);
        }
        ch = getchar_timeout_us(0);
    }
}

/* ------------------------------------------------------------------ */
/* Embedded web UI                                                    */
/* ------------------------------------------------------------------ */
static const char INDEX_HTML[] =
"<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>RS-485 Emulator</title><style>"
"body{font-family:-apple-system,system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:12px}"
"h1{font-size:18px;margin:4px 0 12px}"
".grid{display:grid;gap:12px;grid-template-columns:1fr}"
"@media(min-width:680px){.grid{grid-template-columns:1fr 1fr}}"
".card{background:#1c1c1f;border:1px solid #333;border-radius:10px;padding:12px}"
".card h2{font-size:15px;margin:0 0 8px;display:flex;justify-content:space-between;align-items:center}"
".row{display:flex;align-items:center;gap:8px;margin:6px 0}"
".row label{width:90px;font-size:13px;color:#aaa}"
".row input[type=range]{flex:1}"
".row input[type=number]{width:80px;background:#000;color:#fff;border:1px solid #444;border-radius:4px;padding:3px}"
".row .v{width:64px;text-align:right;font-variant-numeric:tabular-nums}"
"button{background:#2a4;color:#fff;border:none;padding:6px 10px;border-radius:6px;font-size:13px;margin-right:4px}"
"button.warn{background:#a42}button.off{background:#444}"
".badge{font-size:11px;padding:2px 6px;border-radius:10px;background:#444;margin-left:6px}"
".badge.on{background:#2a4}"
".top{display:flex;gap:8px;align-items:center;margin-bottom:10px}"
"</style></head><body>"
"<h1>RS-485 Peripheral Emulator</h1>"
"<div class=top><button id=trace onclick=toggleTrace()>Trace: ?</button>"
"<span class=badge id=conn>connecting&hellip;</span></div>"
"<div class=grid>"
"<div class=card><h2>Searchlight (0x01)<span><span class=badge id=sl-en></span><span class=badge id=sl-intp style=display:none>/INT</span></span></h2>"
"<div class=row><label>Brightness</label><input type=range min=0 max=255 id=sl-brightness-r><span class=v id=sl-brightness-v></span></div>"
"<div class=row><label>Temp &deg;C</label><input type=number min=-40 max=125 id=sl-temp><button onclick=\"setF('sl','temp',v('sl-temp'))\">Set</button></div>"
"<div class=row><label>Faults</label><input type=number min=0 max=255 id=sl-faults><button onclick=\"setF('sl','faults',v('sl-faults'))\">Set</button></div>"
"<div class=row><button onclick=\"cmd('int','sl')\">/INT</button><button class=off onclick=\"cmd('clrint','sl')\">Clear</button><button class=warn id=sl-toggle onclick=toggleEn('sl')>Disable</button></div></div>"

"<div class=card><h2>Radar (0x02)<span><span class=badge id=rd-en></span><span class=badge id=rd-intp style=display:none>/INT</span></span></h2>"
"<div class=row><label>Distance</label><input type=range min=0 max=20000 id=rd-distance-r><span class=v id=rd-distance-v></span></div>"
"<div class=row><label>Signal</label><input type=range min=0 max=255 id=rd-signal-r><span class=v id=rd-signal-v></span></div>"
"<div class=row><label>Status</label><input type=number min=0 max=255 id=rd-status><button onclick=\"setF('rd','status',v('rd-status'))\">Set</button></div>"
"<div class=row><label>Mode</label><input type=number min=0 max=255 id=rd-mode><button onclick=\"setF('rd','mode',v('rd-mode'))\">Set</button></div>"
"<div class=row><label>Range m</label><input type=number min=0 max=255 id=rd-range><button onclick=\"setF('rd','range',v('rd-range'))\">Set</button></div>"
"<div class=row><label>Rate Hz</label><input type=number min=0 max=255 id=rd-rate><button onclick=\"setF('rd','rate',v('rd-rate'))\">Set</button></div>"
"<div class=row><button onclick=\"cmd('int','rd')\">/INT</button><button class=off onclick=\"cmd('clrint','rd')\">Clear</button><button class=warn id=rd-toggle onclick=toggleEn('rd')>Disable</button></div></div>"

"<div class=card><h2>Pan-Tilt (0x03)<span><span class=badge id=pt-en></span><span class=badge id=pt-intp style=display:none>/INT</span></span></h2>"
"<div class=row><label>Pan</label><input type=range min=0 max=180 id=pt-pan-r><span class=v id=pt-pan-v></span></div>"
"<div class=row><label>Tilt</label><input type=range min=0 max=90 id=pt-tilt-r><span class=v id=pt-tilt-v></span></div>"
"<div class=row><label>Moving</label><input type=number min=0 max=1 id=pt-moving><button onclick=\"setF('pt','moving',v('pt-moving'))\">Set</button></div>"
"<div class=row><label>Slew dps</label><input type=number min=0 max=65535 id=pt-slew><button onclick=\"setF('pt','slew',v('pt-slew'))\">Set</button></div>"
"<div class=row><button onclick=\"cmd('int','pt')\">/INT</button><button class=off onclick=\"cmd('clrint','pt')\">Clear</button><button class=warn id=pt-toggle onclick=toggleEn('pt')>Disable</button></div></div>"

"<div class=card><h2>Light Bar (0x04)<span><span class=badge id=lb-en></span><span class=badge id=lb-intp style=display:none>/INT</span></span></h2>"
"<div class=row><label>Brightness</label><input type=range min=0 max=255 id=lb-brightness-r><span class=v id=lb-brightness-v></span></div>"
"<div class=row><label>Mode</label><input type=number min=0 max=65535 id=lb-mode><button onclick=\"setF('lb','mode',v('lb-mode'))\">Set</button></div>"
"<div class=row><label>Color</label><input type=color id=lb-color-c><input type=number min=0 max=65535 id=lb-color><button onclick=\"setF('lb','color',v('lb-color'))\">Set</button></div>"
"<div class=row><button onclick=\"cmd('int','lb')\">/INT</button><button class=off onclick=\"cmd('clrint','lb')\">Clear</button><button class=warn id=lb-toggle onclick=toggleEn('lb')>Disable</button></div></div>"
"</div>"

"<script>"
"const A={sl:1,rd:2,pt:3,lb:4},R={sl:['brightness'],rd:['distance','signal'],pt:['pan','tilt'],lb:['brightness']};"
"let L={};const $=id=>document.getElementById(id);const v=id=>$(id).value;"
"const api=(p,q)=>fetch('/api/'+p+(q?'?'+q:'')).then(r=>r.json()).catch(()=>({ok:false}));"
"const setF=(k,f,x)=>api('set','addr='+A[k]+'&field='+f+'&value='+x).then(refresh);"
"const cmd=(o,k)=>api(o,'addr='+A[k]).then(refresh);"
"const toggleEn=k=>api(L[k]&&L[k].on?'disable':'enable','addr='+A[k]).then(refresh);"
"const toggleTrace=()=>api('trace','on='+(L.trace?0:1)).then(refresh);"
"function bindR(k,f){const r=$(k+'-'+f+'-r'),vv=$(k+'-'+f+'-v');if(!r||r._b)return;r._b=1;"
"r.addEventListener('input',()=>vv.textContent=r.value);"
"r.addEventListener('change',()=>setF(k,f,r.value));}"
"const c=$('lb-color-c');if(c)c.addEventListener('change',()=>{const h=c.value;"
"const r=parseInt(h.substr(1,2),16),g=parseInt(h.substr(3,2),16),b=parseInt(h.substr(5,2),16);"
"const rgb565=((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3);$('lb-color').value=rgb565;setF('lb','color',rgb565);});"
"async function refresh(){const s=await api('state');"
"$('conn').textContent=s.sl?'connected':'no data';$('conn').className='badge'+(s.sl?' on':'');"
"$('trace').textContent='Trace: '+(s.trace?'ON':'off');L=s;"
"for(const k of ['sl','rd','pt','lb']){const d=s[k];if(!d)continue;"
"$(k+'-en').textContent=d.on?'on':'off';$(k+'-en').className='badge'+(d.on?' on':'');"
"$(k+'-intp').style.display=d.intp?'inline-block':'none';"
"$(k+'-toggle').textContent=d.on?'Disable':'Enable';"
"for(const f of R[k]){bindR(k,f);const r=$(k+'-'+f+'-r'),vv=$(k+'-'+f+'-v');"
"if(document.activeElement!==r){r.value=d[f];vv.textContent=d[f];}}}"
"const fill=(id,x)=>{const e=$(id);if(e&&document.activeElement!==e)e.value=x;};"
"fill('sl-temp',s.sl.temp);fill('sl-faults',s.sl.faults);"
"fill('rd-status',s.rd.status);fill('rd-mode',s.rd.mode);fill('rd-range',s.rd.range);fill('rd-rate',s.rd.rate);"
"fill('pt-moving',s.pt.moving);fill('pt-slew',s.pt.slew);"
"fill('lb-mode',s.lb.mode);fill('lb-color',s.lb.color);}"
"setInterval(refresh,1000);refresh();"
"</script></body></html>";

/* ------------------------------------------------------------------ */
/* Web URL handler — invoked by web.c                                 */
/* ------------------------------------------------------------------ */
static int qparam(const char *q, const char *key, char *out, size_t cap) {
    out[0] = 0;
    if (!q || !*q) return 0;
    size_t klen = strlen(key);
    while (*q) {
        const char *amp = strchr(q, '&');
        size_t span = amp ? (size_t)(amp - q) : strlen(q);
        if (span > klen && q[klen] == '=' && !memcmp(q, key, klen)) {
            size_t vlen = span - klen - 1;
            if (vlen >= cap) vlen = cap - 1;
            memcpy(out, q + klen + 1, vlen);
            out[vlen] = 0;
            /* basic url-decode for + and %xx */
            char *r = out, *w = out;
            while (*r) {
                if (*r == '+') { *w++ = ' '; r++; }
                else if (*r == '%' && r[1] && r[2]) {
                    char hx[3] = { r[1], r[2], 0 };
                    *w++ = (char)strtol(hx, NULL, 16);
                    r += 3;
                } else *w++ = *r++;
            }
            *w = 0;
            return 1;
        }
        if (!amp) break;
        q = amp + 1;
    }
    return 0;
}

size_t app_http(const char *path, const char *query,
                char *out, size_t cap,
                const char **content_type, int *status)
{
    *status = 200;

    if (!strcmp(path, "/")) {
        *content_type = "text/html; charset=utf-8";
        size_t n = sizeof(INDEX_HTML) - 1;
        if (n > cap) n = cap;
        memcpy(out, INDEX_HTML, n);
        return n;
    }

    if (!strcmp(path, "/api/state")) {
        *content_type = "application/json";
        periph_t *s = &g_dev[0], *r = &g_dev[1],
                 *t = &g_dev[2], *l = &g_dev[3];
        int n = snprintf(out, cap,
            "{\"trace\":%s,"
            "\"sl\":{\"addr\":1,\"on\":%s,\"stream\":%u,\"brightness\":%u,\"temp\":%d,\"faults\":%u,\"intp\":%s},"
            "\"rd\":{\"addr\":2,\"on\":%s,\"stream\":%u,\"distance\":%u,\"signal\":%u,\"status\":%u,\"mode\":%u,\"range\":%u,\"rate\":%u,\"intp\":%s},"
            "\"pt\":{\"addr\":3,\"on\":%s,\"stream\":%u,\"pan\":%u,\"tilt\":%u,\"moving\":%u,\"slew\":%u,\"intp\":%s},"
            "\"lb\":{\"addr\":4,\"on\":%s,\"stream\":%u,\"brightness\":%u,\"mode\":%u,\"color\":%u,\"intp\":%s}}",
            g_trace ? "true" : "false",
            s->enabled?"true":"false", s->stream_on ? s->stream_interval_ms : 0,
            s->sl_brightness, (int)s->sl_temp_c, s->sl_faults,
            s->int_pending?"true":"false",
            r->enabled?"true":"false", r->stream_on ? r->stream_interval_ms : 0,
            r->rd_distance_mm, r->rd_signal, r->rd_status,
            r->rd_mode, r->rd_range_max_m, r->rd_sample_rate_hz,
            r->int_pending?"true":"false",
            t->enabled?"true":"false", t->stream_on ? t->stream_interval_ms : 0,
            t->pt_pan_deg, t->pt_tilt_deg, t->pt_moving, t->pt_slew_dps,
            t->int_pending?"true":"false",
            l->enabled?"true":"false", l->stream_on ? l->stream_interval_ms : 0,
            l->lb_brightness, l->lb_mode, l->lb_color_rgb565,
            l->int_pending?"true":"false");
        return (n < 0) ? 0 : (size_t)n;
    }

    if (!strncmp(path, "/api/", 5)) {
        char addrs[16], field[16], value[16], onarg[8];
        qparam(query, "addr",  addrs, sizeof(addrs));
        qparam(query, "field", field, sizeof(field));
        qparam(query, "value", value, sizeof(value));
        qparam(query, "on",    onarg, sizeof(onarg));
        periph_t *d = find_dev((uint8_t)parse_num(addrs));
        bool ok = true;

        if (!strcmp(path, "/api/set")) {
            if (!d) ok = false;
            else { char *argv[4] = { "set", addrs, field, value };
                   cli_set(4, argv); }
        } else if (!strcmp(path, "/api/enable"))  { if (d) d->enabled = true;  else ok = false; }
        else  if (!strcmp(path, "/api/disable")) { if (d) d->enabled = false; else ok = false; }
        else  if (!strcmp(path, "/api/int"))     { if (d) d->int_pending = true;  else ok = false; }
        else  if (!strcmp(path, "/api/clrint"))  { if (d) d->int_pending = false; else ok = false; }
        else  if (!strcmp(path, "/api/trace"))   { g_trace = (!strcmp(onarg,"1") || !strcmp(onarg,"true")); }
        else  ok = false;

        *content_type = "application/json";
        if (!ok) *status = 400;
        return (size_t)snprintf(out, cap, "{\"ok\":%s}", ok ? "true" : "false");
    }

    /* Captive portal: redirect anything unknown to the UI. */
    *status = 302;
    return (size_t)snprintf(out, cap, "http://192.168.4.1/");
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */
int main(void) {
    stdio_init_all();

    /* RS-485 UART */
    uart_init(RS485_UART, RS485_BAUD);
    uart_set_format(RS485_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(RS485_UART, true);
    gpio_set_function(PIN_RS485_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_RS485_RX, GPIO_FUNC_UART);

    /* DE/RE — start in receive */
    gpio_init(PIN_RS485_DE);
    gpio_set_dir(PIN_RS485_DE, GPIO_OUT);
    gpio_put(PIN_RS485_DE, 0);

    /* /INT — high-Z by default (open-drain emulation) */
    gpio_init(PIN_RS485_INT);
    gpio_set_dir(PIN_RS485_INT, GPIO_IN);
    gpio_put(PIN_RS485_INT, 0);   /* level when we drive it low */

    /* status LED */
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);

    sleep_ms(500);
    printf("\nRS-485 peripheral emulator ready. Type 'help'.\n> ");
    fflush(stdout);

    /* Bring up Wi-Fi AP + captive portal + HTTP server. */
    if (!web_init(WEB_AP_SSID, WEB_AP_PASSWORD)) {
        printf("[web] init failed -- continuing without AP\n");
    }

    uint32_t last_blink = 0;
    while (true) {
        rs485_rx_poll();
        periodic_tick();
        cli_poll();
        web_poll();

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_blink > 500) {
            last_blink = now;
            gpio_xor_mask(1u << PIN_LED);
        }
    }
}
