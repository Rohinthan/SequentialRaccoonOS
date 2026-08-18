#include <stdio.h>

#include "scrpd_supervisor.h"

static void print_state(scrpd_service_state_t state)
{
    printf(
        "%d -> %s -> active=%s\n",
        state,
        scrpd_state_name(state),
        scrpd_state_is_active(state) ? "yes" : "no"
    );
}

int main(void)
{
    printf("SCRPD SUPERVISOR STATE TEST\n");

    print_state(SCRPD_STATE_STOPPED);
    print_state(SCRPD_STATE_STARTING);
    print_state(SCRPD_STATE_RUNNING);
    print_state(SCRPD_STATE_STOPPING);
    print_state(SCRPD_STATE_FAILED);

    return 0;
}

