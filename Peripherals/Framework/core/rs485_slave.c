#include "rs485_slave.h"
#include "crc8.h"
#include "../hal/hal.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal state                                                       */
/* ------------------------------------------------------------------ */

typedef enum {
    S_SOF = 0, S_ADDR, S_CMD, S_LEN, S_PAYLOAD, S_CRC
} rx_state_t;

static const rs485_slave_cfg_t *s_cfg;

/* RX FSM */
static rx_state_t  s_state;
static uint8_t     s_rx_addr, s_rx_cmd, s_rx_plen, s_rx_idx;
static uint8_t     s_rx_buf[RS485_MAX_PAYLOAD];
static uint32_t    s_rx_t0;            /* hal_millis() when SOF arrived */
static uint32_t    s_last_byte_ms;     /* hal_millis() of last byte seen on bus */

/* Streaming */
static uint16_t    s_stream_interval;  /* 0 = disabled */
static uint32_t    s_stream_last_ms;

/* ------------------------------------------------------------------ */
/* Frame TX                                                             */
/* ------------------------------------------------------------------ */

static void send_frame(uint8_t addr, uint8_t cmd,
                       const uint8_t *payload, uint8_t plen)
{
    uint8_t frame[RS485_MAX_FRAME];
    frame[0] = RS485_SOF;
    frame[1] = addr;
    frame[2] = cmd;
    frame[3] = plen;
    if (plen && payload) memcpy(&frame[4], payload, plen);
    frame[4 + plen] = crc8(&frame[1], 3 + plen);

    hal_uart_set_tx_enable(true);
    hal_uart_write(frame, 5 + plen);
    hal_uart_set_tx_enable(false);
}

/* ------------------------------------------------------------------ */
/* Dispatch                                                             */
/* ------------------------------------------------------------------ */

static const rs485_handler_t *find_handler(uint8_t cmd)
{
    if (!s_cfg || !s_cfg->handlers) return NULL;
    for (const rs485_handler_t *h = s_cfg->handlers; h->cmd != 0; h++) {
        if (h->cmd == cmd) return h;
    }
    return NULL;
}

static void dispatch(uint8_t addr, uint8_t cmd,
                     const uint8_t *payload, uint8_t plen)
{
    const bool broadcast = (addr == RS485_ADDR_BROADCAST);

    /* Built-in: PING -> PONG with [fw_version]. Skip on broadcast. */
    if (cmd == RS485_CMD_PING) {
        if (!broadcast) {
            uint8_t fw = s_cfg->fw_version;
            send_frame(s_cfg->addr, RS485_CMD_PONG, &fw, 1);
        }
        return;
    }

    /* Built-in: STREAM_ON / STREAM_OFF */
    if (cmd == RS485_CMD_STREAM_ON) {
        if (s_cfg->build_stream && plen >= 2) {
            s_stream_interval = (uint16_t)(payload[0] | (payload[1] << 8));
            s_stream_last_ms  = hal_millis();
        }
        return;
    }
    if (cmd == RS485_CMD_STREAM_OFF) {
        s_stream_interval = 0;
        return;
    }

    /* App handlers */
    const rs485_handler_t *h = find_handler(cmd);
    if (!h) return;   /* silently ignore unknown CMDs — keeps the bus quiet */

    if (broadcast) {
        h->handler(payload, plen, NULL, 0);
        return;
    }

    uint8_t resp[RS485_MAX_PAYLOAD];
    int rlen = h->handler(payload, plen, resp, sizeof(resp));
    if (rlen < 0) return;
    if (rlen > (int)sizeof(resp)) rlen = sizeof(resp);

    /* Reply CMD: STATUS for GET_STATUS, PARAM_VAL for GET_PARAM, otherwise
       echo the request CMD as an ack. */
    uint8_t reply_cmd = cmd;
    if (cmd == RS485_CMD_GET_STATUS) reply_cmd = RS485_CMD_STATUS;
    else if (cmd == RS485_CMD_GET_PARAM) reply_cmd = RS485_CMD_PARAM_VAL;

    send_frame(s_cfg->addr, reply_cmd, resp, (uint8_t)rlen);
}

/* ------------------------------------------------------------------ */
/* RX FSM                                                               */
/* ------------------------------------------------------------------ */

static void rx_reset(void) { s_state = S_SOF; }

static void feed_byte(uint8_t b)
{
    s_last_byte_ms = hal_millis();

    switch (s_state) {
    case S_SOF:
        if (b == RS485_SOF) {
            s_state = S_ADDR;
            s_rx_t0 = s_last_byte_ms;
        }
        break;

    case S_ADDR:
        s_rx_addr = b;
        s_state   = S_CMD;
        break;

    case S_CMD:
        s_rx_cmd = b;
        s_state  = S_LEN;
        break;

    case S_LEN:
        s_rx_plen = b;
        s_rx_idx  = 0;
        s_state   = (b == 0) ? S_CRC : S_PAYLOAD;
        break;

    case S_PAYLOAD:
        s_rx_buf[s_rx_idx++] = b;
        if (s_rx_idx >= s_rx_plen) s_state = S_CRC;
        break;

    case S_CRC: {
        uint8_t hdr[3 + RS485_MAX_PAYLOAD];
        hdr[0] = s_rx_addr; hdr[1] = s_rx_cmd; hdr[2] = s_rx_plen;
        if (s_rx_plen) memcpy(&hdr[3], s_rx_buf, s_rx_plen);
        if (b == crc8(hdr, 3 + s_rx_plen)) {
            if (s_rx_addr == s_cfg->addr ||
                s_rx_addr == RS485_ADDR_BROADCAST) {
                dispatch(s_rx_addr, s_rx_cmd, s_rx_buf, s_rx_plen);
            }
        }
        rx_reset();
        break;
    }
    }
}

/* ------------------------------------------------------------------ */
/* Streaming                                                            */
/* ------------------------------------------------------------------ */

static bool bus_quiet(void)
{
    /* Don't TX while a frame is mid-flight or the master is back-to-back. */
    if (s_state != S_SOF) return false;
    return (hal_millis() - s_last_byte_ms) >= RS485_INTERFRAME_GAP_MS;
}

static void stream_tick(void)
{
    if (!s_stream_interval || !s_cfg->build_stream) return;
    if ((hal_millis() - s_stream_last_ms) < s_stream_interval) return;
    if (!bus_quiet()) return;

    uint8_t buf[RS485_MAX_PAYLOAD];
    int len = s_cfg->build_stream(buf, sizeof(buf));
    if (len < 0) return;
    if (len > (int)sizeof(buf)) len = sizeof(buf);

    send_frame(s_cfg->addr, RS485_CMD_STREAM_DATA, buf, (uint8_t)len);
    s_stream_last_ms = hal_millis();
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void rs485_slave_init(const rs485_slave_cfg_t *cfg)
{
    s_cfg = cfg;
    rx_reset();
    s_stream_interval = 0;
    s_last_byte_ms    = 0;

    hal_uart_init(115200);
    hal_uart_set_tx_enable(false);
    hal_int_pin_init();
}

void rs485_slave_poll(void)
{
    /* Drain any UART bytes waiting. */
    uint8_t b;
    while (hal_uart_read(&b)) {
        feed_byte(b);
    }

    /* Drop a stalled mid-frame if no progress for RS485_TIMEOUT_MS. */
    if (s_state != S_SOF &&
        (hal_millis() - s_rx_t0) >= RS485_TIMEOUT_MS) {
        rx_reset();
    }

    stream_tick();
}

void rs485_slave_assert_int(bool asserted)
{
    hal_int_pin_drive(asserted);
}
