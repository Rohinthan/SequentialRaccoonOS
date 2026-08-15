#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_usage(void)
{
    printf("SCRP Package Remover\n\n");

    printf("Usage:\n");
    printf("  scrp-remove <package-name> <root-directory>\n\n");

    printf("Example:\n");
    printf("  scrp-remove hello /tmp/scrp-root\n");
}

static int remove_empty_parent_directories(
    const char *root,
    const char *relative_path
)
{
    char path[4096];

    int written = snprintf(
        path,
        sizeof(path),
        "%s/%s",
        root,
        relative_path
    );

    if (written < 0 ||
        (size_t)written >= sizeof(path)) {

        fprintf(stderr,
                "SCRP-REMOVE: path too long\n");
        return -1;
    }

    char *slash = strrchr(path, '/');

    while (slash != NULL) {

        *slash = '\0';

        /*
         * Never remove anything above the package root.
         */
        if (strcmp(path, root) == 0)
            break;

        if (rmdir(path) != 0) {

            if (errno == ENOTEMPTY ||
                errno == EEXIST ||
                errno == ENOENT) {
                break;
            }

            perror("SCRP-REMOVE: rmdir");
            return -1;
        }

        slash = strrchr(path, '/');
    }

    return 0;
}

static int remove_owned_files(
    const char *root,
    const char *ownership_path
)
{
    FILE *ownership = fopen(ownership_path, "r");

    if (ownership == NULL) {
        if (errno == ENOENT) {
            fprintf(stderr,
                    "SCRP-REMOVE: ownership record not found\n");
        } else {
            perror("SCRP-REMOVE: cannot open ownership record");
        }

        return -1;
    }

    char line[4096];

    while (fgets(line, sizeof(line), ownership) != NULL) {

        size_t length = strlen(line);

        while (length > 0 &&
               (line[length - 1] == '\n' ||
                line[length - 1] == '\r')) {

            line[--length] = '\0';
        }

        if (length == 0)
            continue;

        /*
         * Ownership entries must be absolute paths
         * inside the supplied root.
         */
        if (line[0] != '/') {
            fprintf(stderr,
                    "SCRP-REMOVE: invalid ownership path: %s\n",
                    line);
            fclose(ownership);
            return -1;
        }

        /*
         * Convert /usr/bin/hello into
         * <root>/usr/bin/hello.
         */
        char target[8192];

        int written = snprintf(
            target,
            sizeof(target),
            "%s%s",
            root,
            line
        );

        if (written < 0 ||
            (size_t)written >= sizeof(target)) {

            fprintf(stderr,
                    "SCRP-REMOVE: target path too long\n");
            fclose(ownership);
            return -1;
        }

        if (remove(target) != 0) {

            if (errno == ENOENT) {
                printf("SCRP-REMOVE: already absent: %s\n",
                       line);
            } else {
                perror("SCRP-REMOVE: cannot remove file");
                fclose(ownership);
                return -1;
            }
        } else {
            printf("SCRP-REMOVE: removed %s\n", line);
        }

        if (remove_empty_parent_directories(
                root,
                line + 1
            ) != 0) {

            fclose(ownership);
            return -1;
        }
    }

    fclose(ownership);

    return 0;
}

static int remove_database_entry(
    const char *database_path,
    const char *database_tmp,
    const char *package_name
)
{
    FILE *input = fopen(database_path, "r");

    if (input == NULL) {

        if (errno == ENOENT) {
            return 0;
        }

        perror("SCRP-REMOVE: cannot open package database");
        return -1;
    }

    FILE *output = fopen(database_tmp, "w");

    if (output == NULL) {
        fclose(input);

        perror("SCRP-REMOVE: cannot create temporary database");
        return -1;
    }

    char line[512];

    while (fgets(line, sizeof(line), input) != NULL) {

        char name[128];

        if (sscanf(
                line,
                "%127[^|]",
                name
            ) == 1) {

            if (strcmp(name, package_name) == 0)
                continue;
        }

        fputs(line, output);
    }

    fclose(input);

    if (fclose(output) != 0) {
        perror("SCRP-REMOVE: cannot close temporary database");
        remove(database_tmp);
        return -1;
    }

    if (rename(database_tmp, database_path) != 0) {
        perror("SCRP-REMOVE: cannot replace package database");
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

    const char *package_name = argv[1];
    const char *root = argv[2];

    char database_dir[4096];
    char database_path[4096];
    char database_tmp[4096];
    char ownership_path[4096];

    int written = snprintf(
        database_dir,
        sizeof(database_dir),
        "%s/var/lib/scrp",
        root
    );

    if (written < 0 ||
        (size_t)written >= sizeof(database_dir)) {

        fprintf(stderr,
                "SCRP-REMOVE: database path too long\n");
        return 1;
    }

    written = snprintf(
        database_path,
        sizeof(database_path),
        "%s/database",
        database_dir
    );

    if (written < 0 ||
        (size_t)written >= sizeof(database_path)) {

        fprintf(stderr,
                "SCRP-REMOVE: database path too long\n");
        return 1;
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
                "SCRP-REMOVE: temporary database path too long\n");
        return 1;
    }

    written = snprintf(
        ownership_path,
        sizeof(ownership_path),
        "%s/files/%s",
        database_dir,
        package_name
    );

    if (written < 0 ||
        (size_t)written >= sizeof(ownership_path)) {

        fprintf(stderr,
                "SCRP-REMOVE: ownership path too long\n");
        return 1;
    }

    printf("SCRP-REMOVE: package\n");
    printf("  Name:         %s\n", package_name);
    printf("  Root:         %s\n", root);

    printf("SCRP-REMOVE: reading ownership record...\n");

    if (remove_owned_files(
            root,
            ownership_path
        ) != 0) {

        return 1;
    }

    printf("SCRP-REMOVE: removing ownership record...\n");

    if (remove(ownership_path) != 0) {

        if (errno != ENOENT) {
            perror("SCRP-REMOVE: cannot remove ownership record");
            return 1;
        }
    }

    printf("SCRP-REMOVE: updating package database...\n");

    if (remove_database_entry(
            database_path,
            database_tmp,
            package_name
        ) != 0) {

        return 1;
    }

    printf("SCRP-REMOVE: package removed successfully\n");

    return 0;
}
