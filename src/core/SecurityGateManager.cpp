#include "core/SecurityGateManager.hpp"
#include <iostream>

namespace lms {

SecurityGateManager::SecurityGateManager(CatalogManager& catalog,
                                         CirculationManager& circulation,
                                         ILibraryHardware* hardware)
    : m_catalog(catalog)
    , m_circulation(circulation)
    , m_hardware(hardware)
{
}

void SecurityGateManager::setHardware(ILibraryHardware* hardware)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hardware = hardware;
}

bool SecurityGateManager::handleGateScan(const std::string& rfidTag,
                                         uint8_t gateId,
                                         std::string& outNotice)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Book* book = m_catalog.findByRfid(rfidTag);
    if (!book) {
        // Unknown RFID tag passing gate
        outNotice = "Gate " + std::to_string(gateId) + ": Unregistered RFID tag (" + rfidTag + ") passed gate.";
        return true; // Not recognized as a catalog asset
    }

    if (book->isCheckedOut()) {
        // Legitimate checkout passing exit gate
        outNotice = "Gate " + std::to_string(gateId) + ": Authorized departure of '" +
                    book->getTitle() + "' [ISBN: " + book->getIsbn() + "].";
        return true;
    }

    // SECURITY BREACH: Book is AVAILABLE, RESERVED, or UNDER_RESHELVING
    m_securityBreachCount++;
    m_alarmActive = true;

    outNotice = "Gate " + std::to_string(gateId) + ": *** THEFT BREACH ALERT *** Unauthorized removal of '" +
                book->getTitle() + "' (Status: " + book->getStatusString() + ")!";

    if (m_hardware && m_hardware->isConnected()) {
        m_hardware->setSecurityGateAlarm(true);
    }

    return false;
}

void SecurityGateManager::silenceAlarm(uint8_t gateId)
{
    (void)gateId;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alarmActive = false;
    if (m_hardware && m_hardware->isConnected()) {
        m_hardware->setSecurityGateAlarm(false);
    }
}

bool SecurityGateManager::isAlarmActive() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_alarmActive;
}

size_t SecurityGateManager::getSecurityBreachCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_securityBreachCount;
}

} // namespace lms
