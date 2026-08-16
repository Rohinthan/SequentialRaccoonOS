#ifndef SCRPD_SUPERVISOR_RECOVER_H
#define SCRPD_SUPERVISOR_RECOVER_H

#include "scrpd_runtime.h"
#include "scrpd_service.h"

int scrpd_supervisor_recover(
    scrpd_runtime_t *runtime,
    const scrpd_service_t *service,
    const char *root
);

#endif
