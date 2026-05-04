/*
 * Wi-Fi AP + HTTP server + captive portal.
 * See web.h for the application contract.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"

#include "dhcpserver.h"
#include "dnsserver.h"
#include "web.h"

#define HTTP_PORT       80
#define MAX_REQ_BYTES   2048
#define MAX_RESP_BYTES  16384
#define HDR_RESERVE     256

static dhcp_server_t g_dhcp;
static dns_server_t  g_dns;
static struct tcp_pcb *g_listen_pcb = NULL;

/* AP IP — also the captive-portal target. */
static ip_addr_t g_ap_ip;

typedef struct http_conn {
    char     req[MAX_REQ_BYTES + 1];
    uint32_t req_len;
    char     resp[MAX_RESP_BYTES];
    uint32_t resp_len;
    uint32_t resp_sent;
} http_conn_t;

static void conn_close(struct tcp_pcb *pcb, http_conn_t *c)
{
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_sent(pcb, NULL);
    tcp_err(pcb, NULL);
    tcp_poll(pcb, NULL, 0);
    if (c) free(c);
    if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);
}

static err_t try_send(struct tcp_pcb *pcb, http_conn_t *c)
{
    while (c->resp_sent < c->resp_len) {
        uint32_t room = tcp_sndbuf(pcb);
        if (room == 0) break;
        uint32_t n = c->resp_len - c->resp_sent;
        if (n > room) n = room;
        err_t e = tcp_write(pcb, c->resp + c->resp_sent, (u16_t)n,
                            TCP_WRITE_FLAG_COPY);
        if (e == ERR_MEM) break;
        if (e != ERR_OK) return e;
        c->resp_sent += n;
    }
    tcp_output(pcb);
    return ERR_OK;
}

static err_t on_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
    (void)len;
    http_conn_t *c = (http_conn_t *)arg;
    if (!c) return ERR_OK;
    if (c->resp_sent < c->resp_len) {
        try_send(pcb, c);
    } else {
        conn_close(pcb, c);
    }
    return ERR_OK;
}

static err_t on_poll(void *arg, struct tcp_pcb *pcb)
{
    /* idle for too long — drop. */
    http_conn_t *c = (http_conn_t *)arg;
    conn_close(pcb, c);
    return ERR_OK;
}

static void on_err(void *arg, err_t err)
{
    (void)err;
    http_conn_t *c = (http_conn_t *)arg;
    if (c) free(c);
}

/* Parse "GET /path?query HTTP/1.x" out of c->req. Returns 0 if not yet
 * a complete request line + headers, 1 if parsed (path/query into bufs),
 * -1 on malformed. */
static int parse_request(http_conn_t *c,
                         char *path, size_t path_cap,
                         char *query, size_t query_cap)
{
    /* need full headers (terminator \r\n\r\n) before we proceed */
    if (!strstr(c->req, "\r\n\r\n")) return 0;

    char *sp1 = strchr(c->req, ' ');
    if (!sp1) return -1;
    char *sp2 = strchr(sp1 + 1, ' ');
    if (!sp2) return -1;
    *sp2 = 0;
    char *url = sp1 + 1;
    char *qm = strchr(url, '?');
    if (qm) {
        *qm = 0;
        snprintf(query, query_cap, "%s", qm + 1);
    } else {
        query[0] = 0;
    }
    snprintf(path, path_cap, "%s", url);
    return 1;
}

static void build_response(http_conn_t *c,
                           int status,
                           const char *content_type,
                           const char *body, size_t body_len,
                           const char *extra_hdr)
{
    const char *reason =
        (status == 200) ? "OK" :
        (status == 302) ? "Found" :
        (status == 400) ? "Bad Request" :
        (status == 404) ? "Not Found" : "OK";

    /* Reserve room for headers; clamp body_len if needed BEFORE writing
     * the Content-Length header so the advertised size always matches. */
    if (body_len > sizeof(c->resp) - HDR_RESERVE)
        body_len = sizeof(c->resp) - HDR_RESERVE;

    int n = snprintf(c->resp, HDR_RESERVE,
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %u\r\n"
                     "Cache-Control: no-store\r\n"
                     "Connection: close\r\n"
                     "%s"
                     "\r\n",
                     status, reason, content_type,
                     (unsigned)body_len,
                     extra_hdr ? extra_hdr : "");
    if (n < 0) n = 0;
    if ((size_t)n >= HDR_RESERVE) n = HDR_RESERVE - 1;

    memcpy(c->resp + n, body, body_len);
    c->resp_len  = (uint32_t)(n + body_len);
    c->resp_sent = 0;
}

