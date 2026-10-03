#!/bin/bash
echo "=========================================================="
echo "    Unloading SmartLMS Linux Kernel Device Driver         "
echo "=========================================================="

if lsmod | grep -q "libsys_driver"; then
    echo "Removing kernel module libsys_driver..."
    sudo rmmod libsys_driver
    echo "Module removed successfully."
else
    echo "libsys_driver is not currently loaded in the kernel."
fi
