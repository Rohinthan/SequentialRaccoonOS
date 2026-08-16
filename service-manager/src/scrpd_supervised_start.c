#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "scrpd_runtime.h"
#include "scrpd_service.h"

int scrpd_supervised_start(
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

    /*
     * Extract executable from the command.
     *
     * Example:
     *   /bin/sleep 60
     *
     * becomes:
     *   executable = /bin/sleep
     *   arguments  = 60
     */

    char command[SCRPD_SERVICE_COMMAND_MAX];

    snprintf(
        command,
        sizeof(command),
        "%s",
        service->command
    );

    char *executable_name = command;

    while (*executable_name == ' ') {
        executable_name++;
    }

    char *arguments = executable_name;

    while (*arguments != '\0' &&
           *arguments != ' ') {
        arguments++;
    }

    if (*arguments != '\0') {
        *arguments = '\0';
        arguments++;

        while (*arguments == ' ') {
            arguments++;
        }
    }

    char executable[4096];

    int written = snprintf(
        executable,
        sizeof(executable),
        "%s%s",
        root,
        executable_name
    );

    if (written < 0 ||
        (size_t)written >= sizeof(executable)) {
        fprintf(
            stderr,
            "SCRPD-SUPERVISED: executable path too long\n"
        );
        return -1;
    }

    if (access(executable, X_OK) != 0) {
        fprintf(
            stderr,
            "SCRPD-SUPERVISED: executable unavailable: %s\n",
            executable
        );
        return -1;
    }

    scrpd_runtime_init(
        runtime,
        service->name
    );

    scrpd_runtime_set_state(
        runtime,
        SCRPD_STATE_STARTING
    );

    pid_t pid = fork();

    if (pid < 0) {
        perror("SCRPD-SUPERVISED: fork");

        scrpd_runtime_set_state(
            runtime,
            SCRPD_STATE_FAILED
        );

        return -1;
    }

    if (pid == 0) {

        if (*arguments != '\0') {
            execl(
                executable,
                executable,
                arguments,
                (char *)NULL
            );
        } else {
            execl(
                executable,
                executable,
                (char *)NULL
            );
        }

        perror("SCRPD-SUPERVISED: exec");
        _exit(127);
    }

    if (scrpd_runtime_set_pid(
            runtime,
            (long)pid
        ) != 0) {

        kill(
            pid,
            SIGTERM
        );

        return -1;
    }

    if (!scrpd_runtime_is_alive(runtime)) {

        scrpd_runtime_set_state(
            runtime,
            SCRPD_STATE_FAILED
        );

        return -1;
    }

    scrpd_runtime_set_state(
        runtime,
        SCRPD_STATE_RUNNING
    );

    printf(
        "SCRPD-SUPERVISED: service started\n"
    );

    printf(
        "  Name:  %s\n",
        runtime->runtime.name
    );

    printf(
        "  PID:   %ld\n",
        runtime->runtime.pid
    );

    printf(
        "  State: RUNNING\n"
    );

    return 0;
}
