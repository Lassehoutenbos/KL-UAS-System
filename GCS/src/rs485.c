#include "rs485.h"
#include "pins.h"
#include "protocol.h"
#include "screen_display.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal command queue (CDC → RS-485 task)                           */
/* ------------------------------------------------------------------ */
#define RS485_CMD_QUEUE_DEPTH  8
#define RS485_CMD_BUF_SIZE     64

typedef struct {
    uint8_t buf[RS485_CMD_BUF_SIZE];
    uint8_t len;
} rs485_cmd_item_t;

static QueueHandle_t s_cmd_queue;

/* ------------------------------------------------------------------ */
/* Peripheral registry                                                   */
/* ------------------------------------------------------------------ */
static uint8_t s_known_addrs[RS485_MAX_PERIPHERALS] = {
    0x01, 0x02, 0x03, 0x04,
    0x00, 0x00, 0x00, 0x00
};
static bool s_online[RS485_MAX_PERIPHERALS];

/* ------------------------------------------------------------------ */
/* CRC-8/MAXIM (polynomial 0x31, init 0x00)                             */
/* ------------------------------------------------------------------ */
static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

/* ------------------------------------------------------------------ */
/* Transmit one RS-485 frame                                             */
/* ------------------------------------------------------------------ */
static void rs485_send(uint8_t addr, uint8_t cmd,
                       const uint8_t *payload, uint8_t plen)
{
    uint8_t frame[4 + 255 + 1];
    frame[0] = RS485_SOF;
    frame[1] = addr;
    frame[2] = cmd;
    frame[3] = plen;
    if (plen && payload) memcpy(&frame[4], payload, plen);
    frame[4 + plen] = crc8(&frame[1], 3 + plen);

    gpio_put(PIN_RS485_DE, 1);
    uart_write_blocking(RS485_UART_INST, frame, 5 + plen);
    uart_tx_wait_blocking(RS485_UART_INST);
    gpio_put(PIN_RS485_DE, 0);
}

/* ------------------------------------------------------------------ */
/* Receive one RS-485 frame (blocking with timeout)                      */
/* Returns payload length on success, -1 on timeout or CRC error.       */
/* ------------------------------------------------------------------ */
static int rs485_recv(uint8_t *addr_out, uint8_t *cmd_out,
                      uint8_t *buf, uint8_t buf_size)
{
    typedef enum { S_SOF, S_ADDR, S_CMD, S_LEN, S_PAYLOAD, S_CRC } state_t;
    state_t  state = S_SOF;
    uint8_t  addr = 0, cmd = 0, plen = 0, idx = 0;
    TickType_t t0 = xTaskGetTickCount();

    while ((xTaskGetTickCount() - t0) < pdMS_TO_TICKS(RS485_TIMEOUT_MS)) {
        if (!uart_is_readable(RS485_UART_INST)) {
            taskYIELD();
            continue;
        }
        uint8_t b = (uint8_t)uart_getc(RS485_UART_INST);

        switch (state) {
            case S_SOF:
                if (b == RS485_SOF) state = S_ADDR;
                break;
            case S_ADDR:
                addr  = b;
                state = S_CMD;
                break;
            case S_CMD:
                cmd   = b;
                state = S_LEN;
                break;
            case S_LEN:
                plen  = b;
                if (plen > buf_size) return -1;   /* won't fit */
                idx   = 0;
                state = (plen == 0) ? S_CRC : S_PAYLOAD;
                break;
            case S_PAYLOAD:
                buf[idx++] = b;
                if (idx >= plen) state = S_CRC;
                break;
            case S_CRC: {
                /* CRC over addr, cmd, len, payload */
                uint8_t tmp[3 + 255];
                tmp[0] = addr; tmp[1] = cmd; tmp[2] = plen;
                if (plen) memcpy(&tmp[3], buf, plen);
                if (b == crc8(tmp, 3 + plen)) {
                    *addr_out = addr;
                    *cmd_out  = cmd;
                    return (int)plen;
                }
                return -1;  /* CRC mismatch */
            }
        }
    }
    return -1;  /* timeout */
}

/* ------------------------------------------------------------------ */
/* Report peripheral state change to Pi over CDC                         */
/* ------------------------------------------------------------------ */
static void notify_state(uint8_t addr, bool online)
{
    periph_state_t pkt = { .addr = addr, .online = online ? 1u : 0u };
    tx_item_t item;
    int len = proto_serialize(item.buf, sizeof(item.buf),
                              PROTO_TYPE_PERIPH_STATE,
                              (const uint8_t *)&pkt, sizeof(pkt));
    if (len > 0) {
        item.len = (uint8_t)len;
        xQueueSend(g_tx_queue, &item, 0);
    }
}

