/*
 * Sequential Raccoon OS
 * Raccoon Init v0.2.1
 *
 * PID 1 responsibilities:
 *   - Configure console
 *   - Mount basic filesystems
 *   - Set hostname
 *   - Start the interactive shell
 *   - Reap child processes
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
        if (hostname[i] == '\n' || hostname[i] == '\r') {
            hostname[i] = '\0';
            break;
        }
    }

    if (hostname[0] == '\0')
        return;

    if (sethostname(hostname, strlen(hostname)) != 0)
        perror("raccoon-init: sethostname");
}

/* ---------------------------------------------------------
 * Print boot banner
 * --------------------------------------------------------- */

static void print_banner(void)
{
    char hostname[256];

    printf("\n");
    printf("========================================\n");
    printf("   Sequential Raccoon OS v0.2.1\n");
    printf("========================================\n");
    printf("PID 1: Raccoon Init\n");

    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[sizeof(hostname) - 1] = '\0';
        printf("Hostname: %s\n", hostname);
    } else {
        printf("Hostname: (unknown)\n");
    }

    printf("Init: supervisor mode\n");
    printf("========================================\n");
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
        perror("raccoon-init: fork");
        return -1;
    }

    if (pid == 0) {

        /*
         * -------------------------------------------------
         * Create a new session for the shell.
         * -------------------------------------------------
         */

        if (setsid() < 0) {
            perror("raccoon-init: setsid");
            _exit(127);
        }


        /*
         * -------------------------------------------------
         * Open the system console.
         *
         * In the current QEMU setup this is ttyS0.
         * -------------------------------------------------
         */

        int tty_fd = open(
            "/dev/ttyS0",
            O_RDWR
        );

        if (tty_fd < 0) {
            perror("raccoon-init: open /dev/ttyS0");
            _exit(127);
        }


        /*
         * -------------------------------------------------
         * Make ttyS0 the controlling terminal.
         * -------------------------------------------------
         */

        if (ioctl(tty_fd, TIOCSCTTY, 0) < 0) {
            perror("raccoon-init: TIOCSCTTY");
            close(tty_fd);
            _exit(127);
        }


//temporarily removed

        /*
         * -------------------------------------------------
         * Connect stdin/stdout/stderr to the terminal.
         * -------------------------------------------------
         */

        if (dup2(tty_fd, STDIN_FILENO) < 0)
            perror("raccoon-init: dup2 stdin");

        if (dup2(tty_fd, STDOUT_FILENO) < 0)
            perror("raccoon-init: dup2 stdout");

        if (dup2(tty_fd, STDERR_FILENO) < 0)
            perror("raccoon-init: dup2 stderr");


        if (tty_fd > STDERR_FILENO)
            close(tty_fd);


        /*
         * -------------------------------------------------
         * PID 1 ignores these signals.
         *
         * Restore normal behavior for the shell.
         * -------------------------------------------------
         */

        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);


        /*
         * -------------------------------------------------
         * Execute BusyBox shell directly.
         *
         * No cttyhack is needed anymore because we
         * explicitly created the controlling terminal.
         * -------------------------------------------------
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

        perror("raccoon-init: exec shell");

        _exit(127);
    }


    /*
     * -----------------------------------------------------
     * Parent / PID 1
     * -----------------------------------------------------
     */

    printf(
        "raccoon-init: shell started (PID %d)\n",
        pid
    );

    fflush(stdout);

    return pid;
}
/* ---------------------------------------------------------
 * Reap children without blocking
 * --------------------------------------------------------- */

static void reap_children(void)
{
    int status;

    while (1) {
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid <= 0)
            break;

        if (WIFEXITED(status)) {
            printf(
                "raccoon-init: reaped child %d (exit=%d)\n",
                pid,
                WEXITSTATUS(status)
            );
        } else if (WIFSIGNALED(status)) {
            printf(
                "raccoon-init: reaped child %d (signal=%d)\n",
                pid,
                WTERMSIG(status)
            );
        }

        fflush(stdout);
    }
}

/* ---------------------------------------------------------
 * PID 1 main supervisor
 * --------------------------------------------------------- */

int main(void)
{
    /*
     * PID 1 must not accidentally die from normal terminal
     * signals.
     *
     * The shell handles Ctrl+C / Ctrl+Z for itself.
     */
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);

    /*
     * Basic system initialization.
     */
    mount_filesystems();
    setup_console();
    setup_hostname();

    print_banner();

    printf("Starting Raccoon Init supervisor...\n");
    printf("Starting Sequential Raccoon shell...\n\n");
    fflush(stdout);

    /*
     * Main supervisor loop.
     *
     * PID 1 never becomes the shell.
     * The shell remains a child process.
     */
    while (1) {

        pid_t shell_pid = start_shell();

        /*
         * If fork() fails, don't spin at 100% CPU.
         */
        if (shell_pid < 0) {
            sleep(1);
            continue;
        }

        /*
         * Wait for children.
         */
        while (1) {
            int status;

            pid_t child = waitpid(-1, &status, 0);

            if (child < 0) {

                if (errno == EINTR)
                    continue;

                perror("raccoon-init: waitpid");
                break;
            }

            /*
             * Our shell exited.
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
                 * Break out of the child-wait loop.
                 *
                 * The outer loop will restart the shell.
                 */
                break;
            }

            /*
             * Another child exited.
             *
             * This is important for future v0.2 services.
             */
            if (WIFEXITED(status)) {

                printf(
                    "raccoon-init: child %d exited "
                    "with status %d\n",
                    child,
                    WEXITSTATUS(status)
                );

            } else if (WIFSIGNALED(status)) {

                printf(
                    "raccoon-init: child %d terminated "
                    "by signal %d\n",
                    child,
                    WTERMSIG(status)
                );
            }

            fflush(stdout);
        }

        /*
         * Small delay before restarting the shell.
         *
         * Prevents a broken shell from creating a rapid
         * fork/exec loop.
         */
        sleep(1);

        printf("raccoon-init: restarting shell...\n");
        fflush(stdout);
    }

    return 0;
}
