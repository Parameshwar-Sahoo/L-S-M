#include "core/CatalogManager.hpp"
#include "core/CirculationManager.hpp"
#include "core/SecurityGateManager.hpp"
#include "hal/HardwareFactory.hpp"
#include "ipc/SharedMemory.hpp"
#include "ipc/SignalHandler.hpp"
#include "utils/Logger.hpp"
#include "utils/DataStore.hpp"
#include "ui/TerminalUI.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <string>

using namespace lms;

int main(int argc, char* argv[])
{
    bool runDemoOnly = false;
    bool forceSim = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--demo" || arg == "-d") {
            runDemoOnly = true;
        } else if (arg == "--sim" || arg == "-s") {
            forceSim = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  --demo, -d   Run automated demonstration and exit\n"
                      << "  --sim,  -s   Force user-space simulated hardware mock\n"
                      << "  --help, -h   Show this help message\n";
            return 0;
        }
    }

    // 1. Initialize Logging & Signals
    Logger::getInstance().init("libsys_audit.log");
    Logger::getInstance().info("SmartLMS System initializing...");
    SignalHandler::init();

    // 2. Initialize Hardware Layer (Linux Device Driver or Simulated Mock)
    auto hardware = HardwareFactory::create(forceSim);
    Logger::getInstance().info("Hardware Abstraction Layer attached: " + hardware->getDriverName());

    // 3. Initialize Core Domain Managers
    CatalogManager catalog;
    CirculationManager circulation(catalog, hardware.get());
    SecurityGateManager securityGate(catalog, circulation, hardware.get());

    // 4. Seed Initial Physical Catalog & Patron Accounts
    DataStore::seedSampleData(catalog, circulation);
    Logger::getInstance().info("Database seeded: " + std::to_string(catalog.getTotalBooks()) +
                               " titles, " + std::to_string(circulation.getTotalUsers()) + " users.");

    // 5. Initialize POSIX Shared Memory Telemetry
    SharedMemoryManager shm;
    if (shm.initMaster("/libsys_telemetry")) {
        Logger::getInstance().info("POSIX Shared Memory telemetry registered at /libsys_telemetry");
    } else {
        Logger::getInstance().warn("POSIX Shared Memory unavailable; running without external telemetry feed.");
    }

    // Hook SIGUSR1 for live diagnostic dump
    SignalHandler::setDiagnosticsCallback([&]() {
        Logger::getInstance().info("[SIGUSR1 DIAGNOSTICS] Active Loans: " +
                                   std::to_string(circulation.getActiveLoanCount()) +
                                   " | Breaches: " +
                                   std::to_string(securityGate.getSecurityBreachCount()));
    });

    // 6. Spawn Background Hardware Poller Thread
    std::thread hardwarePoller([&]() {
        while (SignalHandler::isRunning()) {
            lms_hardware_event_t event{};
            if (hardware->readEvent(event, 500)) {
                if (event.event_type == LIBSYS_EVT_DROPBOX_RETURN) {
                    std::string details;
                    circulation.processDropBoxReturnEvent(event, details);
                    Logger::getInstance().info("Hardware Event: " + details);
                } else if (event.event_type == LIBSYS_EVT_SECURITY_GATE) {
                    std::string notice;
                    securityGate.handleGateScan(event.tag_uid, event.gate_or_kiosk_id, notice);
                }
            }
        }
    });

    // 7. Spawn Background Telemetry Heartbeat Daemon
    std::thread telemetryDaemon([&]() {
        while (SignalHandler::isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            SharedTelemetry telem{};
            telem.magic                       = LIBSYS_SHM_MAGIC;
            telem.version                     = 1;
            telem.active_loans_count          = static_cast<uint32_t>(circulation.getActiveLoanCount());
            telem.total_checkouts_cumulative  = static_cast<uint32_t>(circulation.getTotalCheckouts());
            telem.total_returns_cumulative    = static_cast<uint32_t>(circulation.getTotalReturns());
            telem.security_breaches_total     = static_cast<uint32_t>(securityGate.getSecurityBreachCount());
            telem.registered_users_count      = static_cast<uint32_t>(circulation.getTotalUsers());
            telem.catalog_total_books         = static_cast<uint32_t>(catalog.getTotalBooks());
            telem.driver_connected            = hardware->isConnected() ? 1 : 0;
            telem.alarm_sounding              = securityGate.isAlarmActive() ? 1 : 0;
            telem.system_healthy              = 1;

            bool lockState = true;
            hardware->getDropBoxLock(lockState);
            telem.dropbox_unlocked = lockState ? 0 : 1;

            shm.update(telem);
        }
    });

    // 8. Launch User Interface
    TerminalUI ui(catalog, circulation, securityGate, hardware.get(), &shm);

    if (runDemoOnly) {
        ui.runAutomatedDemo(false);
        SignalHandler::requestShutdown();
    } else {
        ui.runInteractiveLoop();
    }

    // 9. Graceful Shutdown Routine
    Logger::getInstance().info("SmartLMS shutting down: joining worker threads...");

    if (hardwarePoller.joinable()) {
        hardwarePoller.join();
    }
    if (telemetryDaemon.joinable()) {
        telemetryDaemon.join();
    }

    // Persist final states to data/
    DataStore::saveCatalogToJson(catalog, "data/catalog.json");
    DataStore::saveUsersToJson(circulation, "data/users.json");

    shm.closeShm();
    Logger::getInstance().info("SmartLMS terminated cleanly. All resources released.");

    return 0;
}
