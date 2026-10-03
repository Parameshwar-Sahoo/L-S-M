#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER_DIR="$SCRIPT_DIR/../driver"

echo "=========================================================="
echo "    Loading SmartLMS Linux Kernel Device Driver           "
echo "=========================================================="

cd "$DRIVER_DIR"

if [ ! -f libsys_driver.ko ]; then
    echo "Compiling libsys_driver.ko..."
    make all
fi

if [ -f libsys_driver.ko ]; then
    echo "Inserting kernel module..."
    sudo insmod libsys_driver.ko || {
        echo "Module already inserted or failed. Checking dmesg..."
        dmesg | tail -n 10
    }
    sudo chmod 666 /dev/libsys_driver 2>/dev/null || true
    echo "Driver node created at /dev/libsys_driver"
    ls -l /dev/libsys_driver
else
    echo "Kernel headers missing or compilation skipped."
    echo "SmartLMS will continue seamlessly using SimulatedHardware fallback."
fi
