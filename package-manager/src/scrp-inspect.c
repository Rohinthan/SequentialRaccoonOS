#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <inttypes.h>

#include "../include/scrp_package_format.h"

static void print_usage(void)
{
    printf("SCRP Package Inspector\n\n");

    printf("Usage:\n");
    printf("  scrp-inspect <package-file>\n\n");

    printf("Example:\n");
    printf("  scrp-inspect package-manager/repository/hello-0.1.0-x86_64.scrp\n");
}

static int validate_string_field(
    const char *field,
    size_t size
)
{
    return memchr(field, '\0', size) != NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return 1;
    }

    const char *package_file = argv[1];

    FILE *file = fopen(package_file, "rb");

    if (file == NULL) {
        perror("SCRP-INSPECT: cannot open package");
        return 1;
    }

    struct stat st;

    if (stat(package_file, &st) != 0) {
        perror("SCRP-INSPECT: cannot stat package");
        fclose(file);
        return 1;
    }

    if ((unsigned long long)st.st_size <
        SCRP_PACKAGE_HEADER_SIZE) {

        printf("SCRP-INSPECT: package is too small.\n");
        fclose(file);
        return 1;
    }

    scrp_package_header_t header;

    size_t bytes_read = fread(
        &header,
        1,
        sizeof(header),
        file
    );

    fclose(file);

    if (bytes_read != sizeof(header)) {
        printf("SCRP-INSPECT: cannot read package header.\n");
        return 1;
    }

    printf("SCRP package inspection\n\n");

    if (memcmp(
            header.magic,
            SCRP_PACKAGE_MAGIC,
            4
        ) != 0) {

        printf("Magic:        INVALID\n");
        printf("Status:       INVALID PACKAGE\n");

        return 1;
    }

    printf("Magic:        SCRP\n");

    if (header.format_version !=
        SCRP_PACKAGE_FORMAT_VERSION) {

        printf(
            "Format:       %u (unsupported)\n",
            header.format_version
        );

        printf("Status:       INVALID PACKAGE\n");

        return 1;
    }

    printf(
        "Format:       %u\n",
        header.format_version
    );

    if (!validate_string_field(
            header.name,
            sizeof(header.name)) ||
        !validate_string_field(
            header.version,
            sizeof(header.version)) ||
        !validate_string_field(
            header.architecture,
            sizeof(header.architecture))) {

        printf("Status:       INVALID PACKAGE\n");
        printf("Reason:       invalid string field\n");

        return 1;
    }

    printf(
        "Name:         %s\n",
        header.name
    );

    printf(
        "Version:      %s\n",
        header.version
    );

    printf(
        "Architecture: %s\n",
        header.architecture
    );

    printf(
        "Payload:      %" PRIu64 " bytes\n",
        header.payload_size
    );

    unsigned long long expected_size =
        (unsigned long long)SCRP_PACKAGE_HEADER_SIZE +
        header.payload_size;

    printf(
        "Package:      %" PRIu64 " bytes\n",
        (uint64_t)st.st_size
    );

    if ((unsigned long long)st.st_size != expected_size) {

        printf("Status:       INVALID PACKAGE\n");
        printf(
            "Reason:       size mismatch "
            "(expected %" PRIu64 " bytes)\n",
            (uint64_t)expected_size
        );

        return 1;
    }

    printf("\n");
    printf("Status:       VALID\n");

    return 0;
}
