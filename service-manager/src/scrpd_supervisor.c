#include <stddef.h>

#include "scrpd_supervisor.h"

const char *scrpd_state_name(
    scrpd_service_state_t state
)
{
    switch (state) {
        case SCRPD_STATE_STOPPED:
            return "STOPPED";

        case SCRPD_STATE_STARTING:
            return "STARTING";

        case SCRPD_STATE_RUNNING:
            return "RUNNING";

        case SCRPD_STATE_STOPPING:
            return "STOPPING";

        case SCRPD_STATE_FAILED:
            return "FAILED";

        default:
            return "UNKNOWN";
    }
}

int scrpd_state_is_active(
    scrpd_service_state_t state
)
{
    return state == SCRPD_STATE_STARTING ||
           state == SCRPD_STATE_RUNNING ||
           state == SCRPD_STATE_STOPPING;
}
