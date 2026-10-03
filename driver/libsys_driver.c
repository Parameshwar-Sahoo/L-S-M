#include "libsys_driver.h"
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);

static struct libsys_dev_state *libsys_dev = NULL;

static int libsys_open(struct inode *inode, struct file *filp)
{
    filp->private_data = libsys_dev;
    pr_info("libsys_driver: Device opened by PID %d\n", current->pid);
    return 0;
}

static int libsys_release(struct inode *inode, struct file *filp)
{
    pr_info("libsys_driver: Device released by PID %d\n", current->pid);
    return 0;
}

static ssize_t libsys_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct libsys_dev_state *dev = filp->private_data;
    lms_hardware_event_t event;
    int ret;

    if (!dev)
        return -ENODEV;

    if (count < sizeof(lms_hardware_event_t))
        return -EINVAL;

    if (filp->f_flags & O_NONBLOCK) {
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        if (dev->count == 0) {
            mutex_unlock(&dev->lock);
            return -EAGAIN;
        }
    } else {
        ret = wait_event_interruptible(dev->read_wait, (dev->count > 0) || dev->is_closing);
        if (ret != 0)
            return -ERESTARTSYS;

        if (dev->is_closing)
            return -ESHUTDOWN;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
    }

    if (dev->count == 0) {
        mutex_unlock(&dev->lock);
        return 0;
    }

    /* Dequeue event from circular FIFO */
    event = dev->ring_buffer[dev->tail];
    dev->tail = (dev->tail + 1) % LIBSYS_BUFFER_CAPACITY;
    dev->count--;

    mutex_unlock(&dev->lock);

    if (copy_to_user(buf, &event, sizeof(lms_hardware_event_t)))
        return -EFAULT;

    return sizeof(lms_hardware_event_t);
}

static int internal_enqueue_event(struct libsys_dev_state *dev, const lms_hardware_event_t *ev)
{
    if (dev->count >= LIBSYS_BUFFER_CAPACITY) {
        dev->dropped_events++;
        pr_warn("libsys_driver: Buffer overflow! Event dropped. Total dropped: %u\n",
                dev->dropped_events);
        return -ENOSPC;
    }

    dev->ring_buffer[dev->head] = *ev;
    dev->head = (dev->head + 1) % LIBSYS_BUFFER_CAPACITY;
    dev->count++;
    dev->total_events++;

    /* Wake up waiting reader thread */
    wake_up_interruptible(&dev->read_wait);
    return 0;
}

static ssize_t libsys_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct libsys_dev_state *dev = filp->private_data;
    lms_hardware_event_t event;
    int ret;

    if (!dev)
        return -ENODEV;

    if (count < sizeof(lms_hardware_event_t))
        return -EINVAL;

    if (copy_from_user(&event, buf, sizeof(lms_hardware_event_t)))
        return -EFAULT;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    ret = internal_enqueue_event(dev, &event);
    mutex_unlock(&dev->lock);

    return (ret == 0) ? sizeof(lms_hardware_event_t) : ret;
}

static __poll_t libsys_poll(struct file *filp, struct poll_table_struct *wait)
{
    struct libsys_dev_state *dev = filp->private_data;
    __poll_t mask = 0;

    if (!dev)
        return EPOLLERR;

    poll_wait(filp, &dev->read_wait, wait);

    if (mutex_lock_interruptible(&dev->lock))
        return EPOLLERR;

    if (dev->count > 0)
        mask |= (EPOLLIN | EPOLLRDNORM);

    if (dev->count < LIBSYS_BUFFER_CAPACITY)
        mask |= (EPOLLOUT | EPOLLWRNORM);

    mutex_unlock(&dev->lock);
    return mask;
}

