#ifndef RS485_SLAVE_H
#define RS485_SLAVE_H

#include "rs485_proto.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Per-command handler entry. Build a response into resp_buf and return its
   length (0..resp_buf_size). Return -1 to send no reply. For broadcast
   frames resp_buf is NULL and the return value is ignored — handlers must
   still apply side effects. */
typedef struct {
    uint8_t cmd;
    int   (*handler)(const uint8_t *payload, uint8_t plen,
                     uint8_t *resp_buf, uint8_t resp_buf_size);
} rs485_handler_t;

typedef struct {
    uint8_t                 addr;          /* this slave 0x01..0xFE */
    uint8_t                 fw_version;    /* returned in PONG payload */
    const rs485_handler_t  *handlers;      /* terminated by .cmd == 0 */
    /* Optional: build a STREAM_DATA payload. NULL = streaming unsupported. */
    int (*build_stream)(uint8_t *buf, uint8_t buf_size);
} rs485_slave_cfg_t;

/* Initialise the slave. Calls hal_uart_init / hal_int_pin_init internally. */
void rs485_slave_init(const rs485_slave_cfg_t *cfg);

/* Drive the protocol. Call as fast as the loop allows — must run at least
   once per millisecond to avoid losing bytes at 115200 baud. Non-blocking. */
void rs485_slave_poll(void);

/* Drive the /INT line. Open-drain semantics — true pulls low, false
   releases. The line stays asserted until the caller releases it. */
void rs485_slave_assert_int(bool asserted);

#ifdef __cplusplus
}
#endif

#endif
