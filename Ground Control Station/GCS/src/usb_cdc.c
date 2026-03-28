#include "usb_cdc.h"
#include "protocol.h"
#include "system_state.h"
#include "tusb.h"

#include "FreeRTOS.h"
#include "task.h"

/* ------------------------------------------------------------------ */
/* Shared state                                                          */
/* ------------------------------------------------------------------ */
volatile TickType_t g_last_heartbeat_rx_tick = 0;

/* ------------------------------------------------------------------ */
/* USB device task (TinyUSB event loop — highest priority)              */
/* ------------------------------------------------------------------ */
void usb_device_task(void *param)
{
    (void)param;

    tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(0, &dev_init);

    while (1) {
        tud_task();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* ------------------------------------------------------------------ */
/* CDC task                                                              */
/* ------------------------------------------------------------------ */

#define HEARTBEAT_TX_INTERVAL_MS    1000
#define HEARTBEAT_TIMEOUT_MS        3000

void cdc_task(void *param)
{
    (void)param;

    uint8_t rx_buf[CFG_TUD_CDC_RX_BUFSIZE];
    tx_item_t tx_item;

    TickType_t last_heartbeat_tx = xTaskGetTickCount();
    bool was_connected = false;
    static uint8_t s_hb_seq = 0;

    /* Initialise RX tick so we don't immediately fire timeout */
    g_last_heartbeat_rx_tick = xTaskGetTickCount();

    sys_state_set(SYS_WAITING_FOR_PI);

    while (1) {
        bool cdc_connected = tud_cdc_connected();

        /* -- Detect USB connect / disconnect ----------------------------- */
        if (cdc_connected && !was_connected) {
            /* Just connected */
            proto_send_event(EVT_USB_CONNECTED, 0);
            /* Reset heartbeat timer so we give the Pi time to respond */
            g_last_heartbeat_rx_tick = xTaskGetTickCount();
            last_heartbeat_tx = xTaskGetTickCount();
            sys_state_set(SYS_WAITING_FOR_PI);
        } else if (!cdc_connected && was_connected) {
            /* Just disconnected */
            proto_send_event(EVT_USB_DISCONNECTED, 0);
            sys_state_set(SYS_WAITING_FOR_PI);
        }
        was_connected = cdc_connected;

        if (cdc_connected) {
            /* -- TX: drain queue ----------------------------------------- */
            while (xQueueReceive(g_tx_queue, &tx_item, 0) == pdTRUE) {
                tud_cdc_write(tx_item.buf, tx_item.len);
            }
            tud_cdc_write_flush();

            /* -- RX: feed into protocol state machine -------------------- */
            if (tud_cdc_available()) {
                uint32_t n = tud_cdc_read(rx_buf, sizeof(rx_buf));
                if (n > 0) {
                    proto_handle_rx(rx_buf, n);
                }
            }

            /* -- Heartbeat TX -------------------------------------------- */
            TickType_t now = xTaskGetTickCount();
            if ((now - last_heartbeat_tx) >= pdMS_TO_TICKS(HEARTBEAT_TX_INTERVAL_MS)) {
                last_heartbeat_tx = now;
                heartbeat_pkt_t hb = { .seq = s_hb_seq++ };
                tx_item_t hb_item;
                int len = proto_serialize(hb_item.buf, sizeof(hb_item.buf),
                                          PROTO_TYPE_HEARTBEAT,
                                          (const uint8_t *)&hb, sizeof(hb));
                if (len > 0) {
                    hb_item.len = (uint8_t)len;
                    tud_cdc_write(hb_item.buf, hb_item.len);
                    tud_cdc_write_flush();
                }
            }

            /* -- Heartbeat RX timeout ------------------------------------ */
            sys_state_t cur = sys_state_get();
            if (cur == SYS_CONNECTED || cur == SYS_ACTIVE || cur == SYS_LOCKED) {
                TickType_t now2 = xTaskGetTickCount();
                if ((now2 - g_last_heartbeat_rx_tick) >=
                    pdMS_TO_TICKS(HEARTBEAT_TIMEOUT_MS)) {
                    proto_send_event(EVT_USB_DISCONNECTED, 0);
                    sys_state_set(SYS_WAITING_FOR_PI);
                }
            } else if (cur == SYS_WAITING_FOR_PI) {
                /* Transition to CONNECTED on first heartbeat received */
                TickType_t now2 = xTaskGetTickCount();
                if ((now2 - g_last_heartbeat_rx_tick) <
                    pdMS_TO_TICKS(HEARTBEAT_TX_INTERVAL_MS + 200)) {
                    /* Recent heartbeat received — we're connected */
                    if (g_last_heartbeat_rx_tick != 0 &&
                        g_last_heartbeat_rx_tick > last_heartbeat_tx) {
                        sys_state_set(SYS_CONNECTED);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
