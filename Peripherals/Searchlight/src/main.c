#include "rs485_slave.h"
#include "board.h"

/* Portable across all MCUs — board.h hides the hardware. The matching
   board_<mcu>.c is selected by the app's CMakeLists.txt. */

static int h_set_output(const uint8_t *p, uint8_t n,
                        uint8_t *r, uint8_t rs)
{
    (void)r; (void)rs;
    if (n != 2) return -1;
    if (p[0] == 0) board_set_pwm(p[1]);   /* channel 0 = brightness */
    return 0;
}

static int h_get_status(const uint8_t *p, uint8_t n,
                        uint8_t *r, uint8_t rs)
{
    (void)p; (void)n;
    if (rs < 3) return -1;
    r[0] = board_get_pwm();
    r[1] = (uint8_t)board_read_temp_c();
    r[2] = board_fault_flags();
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
        .addr        = BOARD_ADDR,
        .fw_version  = 1,
        .handlers    = s_handlers,
        .build_stream = NULL,
    };
    rs485_slave_init(&cfg);

    bool int_asserted = false;
    while (1) {
        rs485_slave_poll();

        bool ot = board_overtemp();
        if (ot != int_asserted) {
            rs485_slave_assert_int(ot);
            int_asserted = ot;
        }
    }
}
