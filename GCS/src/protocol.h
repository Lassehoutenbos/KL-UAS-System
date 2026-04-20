#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* ------------------------------------------------------------------ */
/* Packet framing                                                        */
/* ------------------------------------------------------------------ */
#define PROTO_SOF               0xAA

#define PROTO_TYPE_ADC          0x01  /* Pico→Pi: ADC channel data         */
#define PROTO_TYPE_DIGITAL      0x02  /* Pico→Pi: digital I/O state        */
#define PROTO_TYPE_LED          0x03  /* Pi→Pico: LED command               */
#define PROTO_TYPE_SCREEN       0x04  /* Pi→Pico: screen mode command       */
#define PROTO_TYPE_HEARTBEAT    0x05  /* bidirectional: alive signal        */
#define PROTO_TYPE_EVENT        0x06  /* Pico→Pi: input event               */
#define PROTO_TYPE_ERROR        0x07  /* Pico→Pi: error status              */
#define PROTO_TYPE_BRIGHTNESS   0x08  /* Pi→Pico: global brightness         */
#define PROTO_TYPE_MODE         0x09  /* Pi→Pico: state machine override    */
#define PROTO_TYPE_WARNING      0x0A  /* Pi→Pico: warning panel severity    */
#define PROTO_TYPE_ALS          0x0B  /* Pico→Pi: ambient light sensor data */
#define PROTO_TYPE_PERIPH_CMD   0x0C  /* Pi→Pico:  command for a bus peripheral  */
#define PROTO_TYPE_PERIPH_DATA  0x0D  /* Pico→Pi:  data from a bus peripheral    */
#define PROTO_TYPE_PERIPH_STATE 0x0E  /* Pico→Pi:  peripheral online/offline     */
#define PROTO_TYPE_PERIPH_SCREEN 0x0F /* Pi→Pico:  select peripheral detail screen */
#define PROTO_TYPE_WORKLIGHT     0x10 /* Pi→Pico:  worklight on/off + colour        */

/* Warning severity levels — type 0x0A */
#define WARN_OK                 0
#define WARN_WARNING            1
#define WARN_CRITICAL           2

/* Warning icon indices (LED = WARN_PANEL_LED_BASE + index) */
#define WARN_ICON_TEMP          0   /* temperature warning     */
#define WARN_ICON_SIGNAL        1   /* signal strength warning */
#define WARN_ICON_AIRCRAFT      2   /* aircraft warning        */
#define WARN_ICON_DRONE_LINK    3   /* drone link status       */
#define WARN_ICON_MAIN          4   /* main big warning        */
#define WARN_ICON_GPS_GCS       5   /* GPS lock GCS            */
#define WARN_ICON_NETWORK_GCS   6   /* network connection GCS  */
#define WARN_ICON_LOCKED        7   /* locked state            */
#define WARN_ICON_DRONE_STATUS  8   /* drone status            */
#define WARN_ICON_COUNT         9
/* MAIN occupies two physical LEDs; total panel LED span = WARN_ICON_COUNT + 1 */
#define WARN_PANEL_LED_COUNT    (WARN_ICON_COUNT + 1)
#define WARN_PANEL_LED_BASE     0   /* first LED index of warning panel — change to relocate */

/* LED animation modes — type 0x03, chain=0x01 (WS2811 RGB buttons) */
#define LED_ANIM_OFF            0
#define LED_ANIM_ON             1
#define LED_ANIM_BLINK_SLOW     2   /* 500 ms period */
#define LED_ANIM_BLINK_FAST     3   /* 100 ms period */
#define LED_ANIM_PULSE          4   /* sine-wave 0-100% brightness */

/* Event IDs — type 0x06 */
#define EVT_SWITCH_CHANGED      0x01  /* value = port_a<<8 | port_b */
#define EVT_BUTTON_PRESSED      0x02  /* value = button_id */
#define EVT_BUTTON_RELEASED     0x03  /* value = button_id */
#define EVT_KEY_LOCK_CHANGED    0x04  /* value = 0=locked, 1=unlocked */
#define EVT_SOURCE_DETECTED     0x05  /* value = source bitmask */
#define EVT_USB_CONNECTED       0x06
#define EVT_USB_DISCONNECTED    0x07

/* Error codes — type 0x07 */
#define ERR_WATCHDOG_RESET      0x01
#define ERR_STACK_OVERFLOW      0x02
#define ERR_MALLOC_FAILED       0x03

/* SK6812 chain: 2-byte header + SK6812_MAX_PIXELS(128) * 4 bytes GRBW = 514 */
#define PROTO_MAX_PAYLOAD   514
/* Frame: SOF(1)+TYPE(1)+LEN_LO(1)+LEN_HI(1)+PAYLOAD+CKSUM(1) */
#define PROTO_MAX_PACKET    (5 + PROTO_MAX_PAYLOAD)

/* ------------------------------------------------------------------ */
/* Packet payload structures                                            */
/* ------------------------------------------------------------------ */

/* Type 0x01 — ADC data (14 bytes) */
typedef struct __attribute__((packed)) {
    uint16_t ch[6];       /* raw 12-bit ADC counts for CH0-CH5 */
    uint16_t ts_ms;       /* FreeRTOS tick count in ms */
} adc_packet_t;

