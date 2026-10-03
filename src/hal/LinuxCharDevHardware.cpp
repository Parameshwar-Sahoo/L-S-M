#include "hal/LinuxCharDevHardware.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <cstring>
#include <iostream>

namespace lms {

LinuxCharDevHardware::LinuxCharDevHardware(std::string devicePath)
    : m_devicePath(std::move(devicePath))
    , m_fd(-1)
    , m_connected(false)
{
}

LinuxCharDevHardware::~LinuxCharDevHardware()
{
    disconnect();
}

bool LinuxCharDevHardware::connect()
{
    if (m_connected && m_fd >= 0) {
        return true;
    }

    m_fd = ::open(m_devicePath.c_str(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        m_connected = false;
        return false;
    }

    m_connected = true;
    return true;
}

void LinuxCharDevHardware::disconnect()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_connected = false;
}

bool LinuxCharDevHardware::isConnected() const
{
    return m_connected && (m_fd >= 0);
}

bool LinuxCharDevHardware::readEvent(lms_hardware_event_t& outEvent, int timeoutMs)
{
    if (!isConnected()) {
        return false;
    }

    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int pollRet = ::poll(&pfd, 1, timeoutMs);
    if (pollRet <= 0 || !(pfd.revents & POLLIN)) {
        return false;
    }

    ssize_t bytesRead = ::read(m_fd, &outEvent, sizeof(lms_hardware_event_t));
    return (bytesRead == sizeof(lms_hardware_event_t));
}

bool LinuxCharDevHardware::setDropBoxLock(bool locked)
{
    if (!isConnected()) return false;

    lms_actuator_cmd_t cmd{};
    cmd.terminal_id = 1;
    cmd.state = locked ? 1 : 0;

    return (::ioctl(m_fd, LMS_IOCTL_SET_DROPBOX_LOCK, &cmd) == 0);
}

bool LinuxCharDevHardware::getDropBoxLock(bool& outLocked)
{
    if (!isConnected()) return false;

    lms_actuator_cmd_t cmd{};
    cmd.terminal_id = 1;

    if (::ioctl(m_fd, LMS_IOCTL_GET_DROPBOX_LOCK, &cmd) == 0) {
        outLocked = (cmd.state != 0);
        return true;
    }
    return false;
}

bool LinuxCharDevHardware::setSecurityGateAlarm(bool active)
{
    if (!isConnected()) return false;

    lms_actuator_cmd_t cmd{};
    cmd.terminal_id = 1;
    cmd.state = active ? 1 : 0;

    return (::ioctl(m_fd, LMS_IOCTL_SET_SECURITY_GATE, &cmd) == 0);
}

bool LinuxCharDevHardware::getSecurityGateAlarm(bool& outActive)
{
    if (!isConnected()) return false;

    lms_actuator_cmd_t cmd{};
    cmd.terminal_id = 1;

    if (::ioctl(m_fd, LMS_IOCTL_GET_SECURITY_GATE, &cmd) == 0) {
        outActive = (cmd.state != 0);
        return true;
    }
    return false;
}

bool LinuxCharDevHardware::setShelfLED(uint16_t aisle, uint16_t rack, uint32_t colorRgb)
{
    if (!isConnected()) return false;

    lms_shelf_led_cmd_t cmd{};
    cmd.aisle_id = aisle;
    cmd.rack_id = rack;
    cmd.color_rgb = colorRgb;

    return (::ioctl(m_fd, LMS_IOCTL_SET_SHELF_LED, &cmd) == 0);
}

bool LinuxCharDevHardware::simulateScan(const lms_hardware_event_t& event)
{
    if (!isConnected()) return false;

    return (::ioctl(m_fd, LMS_IOCTL_SIMULATE_EVENT, &event) == 0);
}

bool LinuxCharDevHardware::getStatus(lms_driver_status_t& outStatus)
{
    if (!isConnected()) return false;

    return (::ioctl(m_fd, LMS_IOCTL_GET_STATUS, &outStatus) == 0);
}

bool LinuxCharDevHardware::resetBuffer()
{
    if (!isConnected()) return false;

    return (::ioctl(m_fd, LMS_IOCTL_RESET_BUFFER) == 0);
}

std::string LinuxCharDevHardware::getDriverName() const
{
    return "Linux Character Device Driver (" + m_devicePath + ")";
}

} // namespace lms
