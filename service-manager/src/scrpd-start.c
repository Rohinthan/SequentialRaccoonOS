#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "scrpd_service.h"

#define SCRPD_MAX_ARGS 64

static void print_usage(void)
{
    printf("SCRPD Service Starter\n\n");

    printf("Usage:\n");
    printf("  scrpd-start <root-directory> "
           "<service-file> <state-directory>\n\n");

    printf("Example:\n");
    printf("  scrpd-start /tmp/scrp-root "
           "service-manager/services/hello.service "
           "service-manager/state\n");
}

static int parse_command(
    const char *command,
    char *buffer,
    size_t buffer_size,
    char *argv[],
    size_t argv_capacity
)
{
    if (command == NULL || buffer == NULL || argv == NULL)
        return -1;

    size_t length = strlen(command);

    if (length == 0 || length >= buffer_size)
        return -1;

    memcpy(buffer, command, length + 1);

    size_t argc = 0;
    char *cursor = buffer;

    while (*cursor != '\0') {

        while (*cursor == ' ' ||
               *cursor == '\t' ||
               *cursor == '\n') {
            cursor++;
        }

        if (*cursor == '\0')
            break;

        if (argc + 1 >= argv_capacity)
            return -1;

        argv[argc++] = cursor;

        while (*cursor != '\0' &&
               *cursor != ' ' &&
               *cursor != '\t' &&
               *cursor != '\n') {
            cursor++;
        }

        if (*cursor == '\0')
            break;

        *cursor = '\0';
        cursor++;
    }

    if (argc == 0)
        return -1;

    argv[argc] = NULL;

    return (int)argc;
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
                "SCRPD-START: invalid service\n");
        return 1;
    }

    char command_buffer[SCRPD_SERVICE_COMMAND_MAX];

    char *command_argv[SCRPD_MAX_ARGS];

    int command_argc = parse_command(
        service.command,
        command_buffer,
        sizeof(command_buffer),
        command_argv,
        SCRPD_MAX_ARGS
    );

    if (command_argc < 0) {
        fprintf(stderr,
                "SCRPD-START: invalid command\n");
        return 1;
    }

    char executable[4096];

    int written;

    if (strcmp(root, "/") == 0) {
	written = snprintf(
	    executable,
	    sizeof(executable),
	    "%s",
	    command_argv[0]
	);
    } else {
	written = snprintf(
	    executable,
	    sizeof(executable),
	    "%s%s",
	    root,
	    command_argv[0]
	);
    }


    if (written < 0 ||
        (size_t)written >= sizeof(executable)) {

        fprintf(stderr,
                "SCRPD-START: executable path too long\n");
        return 1;
    }

    if (access(executable, X_OK) != 0) {
        fprintf(stderr,
                "SCRPD-START: executable unavailable: %s\n",
                executable);
        return 1;
    }

    if (mkdir(state_dir, 0755) != 0 &&
        errno != EEXIST) {

        perror("SCRPD-START: cannot create state directory");
        return 1;
    }

    char pid_path[4096];

    written = snprintf(
        pid_path,
        sizeof(pid_path),
        "%s/%s.pid",
        state_dir,
        service.name
    );

    if (written < 0 ||
        (size_t)written >= sizeof(pid_path)) {

        fprintf(stderr,
                "SCRPD-START: PID path too long\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("SCRPD-START: fork");
        return 1;
    }

    if (pid == 0) {

        command_argv[0] = executable;

        execv(
            executable,
            command_argv
        );

        perror("SCRPD-START: exec");
        _exit(127);
    }

    FILE *pid_file = fopen(pid_path, "w");

    if (pid_file == NULL) {
        perror("SCRPD-START: cannot create PID file");
        return 1;
    }

    fprintf(
        pid_file,
        "%ld\n",
        (long)pid
    );

    if (fclose(pid_file) != 0) {
        perror("SCRPD-START: cannot close PID file");
        return 1;
    }

    printf("SCRPD-START: service\n");
    printf("  Name:    %s\n", service.name);
    printf("  Command: %s\n", service.command);
    printf("  Root:    %s\n", root);
    printf("  Exec:    %s\n", executable);
    printf("  PID:     %ld\n", (long)pid);
    printf("SCRPD-START: service started\n");

    return 0;
}
