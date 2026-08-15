#ifndef SCRP_PACKAGE_FORMAT_H
#define SCRP_PACKAGE_FORMAT_H

#include <stdint.h>

#define SCRP_PACKAGE_MAGIC "SCRP"
#define SCRP_PACKAGE_FORMAT_VERSION 1

#define SCRP_PACKAGE_NAME_MAX       128
#define SCRP_PACKAGE_VERSION_MAX    64
#define SCRP_PACKAGE_ARCH_MAX       32

#define SCRP_PACKAGE_HEADER_SIZE    240

typedef struct {
    char magic[4];
    uint32_t format_version;

    char name[SCRP_PACKAGE_NAME_MAX];
    char version[SCRP_PACKAGE_VERSION_MAX];
    char architecture[SCRP_PACKAGE_ARCH_MAX];

    uint64_t payload_size;
} scrp_package_header_t;

#endif
