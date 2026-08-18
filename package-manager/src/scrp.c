#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/scrp.h"

static void print_version(void)
{
    printf("SCRP - Sequential Raccoon Package Manager\n");
    printf("Version: %s\n", SCRP_VERSION);
}

static void print_help(void)
{
    printf("SCRP - Sequential Raccoon Package Manager\n\n");

    printf("Usage:\n");
    printf("  scrp [option] [package]\n\n");

    printf("Options:\n");
    printf("  -S,  --sync       Install package\n");
    printf("  -R,  --remove     Remove package\n");
    printf("  -Q,  --query      Query installed packages\n");
    printf("  -Ss, --search     Search packages\n");
    printf("  -Syu              Synchronize and update system\n");
    printf("  -v,  --version    Show version\n");
    printf("  -h,  --help       Show this help\n");
}


/* =========================================================
 * Database
 * ========================================================= */

int database_init(void)
{
    struct stat st;

    if (stat("/var/lib/scrp", &st) != 0) {

        if (mkdir("/var/lib/scrp", 0755) != 0) {
            perror("SCRP: cannot create database directory");
            return -1;
        }
    }

    FILE *file = fopen(SCRP_DATABASE, "a");

    if (file == NULL) {
        perror("SCRP: cannot open package database");
        return -1;
    }

    fclose(file);

    return 0;
}


/* =========================================================
 * Query database
 * ========================================================= */

int database_query(void)
{
    FILE *file = fopen(SCRP_DATABASE, "r");

    if (file == NULL) {
        perror("SCRP: cannot open package database");
        return -1;
    }

    char line[512];
    int found = 0;

    printf("SCRP: installed packages\n");
    printf("\n");

    while (fgets(line, sizeof(line), file) != NULL) {

        if (line[0] == '#' || line[0] == '\n')
            continue;

        char name[128];
        char version[128];
        char status[128];

        if (sscanf(
                line,
                "%127[^|]|%127[^|]|%127s",
                name,
                version,
                status
            ) == 3) {

            printf(
                "%-20s %-12s %s\n",
                name,
                version,
                status
            );

            found = 1;
        }
    }

    fclose(file);

    if (!found)
        printf("SCRP: package database is currently empty.\n");

    return 0;
}


/* =========================================================
 * Add package
 * ========================================================= */

int database_add(
    const char *name,
    const char *version
)
{
    if (name == NULL || version == NULL)
        return -1;

    FILE *file = fopen(SCRP_DATABASE, "a");

    if (file == NULL) {
        perror("SCRP: cannot open package database");
        return -1;
    }

    fprintf(
        file,
        "%s|%s|installed\n",
        name,
        version
    );

    fclose(file);

    return 0;
}


/* =========================================================
 * Remove package
 * ========================================================= */

int database_remove(const char *name)
{
    if (name == NULL)
        return -1;

    FILE *input = fopen(SCRP_DATABASE, "r");

    if (input == NULL) {
        perror("SCRP: cannot open package database");
        return -1;
    }

    FILE *output = fopen(
        "/var/lib/scrp/database.tmp",
        "w"
    );

    if (output == NULL) {
        fclose(input);
        perror("SCRP: cannot create temporary database");
        return -1;
    }

    char line[512];
    int removed = 0;

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

            if (strcmp(package_name, name) == 0) {
                removed = 1;
                continue;
            }
        }

        fputs(line, output);
    }

    fclose(input);
    fclose(output);

    if (rename(
            "/var/lib/scrp/database.tmp",
            SCRP_DATABASE
        ) != 0) {

        perror("SCRP: cannot replace package database");
        return -1;
    }

    return removed ? 0 : 1;
}


/* =========================================================
 * Main
 * ========================================================= */

int main(int argc, char *argv[])
{
    if (database_init() != 0)
        return 1;

    if (argc < 2) {
        print_help();
        return 0;
    }


    if (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0) {

        print_version();
        return 0;
    }


    if (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {

        print_help();
        return 0;
    }


    if (strcmp(argv[1], "-Q") == 0 ||
        strcmp(argv[1], "--query") == 0) {

        return database_query();
    }


    if (strcmp(argv[1], "-S") == 0) {

        if (argc < 3) {
            printf("SCRP: no package specified.\n");
            return 1;
        }

        printf(
            "SCRP: package installation is not implemented yet.\n"
        );

        printf(
            "SCRP: requested package: %s\n",
            argv[2]
        );

        return 0;
    }


    if (strcmp(argv[1], "-R") == 0) {

        if (argc < 3) {
            printf("SCRP: no package specified.\n");
            return 1;
        }

        printf(
            "SCRP: package removal is not implemented yet.\n"
        );

        printf(
            "SCRP: requested package: %s\n",
            argv[2]
        );

        return 0;
    }


    if (strcmp(argv[1], "-Ss") == 0) {

        if (argc < 3) {
            printf("SCRP: no search term specified.\n");
            return 1;
        }

        printf(
            "SCRP: package search is not implemented yet.\n"
        );

        printf(
            "SCRP: search term: %s\n",
            argv[2]
        );

        return 0;
    }


    if (strcmp(argv[1], "-Syu") == 0) {

        printf(
            "SCRP: system synchronization is not implemented yet.\n"
        );

        return 0;
    }


    printf(
        "SCRP: unknown option: %s\n",
        argv[1]
    );

    printf(
        "Try 'scrp --help' for more information.\n"
    );

    return 1;
}
