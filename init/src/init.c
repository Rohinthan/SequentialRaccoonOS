/*
 * Sequential Raccoon OS
 * Raccoon Service Manager v0.3
 *
 * Responsibilities:
 *
 *   - Maintain registered services
 *   - Start services
 *   - Track service PIDs
 *   - Identify service processes
 *   - Handle service exits
 *   - Restart failed services
 *   - Report service state
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "services/service_manager.h"

/* =========================================================
 * Service table
 * ========================================================= */

static raccoon_service_t services[RACCOON_MAX_SERVICES];

static int service_count = 0;


/* =========================================================
 * Internal helpers
 * ========================================================= */

static const char *state_name(service_state_t state)
{
    switch (state) {

        case SERVICE_STOPPED:
            return "STOPPED";

        case SERVICE_STARTING:
            return "STARTING";

        case SERVICE_RUNNING:
            return "RUNNING";

        case SERVICE_FAILED:
            return "FAILED";

        default:
            return "UNKNOWN";
    }
}


/* =========================================================
 * Initialize service manager
 * ========================================================= */

void service_manager_init(void)
{
    memset(
        services,
        0,
        sizeof(services)
    );

    service_count = 0;

    for (int i = 0; i < RACCOON_MAX_SERVICES; i++) {
        services[i].pid = -1;
        services[i].state = SERVICE_STOPPED;
    }

    printf(
        "raccoon-init: service manager initialized\n"
    );

    fflush(stdout);
}


/* =========================================================
 * Register service
 * ========================================================= */

int service_register(
    const char *name,
    const char *command,
    int restart
)
{
    if (name == NULL || command == NULL) {

        fprintf(
            stderr,
            "raccoon-init: invalid service registration\n"
        );

        return -1;
    }

    if (service_count >= RACCOON_MAX_SERVICES) {

        fprintf(
            stderr,
            "raccoon-init: service table full\n"
        );

        return -1;
    }

    raccoon_service_t *service =
        &services[service_count];


    memset(
        service,
        0,
        sizeof(*service)
    );


    strncpy(
        service->name,
        name,
        RACCOON_SERVICE_NAME_MAX - 1
    );

    service->name[
        RACCOON_SERVICE_NAME_MAX - 1
    ] = '\0';


    strncpy(
        service->command,
        command,
        RACCOON_SERVICE_CMD_MAX - 1
    );

    service->command[
        RACCOON_SERVICE_CMD_MAX - 1
    ] = '\0';


    service->pid = -1;

    service->state = SERVICE_STOPPED;

    service->restart = restart ? 1 : 0;


    printf(
        "raccoon-init: registered service '%s'\n",
        service->name
    );

    fflush(stdout);


    return service_count++;
}


/* =========================================================
 * Start one service
 * ========================================================= */

int service_start(int index)
{
    if (index < 0 || index >= service_count)
        return -1;


    raccoon_service_t *service =
        &services[index];


    /*
     * Don't start a service that is already running.
     */

    if (service->state == SERVICE_RUNNING) {

        printf(
            "raccoon-init: service '%s' "
            "already running\n",
            service->name
        );

        return 0;
    }


    service->state = SERVICE_STARTING;


    pid_t pid = fork();

    if (pid < 0) {

        perror(
            "raccoon-init: service fork"
        );

        service->pid = -1;
        service->state = SERVICE_FAILED;

        return -1;
    }


    /*
     * -----------------------------------------------------
     * Service child
     * -----------------------------------------------------
     */

    if (pid == 0) {

        /*
         * PID 1 ignores these signals.
         *
         * Restore normal behavior for services.
         */

        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);


        /*
         * Execute command through BusyBox ash.
         */

        execl(
            "/bin/busybox",
            "busybox",
            "sh",
            "-c",
            service->command,
            (char *)NULL
        );


        perror(
            "raccoon-init: service exec"
        );

        _exit(127);
    }


    /*
     * -----------------------------------------------------
     * PID 1
     * -----------------------------------------------------
     */

    service->pid = pid;

    service->state = SERVICE_RUNNING;


    printf(
        "raccoon-init: service '%s' "
        "started (PID %d)\n",
        service->name,
        pid
    );

    fflush(stdout);


    return 0;
}


/* =========================================================
 * Start all services
 * ========================================================= */

void service_start_all(void)
{
    for (int i = 0; i < service_count; i++) {

        if (service_start(i) != 0) {

            fprintf(
                stderr,
                "raccoon-init: failed to start "
                "service '%s'\n",
                services[i].name
            );
        }
    }
}


/* =========================================================
 * Check whether PID belongs to a service
 * ========================================================= */

int service_is_pid(pid_t pid)
{
    if (pid <= 0)
        return 0;


    for (int i = 0; i < service_count; i++) {

        if (services[i].pid == pid)
            return 1;
    }


    return 0;
}


/* =========================================================
 * Handle service exit
 * ========================================================= */

void service_handle_exit(
    pid_t pid,
    int status
)
{
    for (int i = 0; i < service_count; i++) {

        raccoon_service_t *service =
            &services[i];


        if (service->pid != pid)
            continue;


        if (WIFEXITED(status)) {

            printf(
                "raccoon-init: service '%s' "
                "exited (status=%d)\n",
                service->name,
                WEXITSTATUS(status)
            );

        } else if (WIFSIGNALED(status)) {

            printf(
                "raccoon-init: service '%s' "
                "terminated (signal=%d)\n",
                service->name,
                WTERMSIG(status)
            );
        }


        service->pid = -1;


        if (service->restart) {

            service->state = SERVICE_FAILED;

            printf(
                "raccoon-init: service '%s' "
                "marked for restart\n",
                service->name
            );

        } else {

            service->state = SERVICE_STOPPED;
        }


        fflush(stdout);

        return;
    }
}


/* =========================================================
 * Restart failed services
 * ========================================================= */

void service_restart_failed(void)
{
    for (int i = 0; i < service_count; i++) {

        raccoon_service_t *service =
            &services[i];


        if (service->state != SERVICE_FAILED)
            continue;


        printf(
            "raccoon-init: restarting service '%s'...\n",
            service->name
        );

        fflush(stdout);


        if (service_start(i) != 0) {

            fprintf(
                stderr,
                "raccoon-init: failed to restart "
                "service '%s'\n",
                service->name
            );
        }
    }
}


/* =========================================================
 * Print service status
 * ========================================================= */

void service_print_status(void)
{
    printf("\n");

    printf(
        "========================================\n"
        "       Raccoon Service Manager\n"
        "========================================\n"
    );


    if (service_count == 0) {

        printf(
            "No services registered.\n"
        );
    }


    for (int i = 0; i < service_count; i++) {

        raccoon_service_t *service =
            &services[i];


        printf(
            "%-16s PID=%-5d STATE=%-9s "
            "RESTART=%s\n",

            service->name,

            service->pid,

            state_name(service->state),

            service->restart
                ? "yes"
                : "no"
        );
    }


    printf(
        "========================================\n\n"
    );

    fflush(stdout);
}

