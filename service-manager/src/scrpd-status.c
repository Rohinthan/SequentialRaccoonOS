#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scrpd_service.h"

static void print_usage(void)
{
    printf("SCRPD Service Status\n\n");

    printf("Usage:\n");
    printf("  scrpd-status <root-directory> <service-file>\n\n");

    printf("Example:\n");
    printf("  scrpd-status /tmp/scrp-root "
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
                "SCRPD-STATUS: invalid service\n");
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
                "SCRPD-STATUS: executable path too long\n");
        return 1;
    }

    struct stat st;

    int available =
        stat(executable, &st) == 0 &&
        S_ISREG(st.st_mode);

    printf("SCRPD STATUS\n\n");

    printf("Name:       %s\n", service.name);
    printf("Command:    %s\n", service.command);
    printf("Root:       %s\n", root);
    printf("Executable: %s\n", executable);

    if (available)
        printf("Status:     AVAILABLE\n");
    else
        printf("Status:     MISSING\n");

    return available ? 0 : 1;
}
