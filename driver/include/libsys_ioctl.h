#ifndef LIBSYS_IOCTL_H
#define LIBSYS_IOCTL_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#endif

#define LIBSYS_DEVICE_NAME "libsys_driver"
#define LIBSYS_DEVICE_PATH "/dev/" LIBSYS_DEVICE_NAME
#define LIBSYS_MAX_TAG_LEN 32
#define LIBSYS_BUFFER_CAPACITY 64

/* Event Types */
#define LIBSYS_EVT_KIOSK_CHECKOUT  1
#define LIBSYS_EVT_DROPBOX_RETURN  2
#define LIBSYS_EVT_SECURITY_GATE   3
#define LIBSYS_EVT_SHELF_SCAN      4

#pragma pack(push, 1)

/**
 * Hardware RFID scan event captured at gates, kiosks, or return drop-box
 */
typedef struct {
    uint32_t event_id;                     /* Monotonically increasing event ID */
    uint64_t timestamp_ns;                 /* Hardware timestamp in nanoseconds */
    char     tag_uid[LIBSYS_MAX_TAG_LEN];  /* RFID / NFC Tag UID or ISBN */
    char     user_card_uid[LIBSYS_MAX_TAG_LEN]; /* Patron membership card UID (if kiosk) */
    uint8_t  event_type;                   /* 1: KIOSK, 2: DROPBOX, 3: GATE, 4: SHELF */
    uint8_t  gate_or_kiosk_id;             /* Terminal identifier (1 to 8) */
    uint8_t  signal_strength;              /* RSSI signal strength (0 to 100) */
    uint8_t  status_flags;                 /* Bitmask: 0x01 = Solenoid Engaged, 0x02 = Alarm Active */
} lms_hardware_event_t;

/**
 * Driver status and runtime statistics
 */
typedef struct {
    uint32_t total_events_logged;          /* Total events processed since driver load */
    uint32_t buffer_occupancy;             /* Current items in the ring buffer */
    uint32_t buffer_capacity;              /* Total capacity of ring buffer */
    uint32_t dropped_events;               /* Count of dropped events due to overflow */
    uint8_t  dropbox_solenoid_locked;      /* 1 = Solenoid locked, 0 = Solenoid open */
    uint8_t  security_alarm_active;        /* 1 = Alarm sounding, 0 = Normal */
    uint8_t  device_ready;                 /* 1 if healthy, 0 if fault state */
} lms_driver_status_t;

/**
 * Actuator actuation command (Drop-box lock or Security gate siren)
 */
typedef struct {
    uint32_t terminal_id;                  /* Terminal identifier */
    uint8_t  state;                        /* 1 = Engaged/Active, 0 = Released/Muted */
} lms_actuator_cmd_t;

/**
 * Shelf LED illumination command for patron guidance
 */
typedef struct {
    uint16_t aisle_id;                     /* Aisle number (1 to 100) */
    uint16_t rack_id;                      /* Rack number (1 to 50) */
    uint32_t color_rgb;                    /* 0x00RRGGBB (e.g. 0x0000FF = Blue, 0x00FF00 = Green) */
} lms_shelf_led_cmd_t;

#pragma pack(pop)

/* IOCTL Definitions using 'L' magic number */
#define LMS_IOC_MAGIC 'L'

/* Read current driver status and telemetry */
#define LMS_IOCTL_GET_STATUS          _IOR(LMS_IOC_MAGIC, 1, lms_driver_status_t)

/* Actuate smart drop-box solenoid lock */
#define LMS_IOCTL_SET_DROPBOX_LOCK    _IOW(LMS_IOC_MAGIC, 2, lms_actuator_cmd_t)

/* Query drop-box solenoid state */
#define LMS_IOCTL_GET_DROPBOX_LOCK    _IOWR(LMS_IOC_MAGIC, 3, lms_actuator_cmd_t)

/* Trigger or silence security anti-theft gate alarm */
#define LMS_IOCTL_SET_SECURITY_GATE   _IOW(LMS_IOC_MAGIC, 4, lms_actuator_cmd_t)

/* Query security gate alarm state */
#define LMS_IOCTL_GET_SECURITY_GATE   _IOWR(LMS_IOC_MAGIC, 5, lms_actuator_cmd_t)

/* Set Electronic Shelf Label / Rack LED color */
#define LMS_IOCTL_SET_SHELF_LED       _IOW(LMS_IOC_MAGIC, 6, lms_shelf_led_cmd_t)

/* Inject simulated hardware scan event directly into kernel ring buffer */
#define LMS_IOCTL_SIMULATE_EVENT      _IOW(LMS_IOC_MAGIC, 7, lms_hardware_event_t)

/* Reset kernel ring buffer and reset metrics */
#define LMS_IOCTL_RESET_BUFFER        _IO(LMS_IOC_MAGIC, 8)

#endif /* LIBSYS_IOCTL_H */
