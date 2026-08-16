#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "scrpd_runtime.h"
#include "scrpd_supervisor.h"

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal)
{
    (void)signal;
    running = 0;
}

int main(void)
{
    scrpd_runtime_t runtime;

    scrpd_runtime_init(
        &runtime,
        "sleep-test"
    );

    runtime.runtime.pid = 0;
    runtime.runtime.state = SCRPD_STATE_STOPPED;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("SCRPD SUPERVISOR\n");
    printf("Name:  %s\n", runtime.runtime.name);
    printf("State: STOPPED\n");
    printf("Supervisor loop started\n");

    while (running) {
        int alive = scrpd_runtime_is_alive(&runtime);

        printf(
            "SCRPD-SUPERVISE: state=%d alive=%s\n",
            runtime.runtime.state,
            alive ? "yes" : "no"
        );

        sleep(1);
    }

    printf("SCRPD SUPERVISOR: stopped\n");

    return 0;
}
