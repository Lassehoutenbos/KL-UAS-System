#ifndef USB_CDC_H
#define USB_CDC_H

#include "FreeRTOS.h"
#include "task.h"

/**
 * Tick count of the last received heartbeat (type 0x05) from the Pi.
 * Written by proto_handle_rx(), read by cdc_task() for timeout detection.
 */
extern volatile TickType_t g_last_heartbeat_rx_tick;

/**
 * FreeRTOS task: initializes TinyUSB and calls tud_task() every 1 ms.
 * Must run at the highest priority to ensure USB enumeration succeeds.
 */
void usb_device_task(void *param);

/**
 * FreeRTOS task: drains g_tx_queue → tud_cdc_write(), feeds received
 * bytes into proto_handle_rx(), and sends heartbeat packets every 1 s.
 * Detects heartbeat timeout (3 s) and USB disconnect/reconnect.
 */
void cdc_task(void *param);

#endif /* USB_CDC_H */
