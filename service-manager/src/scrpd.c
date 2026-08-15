#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void print_usage(void)
{
    printf("Sequential Raccoon Service Manager\n\n");

    printf("Usage:\n");
    printf("  scrpd list <services-directory>\n");
    printf("  scrpd status <root-directory> <service-file>\n");
    printf("  scrpd start <root-directory> <service-file> <state-directory>\n");
    printf("  scrpd stop <service-name> <state-directory>\n");
    printf("  scrpd restart <root-directory> <service-file> <state-directory>\n");
}

static int command_list(const char *directory)
{
    char command[4096];

    int written = snprintf(
        command,
        sizeof(command),
        "./service-manager/scrpd-list \"%s\"",
        directory
    );

    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr,
                "SCRPD: command path too long\n");
        return 1;
    }

    return system(command);
}

static int command_status(
    const char *root,
    const char *service
)
{
    char command[4096];

    int written = snprintf(
        command,
        sizeof(command),
        "./service-manager/scrpd-status \"%s\" \"%s\"",
        root,
        service
    );

    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr,
                "SCRPD: command path too long\n");
        return 1;
    }

    return system(command);
}

static int command_start(
    const char *root,
    const char *service,
    const char *state
)
{
    char command[4096];

    int written = snprintf(
        command,
        sizeof(command),
        "./service-manager/scrpd-start \"%s\" \"%s\" \"%s\"",
        root,
        service,
        state
    );

    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr,
                "SCRPD: command path too long\n");
        return 1;
    }

    return system(command);
}

static int command_stop(
    const char *name,
    const char *state
)
{
    char command[4096];

    int written = snprintf(
        command,
        sizeof(command),
        "./service-manager/scrpd-stop \"%s\" \"%s\"",
        name,
        state
    );

    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr,
                "SCRPD: command path too long\n");
        return 1;
    }

    return system(command);
}

static int command_restart(
    const char *root,
    const char *service,
    const char *state
)
{
    char command[4096];

    int written = snprintf(
        command,
        sizeof(command),
        "./service-manager/scrpd-restart \"%s\" \"%s\" \"%s\"",
        root,
        service,
        state
    );

    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr,
                "SCRPD: command path too long\n");
        return 1;
    }

    return system(command);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {

        if (argc != 3) {
            print_usage();
            return 1;
        }

        return command_list(argv[2]);
    }

    if (strcmp(argv[1], "status") == 0) {

        if (argc != 4) {
            print_usage();
            return 1;
        }

        return command_status(argv[2], argv[3]);
    }

    if (strcmp(argv[1], "start") == 0) {

        if (argc != 5) {
            print_usage();
            return 1;
        }

        return command_start(
            argv[2],
            argv[3],
            argv[4]
        );
    }

    if (strcmp(argv[1], "stop") == 0) {

        if (argc != 4) {
            print_usage();
            return 1;
        }

        return command_stop(
            argv[2],
            argv[3]
        );
    }

    if (strcmp(argv[1], "restart") == 0) {

        if (argc != 5) {
            print_usage();
            return 1;
        }

        return command_restart(
            argv[2],
            argv[3],
            argv[4]
        );
    }

    fprintf(stderr,
            "SCRPD: unknown command: %s\n\n",
            argv[1]);

    print_usage();
    return 1;
}
