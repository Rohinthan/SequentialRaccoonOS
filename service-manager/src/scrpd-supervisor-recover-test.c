#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "scrpd_runtime.h"
#include "scrpd_service.h"
#include "scrpd_supervised_start.h"
#include "scrpd_supervisor_recover.h"

int main(void)
{
    scrpd_service_t service;
    scrpd_runtime_t runtime;

    if (scrpd_service_load(
            "/tmp/scrpd-tests/sleep.service",
            &service
        ) != 0) {

        fprintf(
            stderr,
            "TEST: failed to load service\n"
        );

        return 1;
    }

    printf("SCRPD SUPERVISOR RECOVERY TEST\n\n");

    if (scrpd_supervised_start(
            &runtime,
            &service,
            "/"
        ) != 0) {

        fprintf(
            stderr,
            "TEST: supervised start failed\n"
        );

        return 1;
    }

    long old_pid = runtime.runtime.pid;

    printf(
        "\n=== STARTED ===\n"
        "PID:    %ld\n"
        "State:  %s\n"
        "Alive:  %s\n",
        old_pid,
        scrpd_state_name(runtime.runtime.state),
        scrpd_runtime_is_alive(&runtime) ? "yes" : "no"
    );

    printf("\n=== KILL PROCESS ===\n");

    if (kill(
            (pid_t)old_pid,
            SIGTERM
        ) != 0) {

        perror("TEST: kill");

        return 1;
    }

    sleep(1);

    if (scrpd_supervisor_reconcile(
            &runtime
        ) != 0) {

        fprintf(
            stderr,
            "TEST: reconcile failed\n"
        );

        return 1;
    }

    printf(
        "State:  %s\n"
        "Alive:  %s\n",
        scrpd_state_name(runtime.runtime.state),
        scrpd_runtime_is_alive(&runtime) ? "yes" : "no"
    );

    if (runtime.runtime.state != SCRPD_STATE_FAILED) {
        fprintf(
            stderr,
            "TEST: service did not enter FAILED state\n"
        );

        return 1;
    }

    printf("\n=== RECOVER ===\n");

    if (scrpd_supervisor_recover(
            &runtime,
            &service,
            "/"
        ) != 0) {

        fprintf(
            stderr,
            "TEST: recovery failed\n"
        );

        return 1;
    }

    long new_pid = runtime.runtime.pid;

    printf(
        "\n=== RECOVERED ===\n"
        "Old PID: %ld\n"
        "New PID: %ld\n"
        "State:   %s\n"
        "Alive:   %s\n",
        old_pid,
        new_pid,
        scrpd_state_name(runtime.runtime.state),
        scrpd_runtime_is_alive(&runtime) ? "yes" : "no"
    );

    if (old_pid == new_pid) {
        fprintf(
            stderr,
            "TEST: PID did not change\n"
        );

        kill((pid_t)new_pid, SIGTERM);
        return 1;
    }

    if (runtime.runtime.state != SCRPD_STATE_RUNNING) {
        fprintf(
            stderr,
            "TEST: recovered service is not RUNNING\n"
        );

        kill((pid_t)new_pid, SIGTERM);
        return 1;
    }

    if (!scrpd_runtime_is_alive(&runtime)) {
        fprintf(
            stderr,
            "TEST: recovered service is not alive\n"
        );

        return 1;
    }

    kill(
        (pid_t)new_pid,
        SIGTERM
    );

    printf("\nTEST: SUPERVISOR RECOVERY PASS\n");

    return 0;
}
