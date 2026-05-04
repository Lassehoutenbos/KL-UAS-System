#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/* MCU — RP2040 USB hardware, works on both RP2040 and RP2350 */
#define CFG_TUSB_MCU            OPT_MCU_RP2040
/* CFG_TUSB_OS is set to OPT_OS_FREERTOS by the pico-sdk CMake */

/* Device class enables */
#define CFG_TUD_ENABLED         1
#define CFG_TUD_CDC             1
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

/* Endpoint 0 size */
#define CFG_TUD_ENDPOINT0_SIZE  64

/* CDC buffer sizes */
#define CFG_TUD_CDC_RX_BUFSIZE  64
#define CFG_TUD_CDC_TX_BUFSIZE  256   /* larger TX for burst ADC packets */
#define CFG_TUD_CDC_EP_BUFSIZE  64

#endif /* TUSB_CONFIG_H */
