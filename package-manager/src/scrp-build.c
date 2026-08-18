#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/scrp_package_format.h"

static void usage(const char *program)
{
    printf("SCRP Package Builder\n\n");
    printf("Usage:\n");
    printf("  %s <package-directory> <output-file>\n\n", program);
    printf("Example:\n");
    printf("  %s package-manager/packages/test/hello "
           "package-manager/repository/hello-0.1.0-x86_64.scrp\n",
           program);
}

static int read_metadata(
    const char *package_dir,
    scrp_package_header_t *header
)
{
    char path[512];

    snprintf(
        path,
        sizeof(path),
        "%s/metadata",
        package_dir
    );

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        perror("SCRP-BUILD: cannot open metadata");
        return -1;
    }

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {

        char *equals = strchr(line, '=');

        if (equals == NULL)
            continue;

        *equals = '\0';

        char *key = line;
        char *value = equals + 1;

        value[strcspn(value, "\r\n")] = '\0';

        if (strcmp(key, "name") == 0) {

            strncpy(
                header->name,
                value,
                SCRP_PACKAGE_NAME_MAX - 1
            );

        } else if (strcmp(key, "version") == 0) {

            strncpy(
                header->version,
                value,
                SCRP_PACKAGE_VERSION_MAX - 1
            );

        } else if (strcmp(key, "architecture") == 0) {

            strncpy(
                header->architecture,
                value,
                SCRP_PACKAGE_ARCH_MAX - 1
            );
        }
    }

    fclose(file);

    if (header->name[0] == '\0' ||
        header->version[0] == '\0' ||
        header->architecture[0] == '\0') {

        fprintf(
            stderr,
            "SCRP-BUILD: incomplete package metadata\n"
        );

        return -1;
    }

    return 0;
}

static int calculate_payload_size(const char *payload)
{
    char command[1024];

    snprintf(
        command,
        sizeof(command),
        "tar -cf - -C '%s' . | wc -c",
        payload
    );

    FILE *pipe = popen(command, "r");

    if (pipe == NULL) {
        perror("SCRP-BUILD: cannot calculate payload size");
        return -1;
    }

    unsigned long long size = 0;

    if (fscanf(pipe, "%llu", &size) != 1) {
        pclose(pipe);

        fprintf(
            stderr,
            "SCRP-BUILD: failed to calculate payload size\n"
        );

        return -1;
    }

    pclose(pipe);

    if (size > UINT64_MAX) {
        fprintf(
            stderr,
            "SCRP-BUILD: payload is too large\n"
        );

        return -1;
    }

    return (int)size;
}

static int write_package(
    const char *payload,
    const char *output,
    scrp_package_header_t *header
)
{
    FILE *file = fopen(output, "wb");

    if (file == NULL) {
        perror("SCRP-BUILD: cannot create package");
        return -1;
    }

    if (fwrite(
            header,
            sizeof(*header),
            1,
            file
        ) != 1) {

        perror("SCRP-BUILD: cannot write header");
        fclose(file);
        return -1;
    }

    char command[1024];

    snprintf(
        command,
        sizeof(command),
        "tar -cf - -C '%s' .",
        payload
    );

    FILE *pipe = popen(command, "r");

    if (pipe == NULL) {
        perror("SCRP-BUILD: cannot create payload");
        fclose(file);
        return -1;
    }

    unsigned char buffer[8192];

    size_t bytes;

    while ((bytes = fread(
                buffer,
                1,
                sizeof(buffer),
                pipe
            )) > 0) {

        if (fwrite(
                buffer,
                1,
                bytes,
                file
            ) != bytes) {

            perror("SCRP-BUILD: payload write failed");
            pclose(pipe);
            fclose(file);
            return -1;
        }
    }

    pclose(pipe);
    fclose(file);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    const char *package_dir = argv[1];
    const char *output = argv[2];

    struct stat st;

    if (stat(package_dir, &st) != 0 ||
        !S_ISDIR(st.st_mode)) {

        fprintf(
            stderr,
            "SCRP-BUILD: invalid package directory: %s\n",
            package_dir
        );

        return 1;
    }

    char payload[512];

    snprintf(
        payload,
        sizeof(payload),
        "%s/payload",
        package_dir
    );

    if (stat(payload, &st) != 0 ||
        !S_ISDIR(st.st_mode)) {

        fprintf(
            stderr,
            "SCRP-BUILD: payload directory not found: %s\n",
            payload
        );

        return 1;
    }

    scrp_package_header_t header;

    memset(&header, 0, sizeof(header));

    memcpy(
        header.magic,
        SCRP_PACKAGE_MAGIC,
        sizeof(header.magic)
    );

    header.format_version =
        SCRP_PACKAGE_FORMAT_VERSION;

    if (read_metadata(
            package_dir,
            &header
        ) != 0) {

        return 1;
    }

    printf("SCRP-BUILD: package\n");
    printf("  Name:         %s\n", header.name);
    printf("  Version:      %s\n", header.version);
    printf("  Architecture: %s\n", header.architecture);

    printf("SCRP-BUILD: calculating payload size...\n");

    int payload_size = calculate_payload_size(payload);

    if (payload_size < 0)
        return 1;

    header.payload_size =
        (uint64_t)(unsigned int)payload_size;

    printf(
        "  Payload:      %llu bytes\n",
        (unsigned long long)header.payload_size
    );

    if (sizeof(header) != SCRP_PACKAGE_HEADER_SIZE) {

        fprintf(
            stderr,
            "SCRP-BUILD: header size mismatch\n"
        );

        return 1;
    }

    printf("SCRP-BUILD: creating package...\n");

    if (write_package(
            payload,
            output,
            &header
        ) != 0) {

        return 1;
    }

    printf("SCRP-BUILD: package created:\n");
    printf("  %s\n", output);

    return 0;
}
