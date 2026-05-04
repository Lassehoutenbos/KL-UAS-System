/*
 * Wi-Fi AP + HTTP server + captive portal for the RS-485 peripheral
 * emulator. Brings up a SoftAP, embedded DHCP/DNS servers and a tiny
 * raw-lwIP HTTP server. The application provides app_http() to render
 * responses for any path/query the UI invokes.
 */
#ifndef WEB_H
#define WEB_H

#include <stddef.h>
#include <stdbool.h>

bool web_init(const char *ssid, const char *password);
void web_poll(void);

/* Application callback: build an HTTP response body for the given URL.
 * Returns the number of bytes written. Set *content_type and *status if
 * non-default (defaults: "text/html; charset=utf-8" / 200). Set *status
 * to 302 and write the redirect URL into out (NUL-terminated) to trigger
 * a Location: redirect — the body is then ignored.
 */
size_t app_http(const char *path, const char *query,
                char *out, size_t cap,
                const char **content_type, int *status);

#endif
