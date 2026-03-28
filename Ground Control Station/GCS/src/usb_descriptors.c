#include "tusb.h"
#include "pico/unique_id.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* USB identifiers                                                       */
/* ------------------------------------------------------------------ */
#define USB_VID     0x2E8A   /* Raspberry Pi Foundation */
#define USB_PID     0x000F   /* Custom PID for GCS bottom panel */
#define USB_BCD     0x0200   /* USB 2.0 */

/* ------------------------------------------------------------------ */
/* Device descriptor                                                     */
/* ------------------------------------------------------------------ */
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

/* ------------------------------------------------------------------ */
/* Configuration descriptor                                              */
/* ------------------------------------------------------------------ */
enum {
    ITF_NUM_CDC_0 = 0,
    ITF_NUM_CDC_0_DATA,
    ITF_NUM_TOTAL
};

#define EPNUM_CDC_0_NOTIF   0x81
#define EPNUM_CDC_0_OUT     0x02
#define EPNUM_CDC_0_IN      0x82

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

uint8_t const desc_configuration[] = {
    /* Config number, interface count, string index, total length,
       attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    /* CDC: interface number, string index, EP notification address and size,
       EP data address (out, in) and size */
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8,
                       EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* ------------------------------------------------------------------ */
/* String descriptors                                                    */
/* ------------------------------------------------------------------ */
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},   /* 0: supported language = English (0x0409) */
    "KL-UAS-System",              /* 1: Manufacturer */
    "GCS Bottom Panel",           /* 2: Product */
    NULL,                          /* 3: Serial number — NULL uses RP2040 flash UID */
    "GCS CDC",                    /* 4: CDC interface */
};

static uint16_t s_str_buf[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&s_str_buf[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else if (index == 3) {
        /* Use Pico unique board ID as serial string */
        pico_unique_board_id_t uid;
        pico_get_unique_board_id(&uid);
        chr_count = 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES;
        if (chr_count > (uint8_t)(TU_ARRAY_SIZE(s_str_buf) - 1)) {
            chr_count = (uint8_t)(TU_ARRAY_SIZE(s_str_buf) - 1);
        }
        static const char hex_chars[] = "0123456789ABCDEF";
        for (uint8_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
            s_str_buf[1 + i * 2 + 0] = hex_chars[(uid.id[i] >> 4) & 0x0F];
            s_str_buf[1 + i * 2 + 1] = hex_chars[ uid.id[i]       & 0x0F];
        }
    } else {
        if (index >= TU_ARRAY_SIZE(string_desc_arr)) return NULL;
        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > (uint8_t)(TU_ARRAY_SIZE(s_str_buf) - 1)) {
            chr_count = (uint8_t)(TU_ARRAY_SIZE(s_str_buf) - 1);
        }
        for (uint8_t i = 0; i < chr_count; i++) {
            s_str_buf[1 + i] = (uint16_t)str[i];
        }
    }

    s_str_buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return s_str_buf;
}