static long libsys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct libsys_dev_state *dev = filp->private_data;
    int ret = 0;

    if (!dev)
        return -ENODEV;

    switch (cmd) {
    case LMS_IOCTL_GET_STATUS: {
        lms_driver_status_t status;
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        status.total_events_logged     = dev->total_events;
        status.buffer_occupancy        = dev->count;
        status.buffer_capacity         = LIBSYS_BUFFER_CAPACITY;
        status.dropped_events          = dev->dropped_events;
        status.dropbox_solenoid_locked = dev->dropbox_solenoid_locked;
        status.security_alarm_active   = dev->security_alarm_active;
        status.device_ready            = 1;

        mutex_unlock(&dev->lock);

        if (copy_to_user((void __user *)arg, &status, sizeof(status)))
            return -EFAULT;
        break;
    }

    case LMS_IOCTL_SET_DROPBOX_LOCK: {
        lms_actuator_cmd_t cmd_data;
        if (copy_from_user(&cmd_data, (void __user *)arg, sizeof(cmd_data)))
            return -EFAULT;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        dev->dropbox_solenoid_locked = (cmd_data.state != 0) ? 1 : 0;
        pr_info("libsys_driver: DropBox Solenoid state set to: %s\n",
                dev->dropbox_solenoid_locked ? "LOCKED" : "UNLOCKED");

        mutex_unlock(&dev->lock);
        break;
    }

    case LMS_IOCTL_GET_DROPBOX_LOCK: {
        lms_actuator_cmd_t cmd_data;
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        cmd_data.terminal_id = 1;
        cmd_data.state = dev->dropbox_solenoid_locked;
        mutex_unlock(&dev->lock);

        if (copy_to_user((void __user *)arg, &cmd_data, sizeof(cmd_data)))
            return -EFAULT;
        break;
    }

    case LMS_IOCTL_SET_SECURITY_GATE: {
        lms_actuator_cmd_t cmd_data;
        if (copy_from_user(&cmd_data, (void __user *)arg, sizeof(cmd_data)))
            return -EFAULT;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        dev->security_alarm_active = (cmd_data.state != 0) ? 1 : 0;
        if (dev->security_alarm_active) {
            pr_warn("libsys_driver: *** THEFT SECURITY ALARM TRIGGERED (Gate %u) ***\n",
                    cmd_data.terminal_id);
        } else {
            pr_info("libsys_driver: Security alarm silenced (Gate %u)\n",
                    cmd_data.terminal_id);
        }

        mutex_unlock(&dev->lock);
        break;
    }

    case LMS_IOCTL_GET_SECURITY_GATE: {
        lms_actuator_cmd_t cmd_data;
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        cmd_data.terminal_id = 1;
        cmd_data.state = dev->security_alarm_active;
        mutex_unlock(&dev->lock);

        if (copy_to_user((void __user *)arg, &cmd_data, sizeof(cmd_data)))
            return -EFAULT;
        break;
    }

    case LMS_IOCTL_SET_SHELF_LED: {
        lms_shelf_led_cmd_t led_cmd;
        if (copy_from_user(&led_cmd, (void __user *)arg, sizeof(led_cmd)))
            return -EFAULT;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        dev->active_shelf_led = led_cmd.color_rgb;
        pr_info("libsys_driver: Shelf LED lit: Aisle %u, Rack %u -> Color #%06X\n",
                led_cmd.aisle_id, led_cmd.rack_id, led_cmd.color_rgb);

        mutex_unlock(&dev->lock);
        break;
    }

    case LMS_IOCTL_SIMULATE_EVENT: {
        lms_hardware_event_t event;
        if (copy_from_user(&event, (void __user *)arg, sizeof(event)))
            return -EFAULT;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        ret = internal_enqueue_event(dev, &event);
        mutex_unlock(&dev->lock);
        break;
    }

    case LMS_IOCTL_RESET_BUFFER: {
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;

        dev->head = 0;
        dev->tail = 0;
        dev->count = 0;
        dev->dropped_events = 0;
        pr_info("libsys_driver: Ring buffer reset successfully\n");

        mutex_unlock(&dev->lock);
        break;
    }

    default:
        pr_warn("libsys_driver: Invalid IOCTL command 0x%x\n", cmd);
        return -ENOTTY;
    }

    return ret;
}

