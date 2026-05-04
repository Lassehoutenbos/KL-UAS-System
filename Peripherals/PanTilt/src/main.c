#include "rs485_slave.h"
#include "board.h"

static int h_set_output(const uint8_t *p, uint8_t n,
                        uint8_t *r, uint8_t rs)
{
    (void)r; (void)rs;
    if (n != 2) return -1;
    board_set_angle(p[0], p[1]);
    return 0;
}

static int h_get_status(const uint8_t *p, uint8_t n,
                        uint8_t *r, uint8_t rs)
{
    (void)p; (void)n;
    if (rs < 3) return -1;
    r[0] = board_get_angle(0);
    r[1] = board_get_angle(1);
    r[2] = board_is_moving();
    return 3;
}

static const rs485_handler_t s_handlers[] = {
    { RS485_CMD_SET_OUTPUT, h_set_output },
    { RS485_CMD_GET_STATUS, h_get_status },
    { 0, NULL }
};

int main(void)
{
    board_init();
    rs485_slave_cfg_t cfg = {
        .addr = BOARD_ADDR, .fw_version = 1, .handlers = s_handlers,
    };
    rs485_slave_init(&cfg);
    while (1) rs485_slave_poll();
}
