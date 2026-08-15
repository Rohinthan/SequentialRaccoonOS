#include <stdio.h>

#include "scrpd_service.h"

int main(int argc, char *argv[])
{
    const char *path =
        "service-manager/services/hello.service";

    if (argc == 2)
        path = argv[1];

    if (argc > 2) {
        fprintf(stderr,
                "Usage: scrpd-service-test [service-file]\n");
        return 1;
    }

    scrpd_service_t service;

    if (scrpd_service_load(path, &service) != 0) {

        fprintf(stderr,
                "SCRPD SERVICE TEST: INVALID\n");

        return 1;
    }

    printf("SCRPD SERVICE TEST\n");
    printf("Name:    %s\n", service.name);
    printf("Command: %s\n", service.command);
    printf("Status:  VALID\n");

    return 0;
}

