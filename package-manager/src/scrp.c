#include <stdio.h>
#include <string.h>

#define SCRP_VERSION "0.5.0"

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

int main(int argc, char *argv[])
{
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

        printf("SCRP: package database query\n");
        printf("SCRP: package database is currently empty.\n");
        return 0;
    }

    if (strcmp(argv[1], "-S") == 0) {

        if (argc < 3) {
            printf("SCRP: no package specified.\n");
            return 1;
        }

        printf("SCRP: package installation will be implemented.\n");
        printf("SCRP: requested package: %s\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "-R") == 0) {

        if (argc < 3) {
            printf("SCRP: no package specified.\n");
            return 1;
        }

        printf("SCRP: package removal will be implemented.\n");
        printf("SCRP: requested package: %s\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "-Ss") == 0) {

        if (argc < 3) {
            printf("SCRP: no search term specified.\n");
            return 1;
        }

        printf("SCRP: package search will be implemented.\n");
        printf("SCRP: search term: %s\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "-Syu") == 0) {

        printf("SCRP: system synchronization will be implemented.\n");
        return 0;
    }

    printf("SCRP: unknown option: %s\n", argv[1]);
    printf("Try 'scrp --help' for more information.\n");

    return 1;
}
