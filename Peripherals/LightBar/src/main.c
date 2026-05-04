#include "rs485_slave.h"
#include "board.h"

static int h_set_output(const uint8_t *p, uint8_t n,
                        uint8_t *r, uint8_t rs)
{
    (void)r; (void)rs;
    if (n != 2) return -1;
    if (p[0] == 0) board_set_brightness(p[1]);
    return 0;
}

static int h_set_param(const uint8_t *p, uint8_t n,
                       uint8_t *r, uint8_t rs)
{
    (void)r; (void)rs;
    if (n != 3) return -1;
    uint16_t v = (uint16_t)(p[1] | (p[2] << 8));
    switch (p[0]) {
    case LB_PARAM_MODE:   board_set_mode((uint8_t)v); break;
    case LB_PARAM_COLOUR: board_set_colour_rgb16(v);  break;
    default: return -1;
    }
    return 0;
}

static int h_get_status(const uint8_t *p, uint8_t n,
                        uint8_t *r, uint8_t rs)
{
    (void)p; (void)n;
    if (rs < 4) return -1;
    r[0] = board_get_brightness();
    r[1] = board_get_mode();
    uint16_t c = board_get_colour_rgb16();
    r[2] = (uint8_t)(c & 0xFF);
    r[3] = (uint8_t)(c >> 8);
    return 4;
}

static int build_stream(uint8_t *buf, uint8_t buf_size)
{
    if (buf_size < 2) return -1;
    uint16_t mw = board_estimate_power_mw();
    buf[0] = (uint8_t)(mw & 0xFF);
    buf[1] = (uint8_t)(mw >> 8);
    return 2;
}

static const rs485_handler_t s_handlers[] = {
    { RS485_CMD_SET_OUTPUT, h_set_output },
    { RS485_CMD_SET_PARAM,  h_set_param  },
    { RS485_CMD_GET_STATUS, h_get_status },
    { 0, NULL }
};

int main(void)
{
    board_init();
    rs485_slave_cfg_t cfg = {
        .addr        = BOARD_ADDR,
        .fw_version  = 1,
        .handlers    = s_handlers,
        .build_stream = build_stream,
    };
    rs485_slave_init(&cfg);

    while (1) {
        rs485_slave_poll();
        board_tick();
    }
}
