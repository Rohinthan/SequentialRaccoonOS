#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static void print_usage(void)
{
    printf(
        "SCRPD Service Stopper\n\n"
        "Usage:\n"
        "  scrpd-stop <service-name> <state-directory>\n\n"
        "Example:\n"
        "  scrpd-stop hello service-manager/state\n"
    );
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        print_usage();
        return 1;
    }

    const char *service_name = argv[1];
    const char *state_dir = argv[2];

    char pid_path[4096];

    int written = snprintf(
        pid_path,
        sizeof(pid_path),
        "%s/%s.pid",
        state_dir,
        service_name
    );

    if (written < 0 || (size_t)written >= sizeof(pid_path)) {
        fprintf(stderr,
                "SCRPD-STOP: PID path too long\n");
        return 1;
    }

    FILE *file = fopen(pid_path, "r");

    if (file == NULL) {
        if (errno == ENOENT) {
            fprintf(stderr,
                    "SCRPD-STOP: service is not running: %s\n",
                    service_name);
        } else {
            perror("SCRPD-STOP: cannot open PID file");
        }

        return 1;
    }

    long pid_value;

    if (fscanf(file, "%ld", &pid_value) != 1) {
        fclose(file);

        fprintf(stderr,
                "SCRPD-STOP: invalid PID file\n");

        return 1;
    }

    fclose(file);

    if (pid_value <= 0) {
        fprintf(stderr,
                "SCRPD-STOP: invalid PID: %ld\n",
                pid_value);

        return 1;
    }

    pid_t pid = (pid_t)pid_value;

    printf("SCRPD-STOP: service\n");
    printf("  Name: %s\n", service_name);
    printf("  PID:  %ld\n", pid_value);

    if (kill(pid, SIGTERM) != 0) {
        if (errno == ESRCH) {
            printf("SCRPD-STOP: process already exited\n");
        } else {
            perror("SCRPD-STOP: cannot stop service");
            return 1;
        }
    } else {
        printf("SCRPD-STOP: termination signal sent\n");
    }

    if (remove(pid_path) != 0) {
        perror("SCRPD-STOP: cannot remove PID file");
        return 1;
    }

    printf("SCRPD-STOP: PID state removed\n");
    printf("SCRPD-STOP: service stopped\n");

    return 0;
}
