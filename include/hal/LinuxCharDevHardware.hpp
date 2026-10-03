#ifndef LINUX_CHAR_DEV_HARDWARE_HPP
#define LINUX_CHAR_DEV_HARDWARE_HPP

#include "ILibraryHardware.hpp"
#include <string>

namespace lms {

class LinuxCharDevHardware : public ILibraryHardware {
public:
    explicit LinuxCharDevHardware(std::string devicePath = LIBSYS_DEVICE_PATH);
    ~LinuxCharDevHardware() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    bool readEvent(lms_hardware_event_t& outEvent, int timeoutMs = 1000) override;
    bool setDropBoxLock(bool locked) override;
    bool getDropBoxLock(bool& outLocked) override;
    bool setSecurityGateAlarm(bool active) override;
    bool getSecurityGateAlarm(bool& outActive) override;
    bool setShelfLED(uint16_t aisle, uint16_t rack, uint32_t colorRgb) override;
    virtual bool simulateScan(const lms_hardware_event_t& event) override;
    bool getStatus(lms_driver_status_t& outStatus) override;
    bool resetBuffer() override;
    std::string getDriverName() const override;

private:
    std::string m_devicePath;
    int m_fd;
    bool m_connected;
};

} // namespace lms

#endif // LINUX_CHAR_DEV_HARDWARE_HPP
