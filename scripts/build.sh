#!/bin/bash

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

ROOTFS="$PROJECT_ROOT/rootfs"
BUILD="$PROJECT_ROOT/build"

INITRAMFS_BUILD="$BUILD/initramfs/v0.1"
KERNEL_BUILD="$BUILD/kernel/v0.1"
BOOT_BUILD="$BUILD/boot/v0.1"
DISK_BUILD="$BUILD/disk/v0.1"

DISK="$DISK_BUILD/raccoon-root.qcow2"
NBD="/dev/nbd0"
MOUNT_POINT="/mnt/sequential-raccoon-root"

KERNEL_SRC="$PROJECT_ROOT/kernel/linux-src"

mkdir -p "$INITRAMFS_BUILD"
mkdir -p "$KERNEL_BUILD"
mkdir -p "$BOOT_BUILD"
mkdir -p "$DISK_BUILD"

echo
echo "========================================"
echo " Sequential Raccoon OS Build"
echo "========================================"
echo

# ============================================================
# [1/6] Build normal system init
# ============================================================

echo "[1/6] Building normal system init..."

gcc -static -O2 \
    -Wall \
    -Wextra \
    -o "$ROOTFS/init" \
    "$PROJECT_ROOT/init/src/init.c" \
    "$PROJECT_ROOT/init/src/services/service_manager.c"

chmod +x "$ROOTFS/init"

# ============================================================
# [2/6] Build early boot init
# ============================================================

echo
echo "[2/6] Building early boot init..."

gcc -static -O2 \
    -Wall \
    -Wextra \
    -o "$ROOTFS/early-init" \
    "$PROJECT_ROOT/init/src/early-init.c"

chmod +x "$ROOTFS/early-init"

# ============================================================
# [3/6] Build initramfs
# ============================================================

echo
echo "[3/6] Building initramfs..."

rm -f "$INITRAMFS_BUILD/initramfs.cpio"
rm -f "$INITRAMFS_BUILD/initramfs.cpio.gz"

# Save normal init temporarily.
cp "$ROOTFS/init" "$ROOTFS/init.system"

# Early init becomes /init inside initramfs.
cp "$ROOTFS/early-init" "$ROOTFS/init"

chmod +x "$ROOTFS/init"

(
    cd "$ROOTFS"

    find . \
        -path './init.system' -prune -o \
        -print0 |
    sudo cpio \
        --null \
        -ov \
        --format=newc \
        > "$INITRAMFS_BUILD/initramfs.cpio"
)

# Restore normal system init.
mv "$ROOTFS/init.system" "$ROOTFS/init"

# Compress initramfs.
gzip -9 -f "$INITRAMFS_BUILD/initramfs.cpio"

echo
echo "Initramfs created:"
echo "  $INITRAMFS_BUILD/initramfs.cpio.gz"

# ============================================================
# [4/6] Build kernel
# ============================================================

echo
echo "[4/6] Building kernel..."

cp \
    "$PROJECT_ROOT/kernel/configs/raccoon-x86_64-v0.1.config" \
    "$KERNEL_SRC/.config"

make -C "$KERNEL_SRC" -j"$(nproc)"

cp \
    "$KERNEL_SRC/arch/x86/boot/bzImage" \
    "$KERNEL_BUILD/raccoon-kernel"

cp \
    "$KERNEL_SRC/.config" \
    "$KERNEL_BUILD/kernel.config"

make -C "$KERNEL_SRC" kernelversion \
    > "$KERNEL_BUILD/version.txt"

# ============================================================
# [5/6] Update persistent root filesystem
# ============================================================

echo
echo "[5/6] Updating persistent root filesystem..."

if [ ! -f "$DISK" ]; then
    echo
    echo "ERROR: Persistent disk not found:"
    echo "  $DISK"
    exit 1
fi

# Make sure required tools exist.
command -v qemu-nbd >/dev/null 2>&1 || {
    echo "ERROR: qemu-nbd is not installed."
    exit 1
}

command -v cpio >/dev/null 2>&1 || {
    echo "ERROR: cpio is not installed."
    exit 1
}

# Load NBD module.
sudo modprobe nbd max_part=8

# Disconnect stale NBD connection.
sudo qemu-nbd --disconnect "$NBD" 2>/dev/null || true

echo "Connecting persistent disk..."

sudo qemu-nbd \
    --connect="$NBD" \
    "$DISK"

cleanup_disk()
{
    echo
    echo "Cleaning up persistent disk..."

    sudo umount "$MOUNT_POINT" 2>/dev/null || true
    sudo qemu-nbd --disconnect "$NBD" 2>/dev/null || true
}

trap cleanup_disk EXIT

# Wait for NBD device.
for i in $(seq 1 20); do
    if [ -b "$NBD" ]; then
        break
    fi

    sleep 0.2
done

if [ ! -b "$NBD" ]; then
    echo "ERROR: $NBD did not appear."
    exit 1
fi

echo "Mounting persistent filesystem..."

sudo mkdir -p "$MOUNT_POINT"

sudo mount "$NBD" "$MOUNT_POINT"

echo "Synchronizing rootfs → persistent disk..."

# Remove old root filesystem content.
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

# Copy the newly built root filesystem.
sudo cp -a \
    "$ROOTFS/." \
    "$MOUNT_POINT/"

sudo sync

echo "Persistent filesystem updated."

# ============================================================
# [6/6] Prepare boot artifacts
# ============================================================

echo
echo "[6/6] Preparing boot artifacts..."

cp \
    "$KERNEL_BUILD/raccoon-kernel" \
    "$BOOT_BUILD/raccoon-kernel"

cp \
    "$INITRAMFS_BUILD/initramfs.cpio.gz" \
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
echo

# cleanup happens automatically because of trap

