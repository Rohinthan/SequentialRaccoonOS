#ifndef RACCOON_SERVICE_MANAGER_H
#define RACCOON_SERVICE_MANAGER_H

#include <sys/types.h>

/*
 * =========================================================
 * Sequential Raccoon OS
 * Raccoon Service Manager
 *
 * Public interface for PID 1's service supervisor.
 * =========================================================
 */


/* ---------------------------------------------------------
 * Service limits
 * --------------------------------------------------------- */

#define RACCOON_MAX_SERVICES        8
#define RACCOON_SERVICE_NAME_MAX    32
#define RACCOON_SERVICE_CMD_MAX     128


/* ---------------------------------------------------------
 * Service states
 * --------------------------------------------------------- */

typedef enum {

    SERVICE_STOPPED = 0,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_FAILED

} service_state_t;


/* ---------------------------------------------------------
 * Service descriptor
 * --------------------------------------------------------- */

typedef struct {

    /*
     * Human-readable service name.
     */
    char name[RACCOON_SERVICE_NAME_MAX];

    /*
     * Command executed by the service.
     */
    char command[RACCOON_SERVICE_CMD_MAX];

    /*
     * Process ID of the running service.
     *
     * -1 means the service currently has
     * no running process.
     */
    pid_t pid;

    /*
     * Current service state.
     */
    service_state_t state;

    /*
     * Whether the service should be restarted
     * after exiting.
     */
    int restart;

} raccoon_service_t;


/* =========================================================
 * Service Manager API
 * ========================================================= */


/* ---------------------------------------------------------
 * Initialize service manager
 *
 * Clears the service table and resets service state.
 * --------------------------------------------------------- */

void service_manager_init(void);


/* ---------------------------------------------------------
 * Register service
 *
 * Returns:
 *   >= 0 : service index
 *   -1   : registration failed
 * --------------------------------------------------------- */

int service_register(
    const char *name,
    const char *command,
    int restart
);


/* ---------------------------------------------------------
 * Start one service
 *
 * Returns:
 *    0 : success
 *   -1 : failure
 * --------------------------------------------------------- */

int service_start(int index);


/* ---------------------------------------------------------
 * Start all registered services
 * --------------------------------------------------------- */

void service_start_all(void);


/* ---------------------------------------------------------
 * Check whether a PID belongs to a service
 *
 * Returns:
 *    1 : PID belongs to a registered service
 *    0 : PID is not a service
 * --------------------------------------------------------- */

int service_is_pid(pid_t pid);


/* ---------------------------------------------------------
 * Handle exited service
 *
 * Called by PID 1 after waitpid() reports a child.
 * --------------------------------------------------------- */

void service_handle_exit(
    pid_t pid,
    int status
);


/* ---------------------------------------------------------
 * Restart failed services
 * --------------------------------------------------------- */

void service_restart_failed(void);


/* ---------------------------------------------------------
 * Print service status
 * --------------------------------------------------------- */

void service_print_status(void);


#endif /* RACCOON_SERVICE_MANAGER_H */
