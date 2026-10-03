#ifndef I_LIBRARY_HARDWARE_HPP
#define I_LIBRARY_HARDWARE_HPP

#include "driver/include/libsys_ioctl.h"
#include <string>
#include <cstdint>

namespace lms {

/**
 * Interface defining hardware operations across Linux kernel chardev and simulated mock
 */
class ILibraryHardware {
public:
    virtual ~ILibraryHardware() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual bool readEvent(lms_hardware_event_t& outEvent, int timeoutMs = 1000) = 0;
    virtual bool setDropBoxLock(bool locked) = 0;
    virtual bool getDropBoxLock(bool& outLocked) = 0;
    virtual bool setSecurityGateAlarm(bool active) = 0;
    virtual bool getSecurityGateAlarm(bool& outActive) = 0;
    virtual bool setShelfLED(uint16_t aisle, uint16_t rack, uint32_t colorRgb) = 0;
    virtual bool simulateScan(const lms_hardware_event_t& event) = 0;
    virtual bool getStatus(lms_driver_status_t& outStatus) = 0;
    virtual bool resetBuffer() = 0;
    virtual std::string getDriverName() const = 0;
};

} // namespace lms

#endif // I_LIBRARY_HARDWARE_HPP
