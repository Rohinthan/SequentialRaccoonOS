#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   Welcome to Sequential Raccoon OS\n");
    printf("========================================\n");
    printf("PID 1: Sequential Raccoon init\n");
    printf("\n");

    if (mount("proc", "/proc", "proc", 0, NULL) != 0)
        perror("mount /proc");

    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0)
        perror("mount /sys");

    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0)
        perror("mount /dev");

    while (1)
        pause();

    return 0;
}
