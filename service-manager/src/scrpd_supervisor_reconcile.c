#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "scrpd_runtime.h"

int scrpd_supervisor_reconcile(
    scrpd_runtime_t *runtime
)
{
    if (runtime == NULL) {
        return -1;
    }

    if (runtime->runtime.pid <= 0) {
        return 0;
    }

    int status = 0;

    pid_t result = waitpid(
        (pid_t)runtime->runtime.pid,
        &status,
        WNOHANG
    );

    if (result == 0) {
        /*
         * Child is still running.
         */
        if (runtime->runtime.state == SCRPD_STATE_STARTING) {
            runtime->runtime.state = SCRPD_STATE_RUNNING;
        }

        return 0;
    }

    if (result == (pid_t)runtime->runtime.pid) {
        /*
         * Child exited and has been reaped.
         */
        if (runtime->runtime.state == SCRPD_STATE_RUNNING ||
            runtime->runtime.state == SCRPD_STATE_STARTING) {

            runtime->runtime.state = SCRPD_STATE_FAILED;
        }

        return 0;
    }

    if (result < 0) {
        if (errno == ECHILD) {
            /*
             * The child is no longer owned by this process.
             * Treat the runtime as no longer alive.
             */
            if (runtime->runtime.state == SCRPD_STATE_RUNNING ||
                runtime->runtime.state == SCRPD_STATE_STARTING) {

                runtime->runtime.state = SCRPD_STATE_FAILED;
            }

            return 0;
        }

        return -1;
    }

    return 0;
}
