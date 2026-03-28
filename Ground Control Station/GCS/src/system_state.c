#include "system_state.h"

volatile sys_state_t g_sys_state = SYS_BOOT;

void sys_state_set(sys_state_t new_state)
{
    g_sys_state = new_state;
}

sys_state_t sys_state_get(void)
{
    return g_sys_state;
}
