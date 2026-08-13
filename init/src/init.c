#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <errno.h>

static void mount_filesystems(void)
{
    if (mount("proc", "/proc", "proc", 0, NULL) != 0)
        perror("mount /proc");

    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0)
        perror("mount /sys");

    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0)
        perror("mount /dev");
}

static void setup_console(void)
{
    int fd = open("/dev/console", O_RDWR);

    if (fd < 0) {
        perror("open /dev/console");
        return;
    }

    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    if (fd > STDERR_FILENO)
        close(fd);
}

static void setup_hostname(void)
{
    FILE *file = fopen("/etc/hostname", "r");

    if (file == NULL) {
        perror("open /etc/hostname");
        return;
    }

    char hostname[256];

    if (fgets(hostname, sizeof(hostname), file) == NULL) {
        perror("read /etc/hostname");
        fclose(file);
        return;
    }

    fclose(file);

    /* Remove trailing newline */
    for (int i = 0; hostname[i] != '\0'; i++) {
        if (hostname[i] == '\n' || hostname[i] == '\r') {
            hostname[i] = '\0';
            break;
        }
    }

    if (hostname[0] == '\0')
        return;

    if (sethostname(hostname, strlen(hostname)) != 0)
        perror("sethostname");
}

int main(void)
{

    setup_console();
    setup_hostname();

    printf("\n");
    printf("========================================\n");
    printf("   Welcome to Sequential Raccoon OS\n");
    printf("========================================\n");
    printf("PID 1: Sequential Raccoon init\n");

    printf("Hostname: ");
    
    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) == 0)
        printf("%s\n", hostname);
    else
        printf("(unknown)\n");

    printf("\n");
    printf("Starting Sequential Raccoon shell...\n\n");

    char *argv[] = {
        "/bin/busybox",
        "sh",
        NULL
    };

    execv("/bin/busybox", argv);

    perror("execv /bin/busybox");

    while (1)
        pause();

    return 1;
}
