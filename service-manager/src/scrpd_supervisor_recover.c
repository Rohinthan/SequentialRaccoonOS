#include <stdio.h>

#include "scrpd_supervisor_recover.h"
#include "scrpd_supervised_start.h"

int scrpd_supervisor_recover(
    scrpd_runtime_t *runtime,
    const scrpd_service_t *service,
    const char *root
)
{
    if (runtime == NULL ||
        service == NULL ||
        root == NULL) {
        return -1;
    }

    if (runtime->runtime.state != SCRPD_STATE_FAILED) {
        return 0;
    }

    printf(
        "SCRPD-RECOVER: restarting failed service\n"
    );

    printf(
        "  Name: %s\n",
        runtime->runtime.name
    );

    if (scrpd_supervised_start(
            runtime,
            service,
            root
        ) != 0) {

        runtime->runtime.state = SCRPD_STATE_FAILED;

        fprintf(
            stderr,
            "SCRPD-RECOVER: restart failed\n"
        );

        return -1;
    }

    printf(
        "SCRPD-RECOVER: service recovered\n"
    );

    return 0;
}

