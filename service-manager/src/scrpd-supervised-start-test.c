#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "scrpd_runtime.h"
#include "scrpd_service.h"
#include "scrpd_supervised_start.h"

int main(void)
{
    scrpd_service_t service;
    scrpd_runtime_t runtime;

    if (scrpd_service_load(
            "service-manager/services/sleep-test.service",
            &service
        ) != 0) {
        fprintf(stderr, "TEST: failed to load service\n");
        return 1;
    }

    printf("SCRPD SUPERVISED START TEST\n\n");

    if (scrpd_supervised_start(
            &runtime,
            &service,
            "/"
        ) != 0) {
        fprintf(stderr, "TEST: supervised start failed\n");
        return 1;
    }

    printf("\n=== RUNTIME ===\n");
    printf("Name:   %s\n", runtime.runtime.name);
    printf("PID:    %ld\n", runtime.runtime.pid);
    printf("State:  %d\n", runtime.runtime.state);
    printf("Alive:  %s\n",
           scrpd_runtime_is_alive(&runtime)
               ? "yes"
               : "no");

    if (runtime.runtime.state != SCRPD_STATE_RUNNING) {
        fprintf(stderr, "TEST: state is not RUNNING\n");
        kill((pid_t)runtime.runtime.pid, SIGTERM);
        return 1;
    }

    if (!scrpd_runtime_is_alive(&runtime)) {
        fprintf(stderr, "TEST: service is not alive\n");
        return 1;
    }

    printf("\nTEST: supervised start PASS\n");

    kill(
        (pid_t)runtime.runtime.pid,
        SIGTERM
    );

    return 0;
}
