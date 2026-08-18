#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "scrpd_runtime_state.h"

static int build_state_path(
    char *path,
    size_t path_size,
    const scrpd_runtime_t *runtime,
    const char *state_directory
)
{
    if (path == NULL ||
        runtime == NULL ||
        state_directory == NULL ||
        runtime->runtime.name[0] == '\0') {
        return -1;
    }

    int written = snprintf(
        path,
        path_size,
        "%s/%s.state",
        state_directory,
        runtime->runtime.name
    );

    if (written < 0 ||
        (size_t)written >= path_size) {
        return -1;
    }

    return 0;
}

int scrpd_runtime_state_save(
    const scrpd_runtime_t *runtime,
    const char *state_directory
)
{
    if (runtime == NULL || state_directory == NULL) {
        return -1;
    }

    if (mkdir(state_directory, 0755) != 0 &&
        errno != EEXIST) {
        return -1;
    }

    char path[4096];

    if (build_state_path(
            path,
            sizeof(path),
            runtime,
            state_directory
        ) != 0) {
        return -1;
    }

    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return -1;
    }

    if (fprintf(
            file,
            "name=%s\n"
            "pid=%ld\n"
            "state=%d\n",
            runtime->runtime.name,
            runtime->runtime.pid,
            (int)runtime->runtime.state
        ) < 0) {

        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        return -1;
    }

    return 0;
}

int scrpd_runtime_state_load(
    scrpd_runtime_t *runtime,
    const char *state_directory
)
{
    if (runtime == NULL || state_directory == NULL) {
        return -1;
    }

    char path[4096];

    if (build_state_path(
            path,
            sizeof(path),
            runtime,
            state_directory
        ) != 0) {
        return -1;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return -1;
    }

    char name[SCRPD_SERVICE_STATE_NAME_MAX];
    long pid;
    int state;

    if (fscanf(
            file,
            "name=%127[^\n]\n"
            "pid=%ld\n"
            "state=%d\n",
            name,
            &pid,
            &state
        ) != 3) {

        fclose(file);
        return -1;
    }

    fclose(file);

    if (pid < 0) {
        return -1;
    }

    if (state < SCRPD_STATE_STOPPED ||
        state > SCRPD_STATE_FAILED) {
        return -1;
    }

    strncpy(
        runtime->runtime.name,
        name,
        SCRPD_SERVICE_STATE_NAME_MAX - 1
    );

    runtime->runtime.name[
        SCRPD_SERVICE_STATE_NAME_MAX - 1
    ] = '\0';

    runtime->runtime.pid = pid;
    runtime->runtime.state =
        (scrpd_service_state_t)state;

    return 0;
}

int scrpd_runtime_state_clear(
    const scrpd_runtime_t *runtime,
    const char *state_directory
)
{
    if (runtime == NULL || state_directory == NULL) {
        return -1;
    }

    char path[4096];

    if (build_state_path(
            path,
            sizeof(path),
            runtime,
            state_directory
        ) != 0) {
        return -1;
    }

    if (remove(path) != 0 && errno != ENOENT) {
        return -1;
    }

    return 0;
}
