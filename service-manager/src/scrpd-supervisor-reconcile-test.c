#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "scrpd_runtime.h"

static void print_runtime(
    const scrpd_runtime_t *runtime
)
{
    printf(
        "Name:   %s\n"
        "PID:    %ld\n"
        "State:  %s\n"
        "Alive:  %s\n",
        runtime->runtime.name,
        runtime->runtime.pid,
        scrpd_state_name(runtime->runtime.state),
        scrpd_runtime_is_alive(runtime) ? "yes" : "no"
    );
}

int main(void)
{
    scrpd_runtime_t runtime;

    printf("SCRPD SUPERVISOR RECONCILE TEST\n\n");

    scrpd_runtime_init(
        &runtime,
        "sleep-test"
    );

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execl(
            "/bin/sleep",
            "/bin/sleep",
            "30",
            (char *)NULL
        );

        _exit(127);
    }

    scrpd_runtime_set_pid(
        &runtime,
        (long)pid
    );

    scrpd_runtime_set_state(
        &runtime,
        SCRPD_STATE_STARTING
    );

    printf("=== BEFORE RECONCILE ===\n");
    print_runtime(&runtime);

    printf("\n=== RECONCILE ===\n");

    if (scrpd_supervisor_reconcile(&runtime) != 0) {
        fprintf(stderr, "reconcile failed\n");
        return 1;
    }

    print_runtime(&runtime);

    printf("\n=== KILL PROCESS ===\n");

    kill(pid, SIGTERM);

    waitpid(pid, NULL, 0);

    if (scrpd_supervisor_reconcile(&runtime) != 0) {
        fprintf(stderr, "reconcile failed\n");
        return 1;
    }

    print_runtime(&runtime);

    return 0;
}
