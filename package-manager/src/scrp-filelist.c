#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCRP_PACKAGE_HEADER_SIZE 240

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr,
                "Usage: scrp-filelist <package-file>\n");
        return 1;
    }

    const char *package_file = argv[1];

    char command[8192];

    int written = snprintf(
        command,
        sizeof(command),
        "tail -c +%d \"%s\" | tar -tf -",
        SCRP_PACKAGE_HEADER_SIZE + 1,
        package_file
    );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        fprintf(stderr,
                "SCRP-FILELIST: command too long\n");
        return 1;
    }

    FILE *pipe = popen(command, "r");

    if (pipe == NULL) {
        perror("SCRP-FILELIST: popen");
        return 1;
    }

    char line[4096];

    printf("SCRP file ownership list\n\n");

    while (fgets(line, sizeof(line), pipe) != NULL) {

        size_t length = strlen(line);

        while (length > 0 &&
               (line[length - 1] == '\n' ||
                line[length - 1] == '\r')) {

            line[--length] = '\0';
        }

        if (length == 0)
            continue;

        /* Ignore directories. */
        if (line[length - 1] == '/')
            continue;

        if (strncmp(line, "./", 2) == 0)
            printf("/%s\n", line + 2);
        else
            printf("/%s\n", line);
    }

    int status = pclose(pipe);

    if (status != 0) {
        fprintf(stderr,
                "SCRP-FILELIST: failed to read package payload\n");
        return 1;
    }

    return 0;
}
