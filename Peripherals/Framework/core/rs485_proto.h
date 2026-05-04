#ifndef RS485_PROTO_H
#define RS485_PROTO_H

/* Wire-format constants for the RS-485 peripheral bus. Mirrors
   GCS/src/rs485.h on the master side. The authoritative spec lives in
   Peripherals/Docs/RS485_PERIPHERAL_BUS.md. */

#define RS485_SOF               0xAB

/* Command bytes */
#define RS485_CMD_PING          0x01
#define RS485_CMD_PONG          0x02
#define RS485_CMD_SET_OUTPUT    0x10
#define RS485_CMD_GET_STATUS    0x11
#define RS485_CMD_STATUS        0x12
#define RS485_CMD_SET_PARAM     0x20
#define RS485_CMD_GET_PARAM     0x21
#define RS485_CMD_PARAM_VAL     0x22
#define RS485_CMD_STREAM_ON     0x30
#define RS485_CMD_STREAM_OFF    0x31
#define RS485_CMD_STREAM_DATA   0x32
#define RS485_CMD_ERROR         0xF0
#define RS485_CMD_SYNC          0xFF

#define RS485_ADDR_BROADCAST    0xFF

/* Timing — must agree with the master (GCS/src/rs485.h) */
#define RS485_TIMEOUT_MS        50
#define RS485_INTERFRAME_GAP_MS 2

/* Frame size limits */
#define RS485_MAX_PAYLOAD       255
#define RS485_FRAME_OVERHEAD    5    /* SOF + ADDR + CMD + LEN + CRC */
#define RS485_MAX_FRAME         (RS485_FRAME_OVERHEAD + RS485_MAX_PAYLOAD)

#endif
