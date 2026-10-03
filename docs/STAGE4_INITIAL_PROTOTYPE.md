# Stage 4: Initial Implementation & Prototype
## Kernel Module, Hardware Abstraction, and C++ Core Integration
### Automated Smart Library Management System (SmartLMS)

---

## 1. Prototype Scope & Objectives

The primary focus of **Stage 4** is establishing the foundational engineering plumbing between the **Linux Kernel Character Device Driver** (`/dev/libsys_driver`) and the **Modern C++ Application Domain Engine**.

Key milestones achieved in this stage:
1. **Linux Character Device Driver (`driver/libsys_driver.c`)**:
   - Dynamic major/minor allocation using `alloc_chrdev_region()`.
   - Circular FIFO buffer with 64-event capacity protected by kernel `struct mutex`.
   - Blocking `read()` using `wait_event_interruptible()` and sleep-wake signaling via `wake_up_interruptible()`.
   - Custom IOCTL dispatcher for actuator states (Drop-box lock, Security siren, Shelf LEDs, and test event injection).
2. **Polymorphic Hardware Abstraction Layer (`include/hal/`)**:
   - `ILibraryHardware`: Unified C++ pure virtual interface.
   - `LinuxCharDevHardware`: Interfacing directly with `/dev/libsys_driver` via VFS system calls (`open`, `read`, `write`, `ioctl`, `poll`).
   - `SimulatedHardware`: Thread-safe mock implementation using `std::mutex`, `std::condition_variable`, and `std::queue`.
   - `HardwareFactory`: Dynamic environment sniffing that selects the kernel driver if present or seamlessly falls back to mock simulation.
3. **Core Domain Entities & Business Rules (`include/core/`, `src/core/`)**:
   - `Book`: Call numbers, shelf locations, RFID mapping, and dynamic ESL color encoding.
   - `User`: Quota enforcement based on patron role (Student, Faculty, Librarian, Guest) and fine ceilings.
   - `LoanTransaction`: Due date calculation, overdue detection, and late fee assessment.
   - `CatalogManager`: Fast multi-key hash indexing (`unordered_map`) by ISBN, RFID, and title/author search.
   - `CirculationManager`: Atomic checkouts, drop-box returns, and hardware solenoid actuation.
   - `SecurityGateManager`: Exit gate anti-theft validation against active loan registry.

---

## 2. Kernel Driver Implementation Details

### Ring Buffer & Concurrency Primitives
The kernel driver maintains a fixed-size ring buffer:
```c
struct libsys_dev_state {
    struct cdev cdev;
    dev_t dev_num;
    struct class *dev_class;
    struct device *dev_device;
    struct mutex lock;
    wait_queue_head_t read_wait;
    lms_hardware_event_t ring_buffer[LIBSYS_BUFFER_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    ...
};
```

When an RFID scanner gate fires an interrupt or event injection occurs:
```c
static int internal_enqueue_event(struct libsys_dev_state *dev, const lms_hardware_event_t *ev)
{
    if (dev->count >= LIBSYS_BUFFER_CAPACITY) {
        dev->dropped_events++;
        return -ENOSPC;
    }
    dev->ring_buffer[dev->head] = *ev;
    dev->head = (dev->head + 1) % LIBSYS_BUFFER_CAPACITY;
    dev->count++;
    dev->total_events++;
    wake_up_interruptible(&dev->read_wait);
    return 0;
}
```

The user-space worker thread blocks in `read()` without consuming CPU cycles:
```c
ret = wait_event_interruptible(dev->read_wait, (dev->count > 0) || dev->is_closing);
```

---

## 3. Hardware Abstraction Layer (Dual-Mode Design)

To ensure maximum portability and testability across developer laptops, cloud CI/CD pipelines, and physical embedded targets, the application abstracts hardware behind `ILibraryHardware`:

```
                    ILibraryHardware (C++ Interface)
                               ▲
                ┌──────────────┴──────────────┐
                │                             │
     LinuxCharDevHardware             SimulatedHardware
  (Interacts with /dev/libsys)     (In-Memory Thread-Safe Mock)
```

The `HardwareFactory` automatically handles selection:
```cpp
auto hardware = HardwareFactory::create();
std::cout << "Hardware backend: " << hardware->getDriverName() << std::endl;
```

---

## 4. Initial Prototype Verification

The core modules were integrated and verified in a prototype integration harness:
1. Catalog populated with 8 standard computer science reference texts.
2. Users registered across Student, Faculty, Librarian, and Guest roles.
3. Kiosk checkout flow executed:
   - Book `978-1593272203` (The Linux Programming Interface) checked out to Student `U101` (Alex Chen).
   - Book status transitioned from `AVAILABLE` to `CHECKED_OUT`.
   - Transaction `TX-1001` created with 14-day due date.
4. Security gate exit test:
   - Scanned tag `RFID-LPI-001` (checked out): Authorized exit confirmed, alarm remains silent.
   - Scanned tag `RFID-LDD-003` (unborrowed book): Security gate immediately triggers theft alarm via IOCTL (`LMS_IOCTL_SET_SECURITY_GATE`), alarm logged.
5. Drop-Box Return test:
   - Scanned tag `RFID-LPI-001` at return chute: Chute solenoid unlocked, active loan marked returned, book status changed to `RETURNED_DROPBOX`.

---

## 5. Development Progress, Issues & Solutions Log

| Issue ID | Problem Encountered | Root Cause Analysis | Engineering Solution Applied |
|---|---|---|---|
| **ISS-01** | Kernel module build failure on WSL2 / container environments lacking `/lib/modules/$(uname -r)/build`. | Standard WSL2 kernels omit kbuild headers unless explicitly cloned and installed. | Implemented a dual-path `driver/Makefile` and C++ `HardwareFactory` fallback that detects missing device nodes and auto-selects `SimulatedHardware`. |
| **ISS-02** | Race condition between driver reader thread and IOCTL reset buffer. | Ring buffer head/tail indices modified without locking across simultaneous read/ioctl. | Enforced uniform `mutex_lock_interruptible(&dev->lock)` across all file operations (`read`, `write`, `ioctl`, `poll`). |
| **ISS-03** | Secondary index desynchronization in `CatalogManager` on book updates. | Updating a book's RFID tag left stale pointer in `m_rfidIndex`. | Added explicit cleanup of prior RFID key before mapping the updated tag in `updateBook()` and `removeBook()`. |
| **ISS-04** | Solenoid flap remaining unlocked indefinitely if application crashes. | Flap was left in open state awaiting user acknowledgment. | Enforced default hardware timeout relock policy and RAII destructor cleanup. |
