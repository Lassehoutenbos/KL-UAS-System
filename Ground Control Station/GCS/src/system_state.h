#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

typedef enum {
    SYS_BOOT,           /* hardware initialising */
    SYS_INIT,           /* FreeRTOS tasks starting */
    SYS_WAITING_FOR_PI, /* USB up, no heartbeat received yet */
    SYS_CONNECTED,      /* heartbeat exchange active */
    SYS_LOCKED,         /* key switch in locked position */
    SYS_ACTIVE,         /* normal operation */
    SYS_ERROR,          /* fault condition (watchdog reset, etc.) */
} sys_state_t;

extern volatile sys_state_t g_sys_state;

void sys_state_set(sys_state_t new_state);
sys_state_t sys_state_get(void);

#endif /* SYSTEM_STATE_H */
