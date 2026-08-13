#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

KERNEL_SRC="$PROJECT_ROOT/kernel/linux-src"
ROOTFS="$PROJECT_ROOT/rootfs"

BUILD="$PROJECT_ROOT/build"
KERNEL_BUILD="$BUILD/kernel/v0.1"
INITRAMFS_BUILD="$BUILD/initramfs/v0.1"
BOOT_BUILD="$BUILD/boot/v0.1"
DISK_BUILD="$BUILD/disk/v0.1"

DISK="$DISK_BUILD/raccoon-root.qcow2"
MOUNT_POINT="/mnt/sequential-raccoon-root"
NBD="/dev/nbd0"

echo "========================================"
echo " Sequential Raccoon OS Build"
echo "========================================"

mkdir -p "$KERNEL_BUILD"
mkdir -p "$INITRAMFS_BUILD"
mkdir -p "$BOOT_BUILD"
mkdir -p "$DISK_BUILD"

echo
echo "[1/6] Building normal system init..."

gcc -static -O2 \
    -o "$ROOTFS/init" \
    "$PROJECT_ROOT/init/src/init.c"

chmod +x "$ROOTFS/init"

echo
echo "[2/6] Building early boot init..."

gcc -static -O2 \
    -o "$ROOTFS/early-init" \
    "$PROJECT_ROOT/init/src/early-init.c"

chmod +x "$ROOTFS/early-init"

echo
echo "[3/6] Building initramfs..."

rm -f "$INITRAMFS_BUILD/initramfs.cpio"
rm -f "$INITRAMFS_BUILD/initramfs.cpio.gz"

cp "$ROOTFS/init" "$ROOTFS/init.system"
cp "$ROOTFS/early-init" "$ROOTFS/init"

(
    cd "$ROOTFS"

    find . \
        -path './init.system' -prune -o \
        -print0 |
        cpio --null -ov --format=newc \
        > "$INITRAMFS_BUILD/initramfs.cpio"
)

mv "$ROOTFS/init.system" "$ROOTFS/init"

gzip -9 -f "$INITRAMFS_BUILD/initramfs.cpio"

echo
echo "[4/6] Building kernel..."

cp "$PROJECT_ROOT/kernel/configs/raccoon-x86_64-v0.1.config" \
    "$KERNEL_SRC/.config"

make -C "$KERNEL_SRC" -j"$(nproc)"

cp "$KERNEL_SRC/arch/x86/boot/bzImage" \
    "$KERNEL_BUILD/raccoon-kernel"

cp "$KERNEL_SRC/.config" \
    "$KERNEL_BUILD/kernel.config"

make -C "$KERNEL_SRC" kernelversion \
    > "$KERNEL_BUILD/version.txt"

echo
echo "[5/6] Updating persistent root filesystem..."

if [ ! -f "$DISK" ]; then
    echo "ERROR: Persistent disk not found:"
    echo "  $DISK"
    exit 1
fi

sudo modprobe nbd max_part=8

# Make sure nbd0 isn't already connected.
sudo qemu-nbd --disconnect "$NBD" 2>/dev/null || true

sudo qemu-nbd --connect="$NBD" "$DISK"

cleanup_disk()
{
    sudo umount "$MOUNT_POINT" 2>/dev/null || true
    sudo qemu-nbd --disconnect "$NBD" 2>/dev/null || true
}

trap cleanup_disk EXIT

sudo mkdir -p "$MOUNT_POINT"

sudo mount "$NBD" "$MOUNT_POINT"

echo "Synchronizing rootfs → persistent disk..."

sudo rm -rf \
    "$MOUNT_POINT/bin" \
    "$MOUNT_POINT/etc" \
    "$MOUNT_POINT/home" \
    "$MOUNT_POINT/init" \
    "$MOUNT_POINT/early-init" \
    "$MOUNT_POINT/root" \
    "$MOUNT_POINT/run" \
    "$MOUNT_POINT/sbin" \
    "$MOUNT_POINT/tmp" \
    "$MOUNT_POINT/usr" \
    "$MOUNT_POINT/var"

sudo cp -a "$ROOTFS/." "$MOUNT_POINT/"

sudo sync

echo "Persistent filesystem updated."

echo
echo "[6/6] Preparing boot artifacts..."

cp "$KERNEL_BUILD/raccoon-kernel" \
    "$BOOT_BUILD/raccoon-kernel"

cp "$INITRAMFS_BUILD/initramfs.cpio.gz" \
    "$BOOT_BUILD/initramfs.cpio.gz"

echo
echo "========================================"
echo " Build complete!"
echo "========================================"
echo
echo "Kernel:"
echo "  $BOOT_BUILD/raccoon-kernel"
echo
echo "Initramfs:"
echo "  $BOOT_BUILD/initramfs.cpio.gz"
echo
echo "Persistent disk:"
echo "  $DISK"
