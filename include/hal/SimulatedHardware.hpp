#ifndef SIMULATED_HARDWARE_HPP
#define SIMULATED_HARDWARE_HPP

#include "ILibraryHardware.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

namespace lms {

class SimulatedHardware : public ILibraryHardware {
public:
    SimulatedHardware();
    ~SimulatedHardware() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    bool readEvent(lms_hardware_event_t& outEvent, int timeoutMs = 1000) override;
    bool setDropBoxLock(bool locked) override;
    bool getDropBoxLock(bool& outLocked) override;
    bool setSecurityGateAlarm(bool active) override;
    bool getSecurityGateAlarm(bool& outActive) override;
    bool setShelfLED(uint16_t aisle, uint16_t rack, uint32_t colorRgb) override;
    bool simulateScan(const lms_hardware_event_t& event) override;
    bool getStatus(lms_driver_status_t& outStatus) override;
    bool resetBuffer() override;
    std::string getDriverName() const override;

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<lms_hardware_event_t> m_queue;

    bool m_connected;
    bool m_dropBoxLocked;
    bool m_alarmActive;
    uint32_t m_shelfLedColor;
    uint32_t m_totalEvents;
    uint32_t m_droppedEvents;
};

} // namespace lms

#endif // SIMULATED_HARDWARE_HPP
