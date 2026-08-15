#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#include "scrpd_service.h"

static void print_usage(void)
{
    printf("SCRPD Service Restarter\n\n");

    printf("Usage:\n");
    printf("  scrpd-restart <root-directory> "
           "<service-file> <state-directory>\n\n");

    printf("Example:\n");
    printf("  scrpd-restart /tmp/scrp-root "
           "service-manager/services/hello.service "
           "service-manager/state\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        print_usage();
        return 1;
    }

    const char *root = argv[1];
    const char *service_path = argv[2];
    const char *state_dir = argv[3];

    scrpd_service_t service;

    if (scrpd_service_load(service_path, &service) != 0) {
        fprintf(stderr,
                "SCRPD-RESTART: invalid service\n");
        return 1;
    }

    char pid_path[4096];

    int written = snprintf(
        pid_path,
        sizeof(pid_path),
        "%s/%s.pid",
        state_dir,
        service.name
    );

    if (written < 0 ||
        (size_t)written >= sizeof(pid_path)) {

        fprintf(stderr,
                "SCRPD-RESTART: PID path too long\n");
        return 1;
    }

    printf("SCRPD-RESTART: service\n");
    printf("  Name: %s\n", service.name);

    /*
     * Try to stop the existing service.
     */
    FILE *pid_file = fopen(pid_path, "r");

    if (pid_file != NULL) {

        long pid_value;

        if (fscanf(pid_file, "%ld", &pid_value) != 1) {
            fclose(pid_file);

            fprintf(stderr,
                    "SCRPD-RESTART: invalid PID file\n");
            return 1;
        }

        fclose(pid_file);

        pid_t pid = (pid_t)pid_value;

        if (kill(pid, SIGTERM) != 0) {

            if (errno != ESRCH) {
                perror("SCRPD-RESTART: cannot terminate service");
                return 1;
            }

            printf("SCRPD-RESTART: old process already stopped\n");

        } else {
            printf("SCRPD-RESTART: termination signal sent\n");

            /*
             * Give the process a short opportunity to exit.
             */
            int stopped = 0;

            for (int i = 0; i < 50; ++i) {

                if (kill(pid, 0) != 0) {
                    if (errno == ESRCH) {
                        stopped = 1;
                        break;
                    }
                }

		struct timespec delay = {
		    .tv_sec = 0,
		    .tv_nsec = 10000000L
		};

		nanosleep(&delay, NULL);
            }

            if (!stopped && kill(pid, SIGKILL) != 0 &&
                errno != ESRCH) {

                perror("SCRPD-RESTART: cannot force stop service");
                return 1;
            }

            if (!stopped)
                printf("SCRPD-RESTART: forced termination\n");
        }

        if (remove(pid_path) != 0 && errno != ENOENT) {
            perror("SCRPD-RESTART: cannot remove PID file");
            return 1;
        }

        printf("SCRPD-RESTART: old PID state removed\n");

    } else if (errno != ENOENT) {

        perror("SCRPD-RESTART: cannot open PID file");
        return 1;
    }

    /*
     * Start the service again.
     */
    char executable[4096];
    char command[SCRPD_SERVICE_COMMAND_MAX];

    strncpy(
        command,
        service.command,
        sizeof(command) - 1
    );

    command[sizeof(command) - 1] = '\0';

    char *argv_exec[64];
    int argc_exec = 0;

    char *token = strtok(command, " \t");

    while (token != NULL &&
           argc_exec < 63) {

        argv_exec[argc_exec++] = token;
        token = strtok(NULL, " \t");
    }

    argv_exec[argc_exec] = NULL;

    if (argc_exec == 0) {
        fprintf(stderr,
                "SCRPD-RESTART: empty command\n");
        return 1;
    }

    if (strcmp(root, "/") == 0) {

        written = snprintf(
            executable,
            sizeof(executable),
            "%s",
            argv_exec[0]
        );

    } else {

        written = snprintf(
            executable,
            sizeof(executable),
            "%s%s",
            root,
            argv_exec[0]
        );
    }

    if (written < 0 ||
        (size_t)written >= sizeof(executable)) {

        fprintf(stderr,
                "SCRPD-RESTART: executable path too long\n");
        return 1;
    }

    if (access(executable, X_OK) != 0) {

        fprintf(stderr,
                "SCRPD-RESTART: executable unavailable: %s\n",
                executable);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("SCRPD-RESTART: fork");
        return 1;
    }

    if (pid == 0) {

        execv(executable, argv_exec);

        perror("SCRPD-RESTART: exec");
        _exit(127);
    }

    pid_file = fopen(pid_path, "w");

    if (pid_file == NULL) {
        perror("SCRPD-RESTART: cannot create PID file");

        kill(pid, SIGTERM);
        return 1;
    }

    fprintf(pid_file, "%ld\n", (long)pid);

    if (fclose(pid_file) != 0) {

        perror("SCRPD-RESTART: cannot close PID file");

        kill(pid, SIGTERM);
        remove(pid_path);

        return 1;
    }

    printf("SCRPD-RESTART: service restarted\n");
    printf("  PID: %ld\n", (long)pid);

    return 0;
}

