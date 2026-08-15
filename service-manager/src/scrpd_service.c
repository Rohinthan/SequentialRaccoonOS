#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scrpd_service.h"

static char *trim(char *text)
{
    while (isspace((unsigned char)*text))
        text++;

    char *end = text + strlen(text);

    while (end > text &&
           isspace((unsigned char)end[-1])) {
        end--;
    }

    *end = '\0';

    return text;
}

int scrpd_service_load(
    const char *path,
    scrpd_service_t *service
)
{
    if (path == NULL || service == NULL)
        return -1;

    memset(service, 0, sizeof(*service));

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        perror("SCRPD: cannot open service");
        return -1;
    }

    char line[1024];
    int found_name = 0;
    int found_command = 0;

    while (fgets(line, sizeof(line), file) != NULL) {

        char *text = trim(line);

        if (*text == '\0' || *text == '#')
            continue;

        char *separator = strchr(text, '=');

        if (separator == NULL) {
            fprintf(stderr,
                    "SCRPD: invalid service line: %s\n",
                    text);

            fclose(file);
            return -1;
        }

        *separator = '\0';

        char *key = trim(text);
        char *value = trim(separator + 1);

        if (strcmp(key, "name") == 0) {

            if (*value == '\0') {
                fprintf(stderr,
                        "SCRPD: service name is empty\n");

                fclose(file);
                return -1;
            }

            if (strlen(value) >=
                SCRPD_SERVICE_NAME_MAX) {

                fprintf(stderr,
                        "SCRPD: service name too long\n");

                fclose(file);
                return -1;
            }

            strcpy(service->name, value);
            found_name = 1;

        } else if (strcmp(key, "command") == 0) {

            if (*value == '\0') {
                fprintf(stderr,
                        "SCRPD: service command is empty\n");

                fclose(file);
                return -1;
            }

            if (strlen(value) >=
                SCRPD_SERVICE_COMMAND_MAX) {

                fprintf(stderr,
                        "SCRPD: service command too long\n");

                fclose(file);
                return -1;
            }

            strcpy(service->command, value);
            found_command = 1;

        } else {

            fprintf(stderr,
                    "SCRPD: unknown service key: %s\n",
                    key);

            fclose(file);
            return -1;
        }
    }

    fclose(file);

    if (!found_name) {
        fprintf(stderr,
                "SCRPD: missing name\n");
        return -1;
    }

    if (!found_command) {
        fprintf(stderr,
                "SCRPD: missing command\n");
        return -1;
    }

    return 0;
}

