#ifndef RACCOON_SERVICE_MANAGER_H
#define RACCOON_SERVICE_MANAGER_H

#include <sys/types.h>

#define RACCOON_MAX_SERVICES 8
#define RACCOON_SERVICE_NAME_MAX 32
#define RACCOON_SERVICE_CMD_MAX 128

typedef enum {
    SERVICE_STOPPED = 0,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_FAILED
} service_state_t;

typedef struct {
    char name[RACCOON_SERVICE_NAME_MAX];
    char command[RACCOON_SERVICE_CMD_MAX];

    pid_t pid;

    service_state_t state;

    int restart;
} raccoon_service_t;

/*
 * Initialize the service manager.
 */
void service_manager_init(void);

/*
 * Register a new service.
 */
int service_register(
    const char *name,
    const char *command,
    int restart
);

/*
 * Start a registered service.
 */
int service_start(int index);

/*
 * Start all registered services.
 */
void service_start_all(void);

/*
 * Handle a child process that exited.
 */
void service_handle_exit(pid_t pid, int status);

/*
 * Print current service state.
 */
void service_print_status(void);

/*
 * Restart services marked for restart.
 */
void service_restart_failed(void);

#endif
