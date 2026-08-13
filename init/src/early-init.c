#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <errno.h>

static void mount_virtual_filesystems(void)
{
    if (mount("proc", "/proc", "proc", 0, NULL) != 0)
        perror("mount /proc");

    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0)
        perror("mount /sys");

    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0)
        perror("mount /dev");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" Sequential Raccoon Early Boot\n");
    printf("========================================\n");
    fflush(stdout);

    mount_virtual_filesystems();

    /*
     * The new root is a directory inside the initramfs.
     * It must exist before mount() can use it.
     */
    if (mkdir("/newroot", 0755) != 0 && errno != EEXIST) {
        perror("mkdir /newroot");
        while (1)
            pause();
    }

    printf("Waiting for root device...\n");
    fflush(stdout);

    /*
     * Wait for the VirtIO block device to appear.
     */
    int root_found = 0;

    for (int i = 0; i < 100; i++) {
        int fd = open("/dev/vda", O_RDONLY);

        if (fd >= 0) {
            close(fd);
            root_found = 1;
            break;
        }

        usleep(100000);
    }

    if (!root_found) {
        printf("ERROR: /dev/vda not found\n");
        fflush(stdout);

        while (1)
            pause();
    }

    printf("Persistent root device found.\n");
    printf("Mounting persistent root filesystem...\n");
    fflush(stdout);

    if (mount("/dev/vda", "/newroot", "ext4", 0, NULL) != 0) {
        perror("mount /dev/vda");
        printf("Cannot mount persistent root.\n");
        fflush(stdout);

        while (1)
            pause();
    }

    printf("Persistent root mounted.\n");
    printf("Switching to Sequential Raccoon root...\n");
    fflush(stdout);

    /*
     * Create mount points inside the persistent root.
     */
    mkdir("/newroot/proc", 0755);
    mkdir("/newroot/sys", 0755);
    mkdir("/newroot/dev", 0755);
    mkdir("/newroot/run", 0755);

    /*
     * Move the kernel virtual filesystems to the new root.
     */
    if (mount("/proc", "/newroot/proc", NULL, MS_MOVE, NULL) != 0)
        perror("move /proc");

    if (mount("/sys", "/newroot/sys", NULL, MS_MOVE, NULL) != 0)
        perror("move /sys");

    if (mount("/dev", "/newroot/dev", NULL, MS_MOVE, NULL) != 0)
        perror("move /dev");

    /*
     * Transfer control to the persistent system.
     */
    char *argv[] = {
        "/bin/busybox",
        "switch_root",
        "/newroot",
        "/init",
        NULL
    };

    execv("/bin/busybox", argv);

    perror("exec switch_root");

    while (1)
        pause();

    return 1;
}
