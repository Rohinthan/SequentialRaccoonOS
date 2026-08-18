#ifndef SCRPD_SUPERVISED_START_H
#define SCRPD_SUPERVISED_START_H

#include "scrpd_runtime.h"
#include "scrpd_service.h"

int scrpd_supervised_start(
    scrpd_runtime_t *runtime,
    const scrpd_service_t *service,
    const char *root
);

#endif

