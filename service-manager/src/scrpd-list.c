#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scrpd_service.h"

static void print_usage(void)
{
    printf("SCRPD Service Lister\n\n");

    printf("Usage:\n");
    printf("  scrpd-list <service-directory>\n\n");

    printf("Example:\n");
    printf("  scrpd-list service-manager/services\n");
}

static int has_service_suffix(const char *name)
{
    const char *suffix = ".service";
    size_t name_len = strlen(name);
    size_t suffix_len = strlen(suffix);

    if (name_len < suffix_len)
        return 0;

    return strcmp(
        name + name_len - suffix_len,
        suffix
    ) == 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return 1;
    }

    const char *service_dir = argv[1];

    DIR *directory = opendir(service_dir);

    if (directory == NULL) {
        perror("SCRPD-LIST: cannot open service directory");
        return 1;
    }

    printf("SCRPD SERVICES\n\n");

    struct dirent *entry;
    int found = 0;
    int invalid = 0;

    while ((entry = readdir(directory)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (!has_service_suffix(entry->d_name))
            continue;

        char path[4096];

        int written = snprintf(
            path,
            sizeof(path),
            "%s/%s",
            service_dir,
            entry->d_name
        );

        if (written < 0 ||
            (size_t)written >= sizeof(path)) {

            fprintf(
                stderr,
                "SCRPD-LIST: service path too long: %s\n",
                entry->d_name
            );

            invalid = 1;
            continue;
        }

        scrpd_service_t service;

        if (scrpd_service_load(path, &service) != 0) {
            fprintf(
                stderr,
                "SCRPD-LIST: invalid service: %s\n",
                entry->d_name
            );

            invalid = 1;
            continue;
        }

        printf("%s\n", service.name);
        found = 1;
    }

    closedir(directory);

    if (!found)
        printf("(none)\n");

    return invalid ? 1 : 0;
}

