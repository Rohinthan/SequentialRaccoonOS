/*
 * Sequential Raccoon OS
 * Raccoon Init v0.3
 *
 * PID 1 responsibilities:
 *
 *   - Configure console
 *   - Mount basic filesystems
 *   - Set hostname
 *   - Initialize service manager
 *   - Register system services
 *   - Start registered services
 *   - Start the interactive shell
 *   - Reap child processes
 *   - Detect service exits
 *   - Restart failed services
 *   - Restart the shell if it exits
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


/* ---------------------------------------------------------
 * Mount basic virtual filesystems
 * --------------------------------------------------------- */

static void mount_filesystems(void)
{
    /* /proc */
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        if (errno != EBUSY)
            perror("raccoon-init: mount /proc");
    }

    /* /sys */
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        if (errno != EBUSY)
            perror("raccoon-init: mount /sys");
    }

    /* /dev */
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0) {
        if (errno != EBUSY)
            perror("raccoon-init: mount /dev");
    }
}


/* ---------------------------------------------------------
 * Configure standard input/output/error
 * --------------------------------------------------------- */

static void setup_console(void)
{
    int fd = open("/dev/console", O_RDWR);

    if (fd < 0) {
        perror("raccoon-init: open /dev/console");
        return;
    }

    if (dup2(fd, STDIN_FILENO) < 0)
        perror("raccoon-init: dup2 stdin");

    if (dup2(fd, STDOUT_FILENO) < 0)
        perror("raccoon-init: dup2 stdout");

    if (dup2(fd, STDERR_FILENO) < 0)
        perror("raccoon-init: dup2 stderr");

    if (fd > STDERR_FILENO)
        close(fd);
}


/* ---------------------------------------------------------
 * Set system hostname
 * --------------------------------------------------------- */

static void setup_hostname(void)
{
    FILE *file = fopen("/etc/hostname", "r");

    if (file == NULL) {
        perror("raccoon-init: open /etc/hostname");
        return;
    }

    char hostname[256];

    if (fgets(hostname, sizeof(hostname), file) == NULL) {
        perror("raccoon-init: read hostname");
        fclose(file);
        return;
    }

    fclose(file);

    /* Remove trailing newline */
    for (size_t i = 0; hostname[i] != '\0'; i++) {

        if (hostname[i] == '\n' ||
            hostname[i] == '\r') {

            hostname[i] = '\0';
            break;
        }
    }

    if (hostname[0] == '\0')
        return;

    if (sethostname(
            hostname,
            strlen(hostname)
        ) != 0) {

        perror(
            "raccoon-init: sethostname"
        );
    }
}


/* ---------------------------------------------------------
 * Print boot banner
 * --------------------------------------------------------- */

static void print_banner(void)
{
    char hostname[256];

    printf("\n");

    printf(
        "========================================\n"
    );

    printf(
        "   Sequential Raccoon OS v0.4.0\n"
    );

    printf(
        "========================================\n"
    );

    printf(
        "PID 1: Raccoon Init\n"
    );

    if (gethostname(
            hostname,
            sizeof(hostname)
        ) == 0) {

        hostname[sizeof(hostname) - 1] = '\0';

        printf(
            "Hostname: %s\n",
            hostname
        );

    } else {

        printf(
            "Hostname: (unknown)\n"
        );
    }

    printf(
        "Init: supervisor mode\n"
    );

    printf(
        "Services: enabled\n"
    );

    printf(
        "========================================\n"
    );

    printf("\n");

    fflush(stdout);
}


/* ---------------------------------------------------------
 * Start interactive shell
 * --------------------------------------------------------- */

static pid_t start_shell(void)
{
    pid_t pid = fork();

    if (pid < 0) {

        perror(
            "raccoon-init: fork"
        );

        return -1;
    }


    /* -----------------------------------------------------
     * Shell child
     * ----------------------------------------------------- */

    if (pid == 0) {

        /*
         * Create a new session for the shell.
         */

        if (setsid() < 0) {

            perror(
                "raccoon-init: setsid"
            );

            _exit(127);
        }


        /*
         * Open the system console.
         *
         * Current QEMU configuration uses ttyS0.
         */

        int tty_fd = open(
            "/dev/ttyS0",
            O_RDWR
        );

        if (tty_fd < 0) {

            perror(
                "raccoon-init: open /dev/ttyS0"
            );

            _exit(127);
        }


        /*
         * Make ttyS0 the controlling terminal.
         */

        if (ioctl(
                tty_fd,
                TIOCSCTTY,
                0
            ) < 0) {

            perror(
                "raccoon-init: TIOCSCTTY"
            );

            close(tty_fd);

            _exit(127);
        }


        /*
         * Connect stdin/stdout/stderr.
         */

        if (dup2(
                tty_fd,
                STDIN_FILENO
            ) < 0) {

            perror(
                "raccoon-init: dup2 stdin"
            );
        }

        if (dup2(
                tty_fd,
                STDOUT_FILENO
            ) < 0) {

            perror(
                "raccoon-init: dup2 stdout"
            );
        }

        if (dup2(
                tty_fd,
                STDERR_FILENO
            ) < 0) {

            perror(
                "raccoon-init: dup2 stderr"
            );
        }


        if (tty_fd > STDERR_FILENO)
            close(tty_fd);


        /*
         * PID 1 ignores these signals.
         *
         * Restore normal behavior for the shell.
         */

        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);


        /*
         * Execute BusyBox shell directly.
         */

        char *argv[] = {
            "/bin/busybox",
            "sh",
            "-l",
            NULL
        };

        execv(
            "/bin/busybox",
            argv
        );


        /*
         * exec failed.
         */

        perror(
            "raccoon-init: exec shell"
        );

        _exit(127);
    }


    /* -----------------------------------------------------
     * PID 1 parent
     * ----------------------------------------------------- */

    printf(
        "raccoon-init: shell started (PID %d)\n",
        pid
    );

    fflush(stdout);

    return pid;
}


