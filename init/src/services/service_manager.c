#include "service_manager.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

/* =========================================================
 * Sequential Raccoon OS
 * Raccoon Service Manager v0.2
 *
 * Responsibilities:
 *
 *   - Maintain registered services
 *   - Start services
 *   - Track service PIDs
 *   - Detect exited services
 *   - Restart services when requested
 *   - Report service state
 *
 * ========================================================= */


/* ---------------------------------------------------------
 * Service table
 * --------------------------------------------------------- */

static raccoon_service_t services[RACCOON_MAX_SERVICES];

static int service_count = 0;


/* ---------------------------------------------------------
 * Convert service state to readable string
 * --------------------------------------------------------- */

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


/* ---------------------------------------------------------
 * Initialize service manager
 * --------------------------------------------------------- */

void service_manager_init(void)
{
    memset(services, 0, sizeof(services));

    service_count = 0;

    printf(
        "raccoon-init: service manager initialized\n"
    );

    fflush(stdout);
}


/* ---------------------------------------------------------
 * Register a service
 * --------------------------------------------------------- */

int service_register(
    const char *name,
    const char *command,
    int restart
)
{
    /*
     * Validate arguments.
     */

    if (name == NULL || command == NULL) {
        fprintf(
            stderr,
            "raccoon-init: invalid service registration\n"
        );

        return -1;
    }

    /*
     * Check service table capacity.
     */

    if (service_count >= RACCOON_MAX_SERVICES) {
        fprintf(
            stderr,
            "raccoon-init: service table full\n"
        );

        return -1;
    }

    raccoon_service_t *service =
        &services[service_count];


    /*
     * Copy service name.
     */

    strncpy(
        service->name,
        name,
        RACCOON_SERVICE_NAME_MAX - 1
    );

    service->name[
        RACCOON_SERVICE_NAME_MAX - 1
    ] = '\0';


    /*
     * Copy service command.
     */

    strncpy(
        service->command,
        command,
        RACCOON_SERVICE_CMD_MAX - 1
    );

    service->command[
        RACCOON_SERVICE_CMD_MAX - 1
    ] = '\0';


    /*
     * Initial service state.
     */

    service->pid = -1;

    service->state = SERVICE_STOPPED;

    service->restart = restart;


    printf(
        "raccoon-init: registered service '%s'\n",
        service->name
    );

    fflush(stdout);


    /*
     * Return service index.
     */

    return service_count++;
}


/* ---------------------------------------------------------
 * Start one service
 * --------------------------------------------------------- */

int service_start(int index)
{
    /*
     * Validate service index.
     */

    if (index < 0 || index >= service_count) {
        return -1;
    }

    raccoon_service_t *service =
        &services[index];


    /*
     * Do not start an already running service.
     */

    if (service->state == SERVICE_RUNNING) {

        printf(
            "raccoon-init: service '%s' already running\n",
            service->name
        );

        return 0;
    }


    /*
     * Mark service as starting.
     */

    service->state = SERVICE_STARTING;


    /*
     * Create service process.
     */

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
     * Service child.
     */

    if (pid == 0) {

        /*
         * PID 1 ignores several signals.
         *
         * Those signal dispositions are inherited
         * across fork(), so restore normal behavior
         * before executing the service.
         */

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);


        /*
         * Execute service command through BusyBox sh.
         */

        execl(
            "/bin/busybox",
            "busybox",
            "sh",
            "-c",
            service->command,
            (char *)NULL
        );


        /*
         * exec failed.
         */

        perror(
            "raccoon-init: service exec"
        );

        _exit(127);
    }


    /*
     * Parent / PID 1.
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


/* ---------------------------------------------------------
 * Start all registered services
 * --------------------------------------------------------- */

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


/* ---------------------------------------------------------
 * Handle exited child process
 * --------------------------------------------------------- */

void service_handle_exit(
    pid_t pid,
    int status
)
{
    for (int i = 0; i < service_count; i++) {

        raccoon_service_t *service = &services[i];

        if (service->pid != pid)
            continue;

        /*
         * Process exited normally.
         */
        if (WIFEXITED(status)) {

            int exit_status = WEXITSTATUS(status);

            printf(
                "raccoon-init: service '%s' exited "
                "(status=%d)\n",
                service->name,
                exit_status
            );
        }

        /*
         * Process was terminated by a signal.
         */
        else if (WIFSIGNALED(status)) {

            int signal_number = WTERMSIG(status);

            printf(
                "raccoon-init: service '%s' terminated "
                "(signal=%d)\n",
                service->name,
                signal_number
            );
        }

        /*
         * Process is no longer running.
         */
        service->pid = -1;

        /*
         * Decide whether the service should restart.
         */
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

/* ---------------------------------------------------------
 * Restart failed services
 * --------------------------------------------------------- */

void service_restart_failed(void)
{
    for (int i = 0; i < service_count; i++) {

        raccoon_service_t *service = &services[i];

        if (service->state != SERVICE_FAILED)
            continue;

        printf(
            "raccoon-init: restarting service '%s'...\n",
            service->name
        );

        fflush(stdout);

        if (service_start(i) != 0) {

            /*
             * Restart failed.
             * Keep the service in FAILED state so
             * the supervisor can try again later.
             */
            service->state = SERVICE_FAILED;

            fprintf(
                stderr,
                "raccoon-init: failed to restart "
                "service '%s'\n",
                service->name
            );

        } else {

            /*
             * service_start() should set the state to
             * STARTING/RUNNING and assign a new PID.
             */
            printf(
                "raccoon-init: service '%s' restarted "
                "(PID %d)\n",
                service->name,
                service->pid
            );
        }

        fflush(stdout);
    }
}

/* ---------------------------------------------------------
 * Print service status
 * --------------------------------------------------------- */

void service_print_status(void)
{
    printf("\n");

    printf(
        "========================================\n"
    );

    printf(
        "       Raccoon Service Manager\n"
    );

    printf(
        "========================================\n"
    );


    /*
     * No registered services.
     */

    if (service_count == 0) {

        printf(
            "No services registered.\n"
        );
    }


    /*
     * Print every registered service.
     */

    for (int i = 0; i < service_count; i++) {

        raccoon_service_t *service =
            &services[i];


        printf(
            "%-16s PID=%-5d STATE=%s "
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
        "========================================\n"
    );

    printf("\n");

    fflush(stdout);
}
