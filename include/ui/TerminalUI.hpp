#ifndef TERMINAL_UI_HPP
#define TERMINAL_UI_HPP

#include "core/CatalogManager.hpp"
#include "core/CirculationManager.hpp"
#include "core/SecurityGateManager.hpp"
#include "hal/ILibraryHardware.hpp"
#include "ipc/SharedMemory.hpp"

namespace lms {

class TerminalUI {
public:
    TerminalUI(CatalogManager& catalog,
               CirculationManager& circulation,
               SecurityGateManager& gate,
               ILibraryHardware* hardware,
               SharedMemoryManager* shm = nullptr);

    void runInteractiveLoop();
    void runAutomatedDemo(bool pauseAtEnd = true);

private:
    void printHeader() const;
    void printMainMenu() const;
    void showCatalog() const;
    void searchCatalog() const;
    void handleCheckout();
    void handleManualReturn();
    void handleDropBoxSimulation();
    void handleSecurityGateSimulation();
    void showActiveLoans() const;
    void showHardwareDiagnostics() const;
    void showAuditLog() const;

    void clearScreen() const;
    void pauseForUser() const;

    CatalogManager& m_catalog;
    CirculationManager& m_circulation;
    SecurityGateManager& m_gate;
    ILibraryHardware* m_hardware;
    SharedMemoryManager* m_shm;
};

} // namespace lms

#endif // TERMINAL_UI_HPP
