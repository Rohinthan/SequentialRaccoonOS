#ifndef SCRPD_RUNTIME_H
#define SCRPD_RUNTIME_H

#include "scrpd_supervisor.h"

typedef struct {
    scrpd_service_runtime_t runtime;
} scrpd_runtime_t;

void scrpd_runtime_init(
    scrpd_runtime_t *runtime,
    const char *name
);

int scrpd_runtime_set_pid(
    scrpd_runtime_t *runtime,
    long pid
);

int scrpd_runtime_set_state(
    scrpd_runtime_t *runtime,
    scrpd_service_state_t state
);

int scrpd_runtime_is_alive(
    const scrpd_runtime_t *runtime
);

int scrpd_supervisor_reconcile(
    scrpd_runtime_t *runtime
);

#endif