static const struct file_operations libsys_fops = {
    .owner          = THIS_MODULE,
    .open           = libsys_open,
    .release        = libsys_release,
    .read           = libsys_read,
    .write          = libsys_write,
    .poll           = libsys_poll,
    .unlocked_ioctl = libsys_ioctl,
};

static int __init libsys_driver_init(void)
{
    int ret;

    pr_info("libsys_driver: Initializing Smart Library Character Device Driver v%s\n",
            DRIVER_VERSION);

    libsys_dev = kzalloc(sizeof(struct libsys_dev_state), GFP_KERNEL);
    if (!libsys_dev) {
        pr_err("libsys_driver: Failed to allocate kernel memory for device state\n");
        return -ENOMEM;
    }

    mutex_init(&libsys_dev->lock);
    init_waitqueue_head(&libsys_dev->read_wait);

    libsys_dev->dropbox_solenoid_locked = 1; /* Locked by default */
    libsys_dev->security_alarm_active   = 0;
    libsys_dev->is_closing              = false;

    ret = alloc_chrdev_region(&libsys_dev->dev_num, 0, 1, LIBSYS_DEVICE_NAME);
    if (ret < 0) {
        pr_err("libsys_driver: Failed to allocate major/minor number\n");
        goto err_kfree;
    }

    cdev_init(&libsys_dev->cdev, &libsys_fops);
    libsys_dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&libsys_dev->cdev, libsys_dev->dev_num, 1);
    if (ret < 0) {
        pr_err("libsys_driver: Failed to add cdev\n");
        goto err_unregister_chrdev;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    libsys_dev->dev_class = class_create(LIBSYS_DEVICE_NAME);
#else
    libsys_dev->dev_class = class_create(THIS_MODULE, LIBSYS_DEVICE_NAME);
#endif

    if (IS_ERR(libsys_dev->dev_class)) {
        pr_err("libsys_driver: Failed to create device class\n");
        ret = PTR_ERR(libsys_dev->dev_class);
        goto err_cdev_del;
    }

    libsys_dev->dev_device = device_create(libsys_dev->dev_class, NULL,
                                           libsys_dev->dev_num, NULL,
                                           LIBSYS_DEVICE_NAME);
    if (IS_ERR(libsys_dev->dev_device)) {
        pr_err("libsys_driver: Failed to create device node /dev/%s\n", LIBSYS_DEVICE_NAME);
        ret = PTR_ERR(libsys_dev->dev_device);
        goto err_class_destroy;
    }

    pr_info("libsys_driver: Device registered successfully: /dev/%s [Major %d, Minor %d]\n",
            LIBSYS_DEVICE_NAME, MAJOR(libsys_dev->dev_num), MINOR(libsys_dev->dev_num));

    return 0;

err_class_destroy:
    class_destroy(libsys_dev->dev_class);
err_cdev_del:
    cdev_del(&libsys_dev->cdev);
err_unregister_chrdev:
    unregister_chrdev_region(libsys_dev->dev_num, 1);
err_kfree:
    kfree(libsys_dev);
    return ret;
}

static void __exit libsys_driver_exit(void)
{
    pr_info("libsys_driver: Unloading Smart Library Driver...\n");

    if (libsys_dev) {
        libsys_dev->is_closing = true;
        wake_up_all(&libsys_dev->read_wait);

        device_destroy(libsys_dev->dev_class, libsys_dev->dev_num);
        class_destroy(libsys_dev->dev_class);
        cdev_del(&libsys_dev->cdev);
        unregister_chrdev_region(libsys_dev->dev_num, 1);
        kfree(libsys_dev);
        libsys_dev = NULL;
    }

    pr_info("libsys_driver: Device unregistered cleanly\n");
}

module_init(libsys_driver_init);
module_exit(libsys_driver_exit);