/* Type 0x02 — Digital I/O state (4 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  port_a;      /* MCP23017 GPIOA state */
    uint8_t  port_b;      /* MCP23017 GPIOB state */
    uint16_t ts_ms;
} digital_packet_t;

/* Type 0x03 — LED command header */
typedef struct __attribute__((packed)) {
    uint8_t chain;        /* 0x00=SK6812, 0x01=WS2811 buttons, 0x02=MCP23017 LEDs */
    uint8_t num_pixels;   /* chain=0x00: pixel count; chain=0x01/0x02: unused */
} led_cmd_header_t;

/* Type 0x04 — Screen mode */
typedef struct __attribute__((packed)) {
    uint8_t mode;   /* 0=auto, 1=main, 2=warning, 3=lock, 4=bat_warning */
} screen_cmd_t;

/* Type 0x05 — Heartbeat */
typedef struct __attribute__((packed)) {
    uint8_t seq;
} heartbeat_pkt_t;

/* Type 0x06 — Input event */
typedef struct __attribute__((packed)) {
    uint8_t  event_id;
    uint16_t value;
} event_pkt_t;

/* Type 0x07 — Error */
typedef struct __attribute__((packed)) {
    uint8_t error_code;
} error_pkt_t;

/* Type 0x08 — Brightness */
#define BRIGHTNESS_TGT_SK6812   0   /* SK6812 LED strip        */
#define BRIGHTNESS_TGT_WS2811   1   /* WS2811 RGB buttons      */
#define BRIGHTNESS_TGT_TFT_BLK  2   /* ST7735 TFT backlight    */

typedef struct __attribute__((packed)) {
    uint8_t target;   /* BRIGHTNESS_TGT_* */
    uint8_t level;    /* 0-255 */
} brightness_cmd_t;

/* Type 0x09 — Mode/state override */
typedef struct __attribute__((packed)) {
    uint8_t state;
} mode_cmd_t;

/* Type 0x0A — Warning panel severity */
typedef struct __attribute__((packed)) {
    uint8_t severity[WARN_ICON_COUNT];  /* one WARN_* value per icon, index = WARN_ICON_* */
} warning_cmd_t;

/* Type 0x0C — Peripheral command (Pi → Pico → RS-485 bus) */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* target peripheral address (0x01–0xFE) */
    uint8_t cmd;        /* RS-485 bus CMD byte                   */
    uint8_t len;        /* payload byte count (0–255)            */
    uint8_t payload[];  /* flexible array — len bytes            */
} periph_cmd_t;

/* Type 0x0D — Peripheral data (RS-485 bus → Pico → Pi) */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* source peripheral address             */
    uint8_t cmd;        /* RS-485 CMD byte of the response       */
    uint8_t len;        /* payload byte count                    */
    uint8_t payload[];  /* response payload — len bytes          */
} periph_data_t;

/* Type 0x0E — Peripheral presence notification */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* peripheral address                    */
    uint8_t online;     /* 1 = online, 0 = offline               */
} periph_state_t;

/* Type 0x0F — Peripheral screen select (Pi → Pico) */
typedef struct __attribute__((packed)) {
    uint8_t addr;       /* peripheral address to show on detail screen */
} periph_screen_cmd_t;

/* Type 0x10 — Worklight command (Pi → Pico) */
typedef struct __attribute__((packed)) {
    uint8_t on;   /* 0 = off, 1 = on */
    uint8_t r;
    uint8_t g;
    uint8_t b;
} worklight_cmd_t;

/* Type 0x0B — Ambient light sensor data (VEML7700) */
typedef struct __attribute__((packed)) {
    uint16_t als_raw;   /* raw ALS register count (16-bit) */
    uint16_t white_raw; /* raw WHITE register count (16-bit) */
    uint32_t lux_milli; /* lux × 1000 (i.e. millilux), avoids float on wire */
    uint16_t ts_ms;     /* FreeRTOS tick count in ms */
} als_packet_t;

/* ------------------------------------------------------------------ */
/* TX queue                                                              */
/* ------------------------------------------------------------------ */
#define TX_BUF_SIZE     64
#define TX_QUEUE_DEPTH  16

typedef struct {
    uint8_t buf[TX_BUF_SIZE];
    uint8_t len;
} tx_item_t;

extern QueueHandle_t g_tx_queue;

/* ------------------------------------------------------------------ */
/* API                                                                   */
/* ------------------------------------------------------------------ */
int  proto_serialize(uint8_t *buf, int buf_size,
                     uint8_t type, const uint8_t *payload, uint16_t payload_len);
int  proto_parse(const uint8_t *buf, uint16_t buf_len,
                 uint8_t *type_out, uint8_t *payload_out);
void proto_handle_rx(const uint8_t *data, uint32_t len);
void proto_set_led_task_handles(TaskHandle_t sk6812_handle,
                                TaskHandle_t ws2811_handle);
void proto_set_screen_task_handle(TaskHandle_t screen_handle);

/** Enqueue a Pico→Pi event packet on g_tx_queue (safe from any task). */
void proto_send_event(uint8_t event_id, uint16_t value);

/** Enqueue a Pico→Pi error packet on g_tx_queue. */
void proto_send_error(uint8_t error_code);

#endif /* PROTOCOL_H */