/* ------------------------------------------------------------------ */
/* Forward RS-485 response to Pi over CDC                                */
/* ------------------------------------------------------------------ */
static void forward_to_pi(uint8_t addr, uint8_t cmd,
                           const uint8_t *payload, uint8_t plen)
{
    /* Payload on wire: [addr, cmd, data...] */
    uint8_t cdc_payload[2 + 255];
    cdc_payload[0] = addr;
    cdc_payload[1] = cmd;
    if (plen) memcpy(&cdc_payload[2], payload, plen);

    tx_item_t item;
    int len = proto_serialize(item.buf, sizeof(item.buf),
                              PROTO_TYPE_PERIPH_DATA,
                              cdc_payload, 2 + plen);
    if (len > 0) {
        item.len = (uint8_t)len;
        xQueueSend(g_tx_queue, &item, 0);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                             */
/* ------------------------------------------------------------------ */

void rs485_init(void)
{
    s_cmd_queue = xQueueCreate(RS485_CMD_QUEUE_DEPTH, sizeof(rs485_cmd_item_t));

    uart_init(RS485_UART_INST, RS485_BAUD);
    gpio_set_function(PIN_RS485_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_RS485_RX, GPIO_FUNC_UART);

    gpio_init(PIN_RS485_DE);
    gpio_set_dir(PIN_RS485_DE, GPIO_OUT);
    gpio_put(PIN_RS485_DE, 0);   /* receive mode by default */

    gpio_init(PIN_RS485_INT);
    gpio_set_dir(PIN_RS485_INT, GPIO_IN);
    gpio_pull_up(PIN_RS485_INT); /* open-drain line — pulled high at rest */
}

void rs485_forward_cmd(const uint8_t *payload, uint16_t len)
{
    if (!s_cmd_queue || len > RS485_CMD_BUF_SIZE) return;
    rs485_cmd_item_t item;
    memcpy(item.buf, payload, len);
    item.len = (uint8_t)len;
    xQueueSend(s_cmd_queue, &item, 0);
}

uint8_t rs485_get_peripherals(uint8_t *addrs_out, bool *online_out,
                               uint8_t max_entries)
{
    uint8_t n = 0;
    for (int i = 0; i < RS485_MAX_PERIPHERALS && n < max_entries; i++) {
        if (!s_known_addrs[i]) continue;
        addrs_out[n]  = s_known_addrs[i];
        online_out[n] = s_online[i];
        n++;
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* RS-485 task                                                            */
/* ------------------------------------------------------------------ */
void rs485_task(void *arg)
{
    (void)arg;
    TickType_t last_ping = xTaskGetTickCount();
    uint8_t    resp_buf[255];
    uint8_t    resp_addr, resp_cmd;

    for (;;) {
        /* 1. Forward any commands queued from the Pi */
        rs485_cmd_item_t cmd;
        while (xQueueReceive(s_cmd_queue, &cmd, 0) == pdTRUE) {
            if (cmd.len >= 3) {
                rs485_send(cmd.buf[0], cmd.buf[1],
                           &cmd.buf[3], cmd.buf[2]);
                int r = rs485_recv(&resp_addr, &resp_cmd,
                                   resp_buf, sizeof(resp_buf));
                if (r >= 0) {
                    forward_to_pi(resp_addr, resp_cmd, resp_buf, (uint8_t)r);
                    screen_periph_update_data(resp_addr, resp_cmd,
                                              resp_buf, (uint8_t)r);
                }
            }
        }

        /* 2. Check /INT line — poll any peripheral asserting an async event */
        if (!gpio_get(PIN_RS485_INT)) {
            for (int i = 0; i < RS485_MAX_PERIPHERALS; i++) {
                if (!s_known_addrs[i] || !s_online[i]) continue;
                rs485_send(s_known_addrs[i], RS485_CMD_GET_STATUS, NULL, 0);
                int r = rs485_recv(&resp_addr, &resp_cmd,
                                   resp_buf, sizeof(resp_buf));
                if (r >= 0) {
                    forward_to_pi(resp_addr, resp_cmd, resp_buf, (uint8_t)r);
                    screen_periph_update_data(resp_addr, resp_cmd,
                                              resp_buf, (uint8_t)r);
                }
                /* inter-frame gap so the slave fully re-arms RX */
                vTaskDelay(pdMS_TO_TICKS(2));
            }
        }

        /* 3. Periodic PING sweep */
        if ((xTaskGetTickCount() - last_ping) >=
                pdMS_TO_TICKS(RS485_PING_INTERVAL_MS)) {
            last_ping = xTaskGetTickCount();
            for (int i = 0; i < RS485_MAX_PERIPHERALS; i++) {
                if (!s_known_addrs[i]) continue;
                rs485_send(s_known_addrs[i], RS485_CMD_PING, NULL, 0);
                int r = rs485_recv(&resp_addr, &resp_cmd,
                                   resp_buf, sizeof(resp_buf));
                bool now_online = (r >= 0 && resp_cmd == RS485_CMD_PONG);
                if (now_online != s_online[i]) {
                    s_online[i] = now_online;
                    notify_state(s_known_addrs[i], now_online);
                }
                /* inter-frame gap so the slave fully re-arms RX */
                vTaskDelay(pdMS_TO_TICKS(2));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
