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
    printf("SCRP Package Installer\n\n");

    printf("Usage:\n");
    printf("  scrp-install <package-file> <root-directory>\n\n");

    printf("Example:\n");
    printf("  scrp-install package.scrp /tmp/scrp-root\n");
}

static int create_directory_recursive(const char *path)
{
    char buffer[4096];

    if (strlen(path) >= sizeof(buffer)) {
        fprintf(stderr,
                "SCRP-INSTALL: path too long\n");
        return -1;
    }

    strcpy(buffer, path);

    for (char *p = buffer + 1; *p != '\0'; p++) {

        if (*p != '/')
            continue;

        *p = '\0';

        if (mkdir(buffer, 0755) != 0 &&
            errno != EEXIST) {

            perror("SCRP-INSTALL: mkdir");
            return -1;
        }

        *p = '/';
    }

    if (mkdir(buffer, 0755) != 0 &&
        errno != EEXIST) {

        perror("SCRP-INSTALL: mkdir");
        return -1;
    }

    return 0;
}

static int validate_package(
    FILE *file,
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
                "SCRP-INSTALL: invalid package magic\n");
        return -1;
    }

    if (header->format_version !=
        SCRP_PACKAGE_FORMAT_VERSION) {

        fprintf(stderr,
                "SCRP-INSTALL: unsupported package format: %u\n",
                header->format_version);

        return -1;
    }

    unsigned long long expected_size =
        (unsigned long long)SCRP_PACKAGE_HEADER_SIZE +
        header->payload_size;

    if (file_size != expected_size) {

        fprintf(stderr,
                "SCRP-INSTALL: package size mismatch\n");

        fprintf(stderr,
                "  Expected: %" PRIu64 " bytes\n",
                (uint64_t)expected_size);

        fprintf(stderr,
                "  Actual:   %" PRIu64 " bytes\n",
                (uint64_t)file_size);

        return -1;
    }

    /*
     * Rewind to the beginning. The caller will read the
     * package again when extraction begins.
     */
    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("SCRP-INSTALL: cannot rewind package");
        return -1;
    }

    return 0;
}

static int write_package_file_list(
    const char *package_file,
    const char *root,
    const scrp_package_header_t *header
)
{
    char files_dir[4096];
    char files_path[4096];
    char files_tmp[4096];
    char command[8192];

    int written = snprintf(
        files_dir,
        sizeof(files_dir),
        "%s/var/lib/scrp/files",
        root
    );

    if (written < 0 ||
        (size_t)written >= sizeof(files_dir)) {

        fprintf(stderr,
                "SCRP-INSTALL: ownership directory path too long\n");
        return -1;
    }

    if (create_directory_recursive(files_dir) != 0)
        return -1;

    written = snprintf(
        files_path,
        sizeof(files_path),
        "%s/%s",
        files_dir,
        header->name
    );

    if (written < 0 ||
        (size_t)written >= sizeof(files_path)) {

        fprintf(stderr,
                "SCRP-INSTALL: ownership path too long\n");
        return -1;
    }

    written = snprintf(
        files_tmp,
        sizeof(files_tmp),
        "%s/%s.tmp",
        files_dir,
        header->name
    );

    if (written < 0 ||
        (size_t)written >= sizeof(files_tmp)) {

        fprintf(stderr,
                "SCRP-INSTALL: temporary ownership path too long\n");
        return -1;
    }

    written = snprintf(
        command,
        sizeof(command),
        "tail -c +%d \"%s\" | tar -tf -",
        SCRP_PACKAGE_HEADER_SIZE + 1,
        package_file
    );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        fprintf(stderr,
                "SCRP-INSTALL: ownership command too long\n");
        return -1;
    }

    FILE *pipe = popen(command, "r");

    if (pipe == NULL) {
        perror("SCRP-INSTALL: cannot inspect package payload");
        return -1;
    }

    FILE *output = fopen(files_tmp, "w");

    if (output == NULL) {
        perror("SCRP-INSTALL: cannot create ownership file");
        pclose(pipe);
        return -1;
    }

    char line[4096];

    while (fgets(line, sizeof(line), pipe) != NULL) {

        size_t length = strlen(line);

        while (length > 0 &&
               (line[length - 1] == '\n' ||
                line[length - 1] == '\r')) {

            line[--length] = '\0';
        }

        if (length == 0)
            continue;

        /*
         * Directories end with '/' in tar listings.
         * Only record actual files.
         */
        if (line[length - 1] == '/')
            continue;

        if (strncmp(line, "./", 2) == 0)
            fprintf(output, "/%s\n", line + 2);
        else
            fprintf(output, "/%s\n", line);
    }

    int status = pclose(pipe);

    if (status != 0) {
        fprintf(stderr,
                "SCRP-INSTALL: failed to inspect package payload\n");
        fclose(output);
        remove(files_tmp);
        return -1;
    }

    if (fclose(output) != 0) {
        perror("SCRP-INSTALL: cannot close ownership file");
        remove(files_tmp);
        return -1;
    }

    if (rename(files_tmp, files_path) != 0) {
        perror("SCRP-INSTALL: cannot replace ownership file");
        remove(files_tmp);
        return -1;
    }

    return 0;
}


