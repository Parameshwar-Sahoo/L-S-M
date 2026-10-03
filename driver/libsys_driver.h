#ifndef LIBSYS_DRIVER_H
#define LIBSYS_DRIVER_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>

#include "include/libsys_ioctl.h"

#define DRIVER_AUTHOR "SmartLMS Core Engineering Team"
#define DRIVER_DESC   "Smart Library Automated Hardware & RFID Character Device Driver"
#define DRIVER_VERSION "1.0.0"

struct libsys_dev_state {
    struct cdev cdev;
    dev_t dev_num;
    struct class *dev_class;
    struct device *dev_device;

    /* Synchronization & Wait Queues */
    struct mutex lock;
    wait_queue_head_t read_wait;

    /* FIFO Circular Event Buffer */
    lms_hardware_event_t ring_buffer[LIBSYS_BUFFER_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;

    /* Telemetry Metrics */
    uint32_t total_events;
    uint32_t dropped_events;

    /* Actuator States */
    uint8_t dropbox_solenoid_locked;
    uint8_t security_alarm_active;
    uint32_t active_shelf_led;

    bool is_closing;
};

#endif /* LIBSYS_DRIVER_H */
