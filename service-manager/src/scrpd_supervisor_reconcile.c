#include <stddef.h>

#include "scrpd_runtime.h"

int scrpd_supervisor_reconcile(
    scrpd_runtime_t *runtime
)
{
    if (runtime == NULL) {
        return -1;
    }

    if (!scrpd_runtime_is_alive(runtime)) {
        if (runtime->runtime.state == SCRPD_STATE_RUNNING ||
            runtime->runtime.state == SCRPD_STATE_STARTING) {

            runtime->runtime.state = SCRPD_STATE_FAILED;
        }

        return 0;
    }

    if (runtime->runtime.state == SCRPD_STATE_STARTING) {
        runtime->runtime.state = SCRPD_STATE_RUNNING;
    }

    return 0;
}
