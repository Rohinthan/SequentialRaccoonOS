#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

KERNEL="$PROJECT_ROOT/build/boot/v0.1/raccoon-kernel"
INITRAMFS="$PROJECT_ROOT/build/boot/v0.1/initramfs.cpio.gz"

if [ ! -f "$KERNEL" ]; then
    echo "Kernel not found. Run ./scripts/build.sh first."
    exit 1
fi

if [ ! -f "$INITRAMFS" ]; then
    echo "Initramfs not found. Run ./scripts/build.sh first."
    exit 1
fi

exec qemu-system-x86_64 \
    -enable-kvm \
    -m 512M \
    -smp 2 \
    -kernel "$KERNEL" \
    -initrd "$INITRAMFS" \
    -drive file="$PROJECT_ROOT/build/disk/v0.1/raccoon-root.qcow2",if=virtio,format=qcow2 \
    -append "console=ttyS0 root=/dev/vda rw" \
    -nographic
