#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scrpd_service.h"

static void print_usage(void)
{
    printf("SCRPD Service Runner\n\n");

    printf("Usage:\n");
    printf("  scrpd-run <root-directory> <service-file>\n\n");

    printf("Example:\n");
    printf("  scrpd-run /tmp/scrp-root "
           "service-manager/services/hello.service\n");
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        print_usage();
        return 1;
    }

    const char *root = argv[1];
    const char *service_path = argv[2];

    scrpd_service_t service;

    if (scrpd_service_load(service_path, &service) != 0) {
        fprintf(stderr,
                "SCRPD-RUN: invalid service\n");
        return 1;
    }

    char executable[4096];

    int written = snprintf(
        executable,
        sizeof(executable),
        "%s%s",
        root,
        service.command
    );

    if (written < 0 ||
        (size_t)written >= sizeof(executable)) {

        fprintf(stderr,
                "SCRPD-RUN: executable path too long\n");
        return 1;
    }

    printf("SCRPD-RUN: service\n");
    printf("  Name:    %s\n", service.name);
    printf("  Command: %s\n", service.command);
    printf("  Root:    %s\n", root);
    printf("  Exec:    %s\n", executable);

    printf("SCRPD-RUN: starting service...\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("SCRPD-RUN: fork");
        return 1;
    }

    if (pid == 0) {
        execl(
            executable,
            executable,
            (char *)NULL
        );

        perror("SCRPD-RUN: exec");
        _exit(127);
    }

    printf("SCRPD-RUN: started\n");
    printf("  PID: %ld\n", (long)pid);

    int status;

    if (waitpid(pid, &status, 0) < 0) {
        perror("SCRPD-RUN: waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);

        printf("SCRPD-RUN: service exited\n");
        printf("  Exit code: %d\n", exit_code);

        return exit_code;
    }

    if (WIFSIGNALED(status)) {
        printf("SCRPD-RUN: service terminated\n");
        printf("  Signal: %d\n",
               WTERMSIG(status));

        return 1;
    }

    return 1;
}
