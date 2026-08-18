#include <stdio.h>
#include <stdlib.h>

#include "scrpd_runtime.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(
            stderr,
            "Usage: scrpd-runtime-test <pid>\n"
        );
        return 1;
    }

    long pid = strtol(argv[1], NULL, 10);

    scrpd_runtime_t runtime;

    scrpd_runtime_init(
        &runtime,
        "sleep-test"
    );

    if (scrpd_runtime_set_pid(
            &runtime,
            pid
        ) != 0) {
        fprintf(
            stderr,
            "invalid PID\n"
        );
        return 1;
    }

    scrpd_runtime_set_state(
        &runtime,
        SCRPD_STATE_RUNNING
    );

    printf("SCRPD RUNTIME TEST\n\n");

    printf("Name:   %s\n",
           runtime.runtime.name);

    printf("PID:    %ld\n",
           runtime.runtime.pid);

    printf("State:  %s\n",
           scrpd_state_name(
               runtime.runtime.state
           ));

    printf("Alive:  %s\n",
           scrpd_runtime_is_alive(&runtime)
               ? "yes"
               : "no");

    return 0;
}