static void serve(http_conn_t *c)
{
    char path[128]  = {0};
    char query[256] = {0};
    int r = parse_request(c, path, sizeof(path), query, sizeof(query));
    if (r <= 0) {
        build_response(c, 400, "text/plain", "bad request", 11, NULL);
        return;
    }

    /* Body buffer that the app may fill. */
    static char body[MAX_RESP_BYTES];
    const char *ctype = "text/html; charset=utf-8";
    int status = 200;

    size_t blen = app_http(path, query, body, sizeof(body),
                           &ctype, &status);
    printf("[web] %s%s%s -> %d (%u bytes)\n",
           path, query[0] ? "?" : "", query, status, (unsigned)blen);

    if (status == 302) {
        /* app wrote a Location URL into body */
        char hdr[160];
        snprintf(hdr, sizeof(hdr), "Location: %s\r\n", body);
        const char *html = "<a href=\"/\">/</a>";
        build_response(c, 302, "text/html; charset=utf-8",
                       html, strlen(html), hdr);
        return;
    }
    build_response(c, status, ctype, body, blen, NULL);
}

static err_t on_recv(void *arg, struct tcp_pcb *pcb,
                     struct pbuf *p, err_t err)
{
    http_conn_t *c = (http_conn_t *)arg;
    if (!p) { conn_close(pcb, c); return ERR_OK; }
    if (err != ERR_OK || !c) {
        if (p) pbuf_free(p);
        conn_close(pcb, c);
        return ERR_OK;
    }

    /* append to req buffer */
    uint16_t copy = p->tot_len;
    if (copy > MAX_REQ_BYTES - c->req_len)
        copy = MAX_REQ_BYTES - c->req_len;
    pbuf_copy_partial(p, c->req + c->req_len, copy, 0);
    c->req_len += copy;
    c->req[c->req_len] = 0;
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    /* once we have full headers, build & start sending the response */
    if (c->resp_len == 0 && strstr(c->req, "\r\n\r\n")) {
        serve(c);
        try_send(pcb, c);
        if (c->resp_sent >= c->resp_len) {
            conn_close(pcb, c);
        }
    }
    return ERR_OK;
}

static err_t on_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
    (void)arg;
    if (err != ERR_OK || !pcb) return ERR_VAL;
    http_conn_t *c = (http_conn_t *)calloc(1, sizeof(http_conn_t));
    if (!c) { tcp_abort(pcb); return ERR_ABRT; }
    tcp_arg(pcb, c);
    tcp_recv(pcb, on_recv);
    tcp_sent(pcb, on_sent);
    tcp_err(pcb, on_err);
    tcp_poll(pcb, on_poll, 20);    /* ~10s idle (poll = 0.5s × 20) */
    tcp_nagle_disable(pcb);
    return ERR_OK;
}

bool web_init(const char *ssid, const char *password)
{
    if (cyw43_arch_init()) {
        printf("[web] cyw43_arch_init failed\n");
        return false;
    }
    cyw43_arch_enable_ap_mode(ssid, password,
                              password ? CYW43_AUTH_WPA2_AES_PSK
                                       : CYW43_AUTH_OPEN);

    IP4_ADDR(ip_2_ip4(&g_ap_ip), 192, 168, 4, 1);
    ip_addr_t mask;
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    /* Explicitly set the AP netif IP so it doesn't depend on SDK defaults. */
    struct netif *ap = &cyw43_state.netif[CYW43_ITF_AP];
    netif_set_addr(ap, ip_2_ip4(&g_ap_ip), ip_2_ip4(&mask),
                       ip_2_ip4(&g_ap_ip));

    dhcp_server_init(&g_dhcp, &g_ap_ip, &mask);
    dns_server_init(&g_dns, &g_ap_ip);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return false;
    if (tcp_bind(pcb, IP_ANY_TYPE, HTTP_PORT) != ERR_OK) {
        tcp_close(pcb);
        return false;
    }
    g_listen_pcb = tcp_listen_with_backlog(pcb, 4);
    if (!g_listen_pcb) return false;
    tcp_accept(g_listen_pcb, on_accept);

    printf("[web] AP up: SSID=\"%s\"  IP=192.168.4.1\n", ssid);
    return true;
}

void web_poll(void)
{
#if PICO_CYW43_ARCH_POLL
    cyw43_arch_poll();
#endif
}
