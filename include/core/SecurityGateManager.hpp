#ifndef SECURITY_GATE_MANAGER_HPP
#define SECURITY_GATE_MANAGER_HPP

#include "CatalogManager.hpp"
#include "CirculationManager.hpp"
#include "hal/ILibraryHardware.hpp"
#include <mutex>
#include <string>

namespace lms {

class SecurityGateManager {
public:
    SecurityGateManager(CatalogManager& catalog, CirculationManager& circulation, ILibraryHardware* hardware = nullptr);

    void setHardware(ILibraryHardware* hardware);

    /**
     * Evaluates an RFID tag passing through a library physical security exit gate
     * Returns true if passage is authorized, false if security breach triggered
     */
    bool handleGateScan(const std::string& rfidTag, uint8_t gateId, std::string& outNotice);

    void silenceAlarm(uint8_t gateId = 1);
    bool isAlarmActive() const;
    size_t getSecurityBreachCount() const;

private:
    CatalogManager& m_catalog;
    CirculationManager& m_circulation;
    ILibraryHardware* m_hardware;

    mutable std::mutex m_mutex;
    bool m_alarmActive{false};
    size_t m_securityBreachCount{0};
};

} // namespace lms

#endif // SECURITY_GATE_MANAGER_HPP
