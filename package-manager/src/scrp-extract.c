#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/scrp_package_format.h"

static void print_usage(void)
{
    printf("SCRP Package Extractor\n\n");

    printf("Usage:\n");
    printf("  scrp-extract <package-file> <output-directory>\n\n");

    printf("Example:\n");
    printf("  scrp-extract package.scrp /tmp/scrp-extract\n");
}

static int create_directory_recursive(const char *path)
{
    char buffer[4096];

    if (strlen(path) >= sizeof(buffer)) {
        fprintf(stderr,
                "SCRP-EXTRACT: path too long\n");
        return -1;
    }

    strcpy(buffer, path);

    for (char *p = buffer + 1; *p != '\0'; p++) {

        if (*p != '/')
            continue;

        *p = '\0';

        if (mkdir(buffer, 0755) != 0 &&
            errno != EEXIST) {

            perror("SCRP-EXTRACT: mkdir");
            return -1;
        }

        *p = '/';
    }

    if (mkdir(buffer, 0755) != 0 &&
        errno != EEXIST) {

        perror("SCRP-EXTRACT: mkdir");
        return -1;
    }

    return 0;
}

static int validate_header(
    const scrp_package_header_t *header,
    unsigned long long file_size
)
{
    if (memcmp(
            header->magic,
            SCRP_PACKAGE_MAGIC,
            4
        ) != 0) {

        fprintf(stderr,
                "SCRP-EXTRACT: invalid package magic\n");
        return -1;
    }

    if (header->format_version !=
        SCRP_PACKAGE_FORMAT_VERSION) {

        fprintf(stderr,
                "SCRP-EXTRACT: unsupported package format: %u\n",
                header->format_version);

        return -1;
    }

    unsigned long long expected_size =
        (unsigned long long)SCRP_PACKAGE_HEADER_SIZE +
        header->payload_size;

    if (file_size != expected_size) {

        fprintf(stderr,
                "SCRP-EXTRACT: package size mismatch\n");

        fprintf(stderr,
                "  Expected: %" PRIu64 " bytes\n",
                (uint64_t)expected_size);

        fprintf(stderr,
                "  Actual:   %" PRIu64 " bytes\n",
                (uint64_t)file_size);

        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        print_usage();
        return 1;
    }

    const char *package_file = argv[1];
    const char *output_dir = argv[2];

    FILE *file = fopen(package_file, "rb");

    if (file == NULL) {
        perror("SCRP-EXTRACT: cannot open package");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("SCRP-EXTRACT: cannot seek");
        fclose(file);
        return 1;
    }

    long size = ftell(file);

    if (size < 0) {
        perror("SCRP-EXTRACT: cannot determine package size");
        fclose(file);
        return 1;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("SCRP-EXTRACT: cannot seek");
        fclose(file);
        return 1;
    }

    unsigned long long file_size =
        (unsigned long long)size;

    if (file_size < SCRP_PACKAGE_HEADER_SIZE) {
        fprintf(stderr,
                "SCRP-EXTRACT: package is too small\n");
        fclose(file);
        return 1;
    }

    scrp_package_header_t header;

    if (fread(
            &header,
            1,
            sizeof(header),
            file
        ) != sizeof(header)) {

        fprintf(stderr,
                "SCRP-EXTRACT: cannot read package header\n");

        fclose(file);
        return 1;
    }

    if (validate_header(&header, file_size) != 0) {
        fclose(file);
        return 1;
    }

    if (create_directory_recursive(output_dir) != 0) {
        fclose(file);
        return 1;
    }

    char payload_path[4096];

    int written = snprintf(
        payload_path,
        sizeof(payload_path),
        "%s/payload",
        output_dir
    );

    if (written < 0 ||
        (size_t)written >= sizeof(payload_path)) {

        fprintf(stderr,
                "SCRP-EXTRACT: output path too long\n");

        fclose(file);
        return 1;
    }

    if (create_directory_recursive(payload_path) != 0) {
        fclose(file);
        return 1;
    }

    char command[8192];

    written = snprintf(
        command,
        sizeof(command),
        "tail -c +%d \"%s\" | tar -x -C \"%s\"",
        SCRP_PACKAGE_HEADER_SIZE + 1,
        package_file,
        payload_path
    );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        fprintf(stderr,
                "SCRP-EXTRACT: command too long\n");

        fclose(file);
        return 1;
    }

    fclose(file);

    printf("SCRP-EXTRACT: package\n");
    printf("  Name:         %s\n", header.name);
    printf("  Version:      %s\n", header.version);
    printf("  Architecture: %s\n", header.architecture);
    printf("  Payload:      %" PRIu64 " bytes\n",
           (uint64_t)header.payload_size);

    printf("SCRP-EXTRACT: validating package...\n");
    printf("SCRP-EXTRACT: package valid\n");

    printf("SCRP-EXTRACT: extracting payload...\n");

    int result = system(command);

    if (result != 0) {
        fprintf(stderr,
                "SCRP-EXTRACT: payload extraction failed\n");
        return 1;
    }

    printf("SCRP-EXTRACT: extraction complete\n");
    printf("  Output:       %s\n", payload_path);

    return 0;
}
