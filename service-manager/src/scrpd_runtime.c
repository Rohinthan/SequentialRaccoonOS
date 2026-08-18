#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <sys/types.h>
#include <string.h>

#include "scrpd_runtime.h"

void scrpd_runtime_init(
    scrpd_runtime_t *runtime,
    const char *name
)
{
    if (runtime == NULL) {
        return;
    }

    memset(runtime, 0, sizeof(*runtime));

    if (name != NULL) {
        strncpy(
            runtime->runtime.name,
            name,
            SCRPD_SERVICE_STATE_NAME_MAX - 1
        );

        runtime->runtime.name[
            SCRPD_SERVICE_STATE_NAME_MAX - 1
        ] = '\0';
    }

    runtime->runtime.state = SCRPD_STATE_STOPPED;
    runtime->runtime.pid = 0;
}

int scrpd_runtime_set_pid(
    scrpd_runtime_t *runtime,
    long pid
)
{
    if (runtime == NULL || pid <= 0) {
        return -1;
    }

    runtime->runtime.pid = pid;

    return 0;
}

int scrpd_runtime_set_state(
    scrpd_runtime_t *runtime,
    scrpd_service_state_t state
)
{
    if (runtime == NULL) {
        return -1;
    }

    runtime->runtime.state = state;

    return 0;
}

int scrpd_runtime_is_alive(
    const scrpd_runtime_t *runtime
)
{
    if (runtime == NULL || runtime->runtime.pid <= 0) {
        return 0;
    }

    if (kill(
            (pid_t)runtime->runtime.pid,
            0
        ) == 0) {
        return 1;
    }

    return 0;
}
