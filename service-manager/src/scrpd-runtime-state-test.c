#include <stdio.h>
#include <string.h>

#include "scrpd_runtime.h"
#include "scrpd_runtime_state.h"

int main(void)
{
    const char *state_directory =
        "/tmp/scrpd-runtime-state-test";

    scrpd_runtime_t runtime;

    scrpd_runtime_init(
        &runtime,
        "sleep-test"
    );

    scrpd_runtime_set_pid(
        &runtime,
        12345
    );

    scrpd_runtime_set_state(
        &runtime,
        SCRPD_STATE_RUNNING
    );

    printf("SCRPD RUNTIME STATE TEST\n\n");

    printf("=== SAVE ===\n");
    printf("Name:  %s\n", runtime.runtime.name);
    printf("PID:   %ld\n", runtime.runtime.pid);
    printf("State: %s\n",
           scrpd_state_name(runtime.runtime.state));

    if (scrpd_runtime_state_save(
            &runtime,
            state_directory
        ) != 0) {

        printf("Save:  FAIL\n");
        return 1;
    }

    printf("Save:  PASS\n\n");

    scrpd_runtime_t loaded;

    scrpd_runtime_init(
        &loaded,
        "sleep-test"
    );

    printf("=== LOAD ===\n");

    if (scrpd_runtime_state_load(
            &loaded,
            state_directory
        ) != 0) {

        printf("Load:  FAIL\n");
        return 1;
    }

    printf("Name:  %s\n", loaded.runtime.name);
    printf("PID:   %ld\n", loaded.runtime.pid);
    printf("State: %s\n",
           scrpd_state_name(loaded.runtime.state));

    if (strcmp(
            loaded.runtime.name,
            "sleep-test"
        ) != 0 ||
        loaded.runtime.pid != 12345 ||
        loaded.runtime.state != SCRPD_STATE_RUNNING) {

        printf("Load:  FAIL\n");
        return 1;
    }

    printf("Load:  PASS\n\n");

    printf("=== CLEAR ===\n");

    if (scrpd_runtime_state_clear(
            &loaded,
            state_directory
        ) != 0) {

        printf("Clear: FAIL\n");
        return 1;
    }

    printf("Clear: PASS\n\n");

    printf(
        "TEST: RUNTIME STATE PERSISTENCE PASS\n"
    );

    return 0;
}
