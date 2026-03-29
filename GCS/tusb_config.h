#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

/* MCU and OS */
#define CFG_TUSB_MCU            OPT_MCU_RP2040
#define CFG_TUSB_OS             OPT_OS_FREERTOS

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
