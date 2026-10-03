#include "hal/SimulatedHardware.hpp"
#include <chrono>

namespace lms {

SimulatedHardware::SimulatedHardware()
    : m_connected(false)
    , m_dropBoxLocked(true)
    , m_alarmActive(false)
    , m_shelfLedColor(0)
    , m_totalEvents(0)
    , m_droppedEvents(0)
{
}

SimulatedHardware::~SimulatedHardware()
{
    disconnect();
}

bool SimulatedHardware::connect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = true;
    return true;
}

void SimulatedHardware::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
    m_cv.notify_all();
}

bool SimulatedHardware::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

bool SimulatedHardware::readEvent(lms_hardware_event_t& outEvent, int timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_connected) {
        return false;
    }

    if (m_queue.empty()) {
        if (timeoutMs <= 0) {
            return false;
        }
        bool waitRes = m_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return !m_queue.empty() || !m_connected;
        });

        if (!waitRes || !m_connected || m_queue.empty()) {
            return false;
        }
    }

    outEvent = m_queue.front();
    m_queue.pop();
    return true;
}

bool SimulatedHardware::setDropBoxLock(bool locked)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dropBoxLocked = locked;
    return true;
}

bool SimulatedHardware::getDropBoxLock(bool& outLocked)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    outLocked = m_dropBoxLocked;
    return true;
}

bool SimulatedHardware::setSecurityGateAlarm(bool active)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alarmActive = active;
    return true;
}

bool SimulatedHardware::getSecurityGateAlarm(bool& outActive)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    outActive = m_alarmActive;
    return true;
}

bool SimulatedHardware::setShelfLED(uint16_t aisle, uint16_t rack, uint32_t colorRgb)
{
    (void)aisle;
    (void)rack;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shelfLedColor = colorRgb;
    return true;
}

bool SimulatedHardware::simulateScan(const lms_hardware_event_t& event)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.size() >= LIBSYS_BUFFER_CAPACITY) {
        m_droppedEvents++;
        return false;
    }

    m_queue.push(event);
    m_totalEvents++;
    m_cv.notify_one();
    return true;
}

bool SimulatedHardware::getStatus(lms_driver_status_t& outStatus)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    outStatus.total_events_logged     = m_totalEvents;
    outStatus.buffer_occupancy        = static_cast<uint32_t>(m_queue.size());
    outStatus.buffer_capacity         = LIBSYS_BUFFER_CAPACITY;
    outStatus.dropped_events          = m_droppedEvents;
    outStatus.dropbox_solenoid_locked = m_dropBoxLocked ? 1 : 0;
    outStatus.security_alarm_active   = m_alarmActive ? 1 : 0;
    outStatus.device_ready            = m_connected ? 1 : 0;
    return true;
}

bool SimulatedHardware::resetBuffer()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
    m_droppedEvents = 0;
    return true;
}

std::string SimulatedHardware::getDriverName() const
{
    return "User-Space Simulated Mock Hardware Engine";
}

} // namespace lms