static int extract_payload(
    const char *package_file,
    const char *staging_root
)
{
    char command[8192];

    int written = snprintf(
        command,
        sizeof(command),
        "mkdir -p \"%s\" && "
        "tail -c +%d \"%s\" | tar -x -C \"%s\"",
        staging_root,
        SCRP_PACKAGE_HEADER_SIZE + 1,
        package_file,
        staging_root
    );

    if (written < 0 ||
        (size_t)written >= sizeof(command)) {

        fprintf(stderr,
                "SCRP-INSTALL: extraction command too long\n");
        return -1;
    }

    int result = system(command);

    if (result != 0) {
        fprintf(stderr,
                "SCRP-INSTALL: payload extraction failed\n");
        return -1;
    }

    return 0;
}

static int install_database_entry(
    const char *root,
    const scrp_package_header_t *header
)
{
    char database_dir[4096];
    char database_path[4096];
    char database_tmp[4096];

    int written = snprintf(
        database_dir,
        sizeof(database_dir),
        "%s/var/lib/scrp",
        root
    );

    if (written < 0 ||
        (size_t)written >= sizeof(database_dir)) {

        fprintf(stderr,
                "SCRP-INSTALL: database path too long\n");
        return -1;
    }

    if (create_directory_recursive(database_dir) != 0)
        return -1;

    written = snprintf(
        database_path,
        sizeof(database_path),
        "%s/database",
        database_dir
    );

    if (written < 0 ||
        (size_t)written >= sizeof(database_path)) {

        fprintf(stderr,
                "SCRP-INSTALL: database path too long\n");
        return -1;
    }

    written = snprintf(
        database_tmp,
        sizeof(database_tmp),
        "%s/database.tmp",
        database_dir
    );

    if (written < 0 ||
        (size_t)written >= sizeof(database_tmp)) {

        fprintf(stderr,
                "SCRP-INSTALL: temporary database path too long\n");
        return -1;
    }

    FILE *input = fopen(database_path, "r");

    if (input == NULL && errno != ENOENT) {
        perror("SCRP-INSTALL: cannot open package database");
        return -1;
    }

    FILE *output = fopen(database_tmp, "w");

    if (output == NULL) {
        if (input != NULL)
            fclose(input);

        perror("SCRP-INSTALL: cannot create temporary database");
        return -1;
    }

    char line[512];
    int found = 0;

    if (input != NULL) {

        while (fgets(line, sizeof(line), input) != NULL) {

            if (line[0] == '#' || line[0] == '\n') {
                fputs(line, output);
                continue;
            }

            char package_name[128];

            if (sscanf(
                    line,
                    "%127[^|]",
                    package_name
                ) == 1) {

                if (strcmp(package_name, header->name) == 0) {

                    if (!found) {
                        fprintf(
                            output,
                            "%s|%s|installed\n",
                            header->name,
                            header->version
                        );

                        found = 1;
                    }

                    continue;
                }
            }

            fputs(line, output);
        }

        fclose(input);
    }

    if (!found) {
        fprintf(
            output,
            "%s|%s|installed\n",
            header->name,
            header->version
        );
    }

    if (fclose(output) != 0) {
        perror("SCRP-INSTALL: cannot close temporary database");
        remove(database_tmp);
        return -1;
    }

    if (rename(database_tmp, database_path) != 0) {
        perror("SCRP-INSTALL: cannot replace package database");
        remove(database_tmp);
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
    const char *root = argv[2];

    FILE *file = fopen(package_file, "rb");

    if (file == NULL) {
        perror("SCRP-INSTALL: cannot open package");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("SCRP-INSTALL: cannot seek");
        fclose(file);
        return 1;
    }

    long size = ftell(file);

    if (size < 0) {
        perror("SCRP-INSTALL: cannot determine package size");
        fclose(file);
        return 1;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("SCRP-INSTALL: cannot seek");
        fclose(file);
        return 1;
    }

    unsigned long long file_size =
        (unsigned long long)size;

    if (file_size < SCRP_PACKAGE_HEADER_SIZE) {
        fprintf(stderr,
                "SCRP-INSTALL: package is too small\n");
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
                "SCRP-INSTALL: cannot read package header\n");

        fclose(file);
        return 1;
    }

    if (validate_package(
            file,
            &header,
            file_size
        ) != 0) {

        fclose(file);
        return 1;
    }

    fclose(file);

    printf("SCRP-INSTALL: package\n");
    printf("  Name:         %s\n", header.name);
    printf("  Version:      %s\n", header.version);
    printf("  Architecture: %s\n", header.architecture);
    printf("  Payload:      %" PRIu64 " bytes\n",
           (uint64_t)header.payload_size);

    printf("SCRP-INSTALL: package validated\n");

    printf("SCRP-INSTALL: staging root:\n");
    printf("  %s\n", root);

    if (create_directory_recursive(root) != 0)
        return 1;

    printf("SCRP-INSTALL: extracting payload...\n");

    if (extract_payload(
            package_file,
            root
        ) != 0) {

        return 1;
    }

    printf("SCRP-INSTALL: payload extracted\n");

   if (write_package_file_list(
	   package_file,
	   root,
	   &header
	) != 0) {

	return 1;
}


    printf("SCRP-INSTALL: package ownership recorded\n");



    printf("SCRP-INSTALL: updating package database...\n");

    if (install_database_entry(
            root,
            &header
        ) != 0) {

        return 1;
    }

    printf("SCRP-INSTALL: package installed successfully\n");

    printf("\n");
    printf("Package:\n");
    printf("  %s-%s-%s\n",
           header.name,
           header.version,
           header.architecture);

    printf("Root:\n");
    printf("  %s\n", root);

    return 0;
}