/* ---------------------------------------------------------
 * PID 1 main supervisor
 * --------------------------------------------------------- */

int main(void)
{
    /*
     * -----------------------------------------------------
     * PID 1 signal policy
     * -----------------------------------------------------
     *
     * PID 1 ignores normal terminal signals.
     *
     * Child processes restore default behavior.
     */

    signal(SIGTERM, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);


    /*
     * -----------------------------------------------------
     * Basic system initialization
     * -----------------------------------------------------
     */

    mount_filesystems();

    setup_console();

    setup_hostname();


    /*
     * -----------------------------------------------------
     * Initialize service manager
     * -----------------------------------------------------
     */

    service_manager_init();


    /*
     * -----------------------------------------------------
     * Register Phase 2 test service
     * -----------------------------------------------------
     *
     * This service intentionally stays alive.
     *
     * restart = 1 means PID 1 should restart it if
     * the process exits.
     */

    if (service_register(
            "raccoon-test",
            "/bin/busybox sh -c 'while true; do sleep 30; done'",
            1
        ) < 0) {

        fprintf(
            stderr,
            "raccoon-init: failed to register "
            "raccoon-test service\n"
        );
    }


    /*
     * -----------------------------------------------------
     * Boot banner
     * -----------------------------------------------------
     */

    print_banner();


    printf(
        "Starting Raccoon Init supervisor...\n"
    );

    printf(
        "Starting Raccoon services...\n"
    );

    fflush(stdout);


    /*
     * -----------------------------------------------------
     * Start all registered services
     * -----------------------------------------------------
     */

    service_start_all();


    /*
     * -----------------------------------------------------
     * Start interactive shell
     * -----------------------------------------------------
     */

    printf(
        "Starting Sequential Raccoon shell...\n\n"
    );

    fflush(stdout);


    /*
     * -----------------------------------------------------
     * Main supervisor loop
     * -----------------------------------------------------
     *
     * PID 1 remains alive permanently.
     *
     * waitpid(-1) allows PID 1 to supervise:
     *
     *   - the interactive shell
     *   - registered services
     *   - future system services
     */

    while (1) {

        pid_t shell_pid = start_shell();


        /*
         * If shell creation fails,
         * avoid a rapid fork loop.
         */

        if (shell_pid < 0) {

            sleep(1);

            continue;
        }


        /*
         * -------------------------------------------------
         * Wait for any child process.
         * -------------------------------------------------
         */

        while (1) {

            int status;

            pid_t child =
                waitpid(
                    -1,
                    &status,
                    0
                );


            /*
             * waitpid interrupted by signal.
             */

            if (child < 0) {

                if (errno == EINTR)
                    continue;

                perror(
                    "raccoon-init: waitpid"
                );

                break;
            }


            /*
             * -------------------------------------------------
             * Interactive shell exited.
             * -------------------------------------------------
             */

            if (child == shell_pid) {

                if (WIFEXITED(status)) {

                    printf(
                        "\nraccoon-init: shell exited "
                        "with status %d\n",
                        WEXITSTATUS(status)
                    );

                } else if (WIFSIGNALED(status)) {

                    printf(
                        "\nraccoon-init: shell terminated "
                        "by signal %d\n",
                        WTERMSIG(status)
                    );
                }

                fflush(stdout);


                /*
                 * Leave the inner wait loop.
                 *
                 * The outer loop will restart the shell.
                 */

                break;
            }


            /*
             * -------------------------------------------------
             * Another child exited.
             *
             * This may be a registered service.
             *
             * Give the service manager ownership of the
             * exit event.
             * -------------------------------------------------
             */

            service_handle_exit(
                child,
                status
            );


            /*
             * -------------------------------------------------
             * Restart failed services.
             * -------------------------------------------------
             */

            service_restart_failed();
        }


        /*
         * -----------------------------------------------------
         * Small delay before restarting shell.
         * -----------------------------------------------------
         *
         * Prevents a broken shell from creating a rapid
         * fork/exec loop.
         */

        sleep(1);


        printf(
            "raccoon-init: restarting shell...\n"
        );

        fflush(stdout);
    }


    return 0;
}
