#ifndef SCRPD_RUNTIME_STATE_H
#define SCRPD_RUNTIME_STATE_H

#include "scrpd_runtime.h"

int scrpd_runtime_state_save(
    const scrpd_runtime_t *runtime,
    const char *state_directory
);

int scrpd_runtime_state_load(
    scrpd_runtime_t *runtime,
    const char *state_directory
);

int scrpd_runtime_state_clear(
    const scrpd_runtime_t *runtime,
    const char *state_directory
);

#endif
