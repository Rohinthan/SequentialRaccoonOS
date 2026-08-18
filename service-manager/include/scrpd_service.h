#ifndef SCRPD_SERVICE_H
#define SCRPD_SERVICE_H

#define SCRPD_SERVICE_NAME_MAX    128
#define SCRPD_SERVICE_COMMAND_MAX 512

typedef struct {
    char name[SCRPD_SERVICE_NAME_MAX];
    char command[SCRPD_SERVICE_COMMAND_MAX];
} scrpd_service_t;

int scrpd_service_load(
    const char *path,
    scrpd_service_t *service
);

#endif

