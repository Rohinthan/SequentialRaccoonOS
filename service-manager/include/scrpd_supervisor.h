#ifndef SCRPD_SUPERVISOR_H
#define SCRPD_SUPERVISOR_H

#define SCRPD_SERVICE_STATE_NAME_MAX 128

typedef enum {
    SCRPD_STATE_STOPPED = 0,
    SCRPD_STATE_STARTING,
    SCRPD_STATE_RUNNING,
    SCRPD_STATE_STOPPING,
    SCRPD_STATE_FAILED
} scrpd_service_state_t;

typedef struct {
    char name[SCRPD_SERVICE_STATE_NAME_MAX];
    scrpd_service_state_t state;
    long pid;
} scrpd_service_runtime_t;

const char *scrpd_state_name(
    scrpd_service_state_t state
);

int scrpd_state_is_active(
    scrpd_service_state_t state
);

#endif
