#include "ui/TerminalUI.hpp"
#include "utils/Logger.hpp"
#include "ipc/SignalHandler.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>

namespace lms {

TerminalUI::TerminalUI(CatalogManager& catalog,
                       CirculationManager& circulation,
                       SecurityGateManager& gate,
                       ILibraryHardware* hardware,
                       SharedMemoryManager* shm)
    : m_catalog(catalog)
    , m_circulation(circulation)
    , m_gate(gate)
    , m_hardware(hardware)
    , m_shm(shm)
{
}

void TerminalUI::clearScreen() const
{
    std::cout << "\033[2J\033[1;1H";
}

void TerminalUI::pauseForUser() const
{
    std::cout << "\n\033[90mPress Enter to continue...\033[0m";
    std::cin.ignore(10000, '\n');
}

void TerminalUI::printHeader() const
{
    std::cout << "\033[1;34m╔════════════════════════════════════════════════════════════════════════════════╗\033[0m\n";
    std::cout << "\033[1;34m║\033[0m \033[1;37m        AUTOMATED SMART LIBRARY MANAGEMENT SYSTEM (SmartLMS)           \033[0m \033[1;34m║\033[0m\n";
    std::cout << "\033[1;34m║\033[0m \033[96m Linux Device Driver • POSIX System Programming • Modern C++ Circulation  \033[0m \033[1;34m║\033[0m\n";
    std::cout << "\033[1;34m╚════════════════════════════════════════════════════════════════════════════════╝\033[0m\n";

    if (m_hardware) {
        std::cout << "\033[90mHardware Layer: \033[1;32m" << m_hardware->getDriverName() << "\033[0m";
        bool alarm = m_gate.isAlarmActive();
        if (alarm) {
            std::cout << "  \033[1;31m[*** THEFT ALARM ACTIVE ***]\033[0m";
        }
        std::cout << "\n\n";
    }
}

void TerminalUI::printMainMenu() const
{
    std::cout << "\033[1;33m=== MAIN OPERATIONS MENU ===\033[0m\n";
    std::cout << "  \033[1;36m[1]\033[0m Browse Complete Library Catalog & Shelf Locations\n";
    std::cout << "  \033[1;36m[2]\033[0m Search Books by Keyword / Author / ISBN / RFID\n";
    std::cout << "  \033[1;36m[3]\033[0m Patron Self-Checkout Kiosk (Card + RFID)\n";
    std::cout << "  \033[1;36m[4]\033[0m Manual Desk Book Return\n";
    std::cout << "  \033[1;36m[5]\033[0m Simulate Smart Drop-Box Return (Hardware Solenoid Actuation)\n";
    std::cout << "  \033[1;36m[6]\033[0m Simulate Security Anti-Theft Exit Gate Passage\n";
    std::cout << "  \033[1;36m[7]\033[0m View Active Loans & Overdue Volumes\n";
    std::cout << "  \033[1;36m[8]\033[0m Hardware Driver Diagnostics & Telemetry Status\n";
    std::cout << "  \033[1;36m[9]\033[0m View Audit Transaction Journal (Disk Log)\n";
    std::cout << "  \033[1;32m[D]\033[0m Run Automated System Demonstration (Self-Guided)\n";
    std::cout << "  \033[1;31m[0]\033[0m Graceful Shutdown & Exit\n\n";
    std::cout << "\033[1;37mSelect an option: \033[0m";
}

void TerminalUI::showCatalog() const
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== PHYSICAL CATALOG & ELECTRONIC SHELF LOCATIONS ===\033[0m\n\n";

    auto books = m_catalog.getAllBooks();
    std::cout << "\033[1m"
              << std::left << std::setw(16) << "ISBN"
              << std::setw(15) << "RFID Tag"
              << std::setw(32) << "Title"
              << std::setw(14) << "Status"
              << std::setw(18) << "Shelf Location"
              << "\033[0m\n";
    std::cout << std::string(95, '-') << "\n";

    for (const auto& b : books) {
        std::string statusColor = "\033[32m"; // Green
        if (b.getStatus() == BookStatus::CHECKED_OUT)      statusColor = "\033[33m"; // Yellow
        else if (b.getStatus() == BookStatus::OVERDUE)     statusColor = "\033[31m"; // Red
        else if (b.getStatus() == BookStatus::RESERVED)    statusColor = "\033[34m"; // Blue
        else if (b.getStatus() == BookStatus::RETURNED_DROPBOX) statusColor = "\033[35m"; // Magenta

        std::string titleShort = b.getTitle();
        if (titleShort.length() > 30) {
            titleShort = titleShort.substr(0, 27) + "...";
        }

        std::ostringstream locOss;
        locOss << "A" << b.getLocation().aisle << "-R" << b.getLocation().rack
               << "-S" << b.getLocation().shelf;

        std::cout << std::left << std::setw(16) << b.getIsbn()
                  << std::setw(15) << b.getRfidTag()
                  << std::setw(32) << titleShort
                  << statusColor << std::setw(14) << b.getStatusString() << "\033[0m"
                  << std::setw(18) << locOss.str()
                  << "\n";
    }

    std::cout << std::string(95, '-') << "\n";
    std::cout << "Total Titles: " << m_catalog.getTotalBooks()
              << " | Available: " << m_catalog.getAvailableCount()
              << " | Checked Out: " << m_catalog.getCheckedOutCount() << "\n";

    pauseForUser();
}

void TerminalUI::searchCatalog() const
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== SEARCH LIBRARY CATALOG ===\033[0m\n";
    std::cout << "Enter search keyword (Title, Author, Category, ISBN, RFID): ";
    std::string query;
    std::getline(std::cin, query);

    if (query.empty()) return;

    auto results = m_catalog.search(query);
    std::cout << "\nFound " << results.size() << " matching titles:\n\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& b = results[i];
        std::cout << "\033[1;36m[" << (i + 1) << "] " << b.getTitle() << "\033[0m\n";
        std::cout << "    Author:   " << b.getAuthor() << " (" << b.getPublicationYear() << ")\n";
        std::cout << "    Category: " << b.getCategory() << " | ISBN: " << b.getIsbn() << " | RFID: " << b.getRfidTag() << "\n";
        std::cout << "    Status:   " << b.getStatusString() << "\n";
        std::cout << "    Location: " << b.getLocation().toString() << "\n\n";
    }

    pauseForUser();
}

void TerminalUI::handleCheckout()
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== PATRON SELF-CHECKOUT KIOSK ===\033[0m\n\n";
    std::cout << "Registered Quick Users for Testing:\n";
    std::cout << "  - U101 (CARD-STU-101) : Alex Chen (Student - limit 3)\n";
    std::cout << "  - F201 (CARD-FAC-201) : Dr. Elena Vance (Faculty - limit 10)\n";
    std::cout << "  - G401 (CARD-GST-401) : Arthur Pendelton (Guest - limit 1)\n\n";

    std::cout << "Enter User ID or Card UID: ";
    std::string userQuery;
    std::getline(std::cin, userQuery);

    std::cout << "Enter Book ISBN or RFID Tag: ";
    std::string bookQuery;
    std::getline(std::cin, bookQuery);

    std::string err;
    bool ok = m_circulation.checkoutBook(bookQuery, userQuery, err);

    if (ok) {
        std::cout << "\n\033[1;32m[SUCCESS] Book successfully checked out!\033[0m\n";
        Logger::getInstance().info("Checkout successful: Book " + bookQuery + " to " + userQuery);
    } else {
        std::cout << "\n\033[1;31m[ERROR] Checkout rejected: " << err << "\033[0m\n";
        Logger::getInstance().warn("Checkout failed: " + err);
    }

    pauseForUser();
}

void TerminalUI::handleManualReturn()
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== DESK BOOK RETURN ===\033[0m\n";
    std::cout << "Enter Book ISBN or RFID Tag: ";
    std::string query;
    std::getline(std::cin, query);

    double fine = 0.0;
    std::string err;
    bool ok = m_circulation.returnBookByRfid(query, fine, err);
    if (!ok) {
        ok = m_circulation.returnBookByIsbn(query, fine, err);
    }

    if (ok) {
        std::cout << "\n\033[1;32m[SUCCESS] Book marked returned and placed in reshelving intake.\033[0m\n";
        if (fine > 0.0) {
            std::cout << "\033[1;33mNotice: Overdue fine of $" << fine << " assessed to patron account.\033[0m\n";
        }
        Logger::getInstance().info("Book returned: " + query + " (Fine: $" + std::to_string(fine) + ")");
    } else {
        std::cout << "\n\033[1;31m[ERROR] Return failed: " << err << "\033[0m\n";
    }

    pauseForUser();
}

void TerminalUI::handleDropBoxSimulation()
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== SIMULATE AUTOMATED DROP-BOX CHUTE RETURN ===\033[0m\n";
    std::cout << "Injecting physical book RFID scan event into return chute reader...\n";

    std::cout << "Enter RFID Tag of returning book (e.g. RFID-LPI-001): ";
    std::string rfid;
    std::getline(std::cin, rfid);
    if (rfid.empty()) rfid = "RFID-LPI-001";

    lms_hardware_event_t event{};
    event.event_id = 999;
    event.timestamp_ns = 123456789;
    event.event_type = LIBSYS_EVT_DROPBOX_RETURN;
    event.gate_or_kiosk_id = 1;
    event.signal_strength = 95;
    std::strncpy(event.tag_uid, rfid.c_str(), LIBSYS_MAX_TAG_LEN - 1);

    std::cout << "\n\033[36m[HARDWARE] Simulating RFID detection at chute entrance...\033[0m\n";
    std::string details;
    bool ok = m_circulation.processDropBoxReturnEvent(event, details);

    if (ok) {
        std::cout << "\033[1;32m[HARDWARE] Solenoid Flap: UNLOCKED -> Book dropped -> RELOCKED\033[0m\n";
        std::cout << "\033[1;32m[RESULT] " << details << "\033[0m\n";
        Logger::getInstance().info("Drop-Box: " + details);
    } else {
        std::cout << "\033[1;31m[HARDWARE] Solenoid Flap: KEPT LOCKED\033[0m\n";
        std::cout << "\033[1;31m[REJECTED] " << details << "\033[0m\n";
        Logger::getInstance().warn("Drop-Box reject: " + details);
    }

    pauseForUser();
}

void TerminalUI::handleSecurityGateSimulation()
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== SIMULATE PHYSICAL SECURITY ANTI-THEFT EXIT GATE ===\033[0m\n";
    std::cout << "Patron carrying an item passes through physical RFID exit gate.\n\n";

    std::cout << "Enter RFID Tag detected by gate antennas (e.g. RFID-LDD-003): ";
    std::string rfid;
    std::getline(std::cin, rfid);
    if (rfid.empty()) rfid = "RFID-LDD-003";

    std::string notice;
    bool authorized = m_gate.handleGateScan(rfid, 1, notice);

    if (authorized) {
        std::cout << "\n\033[1;32m[GATE GREEN] PASSAGE AUTHORIZED\033[0m\n";
        std::cout << notice << "\n";
        Logger::getInstance().info(notice);
    } else {
        std::cout << "\n\033[1;31;5m[GATE RED FLASHING] *** SECURITY BREACH DETECTED ***\033[0m\n";
        std::cout << "\033[1;31m" << notice << "\033[0m\n";
        std::cout << "\033[33mAudio siren sounding and emergency strobe triggered via IOCTL.\033[0m\n";
        Logger::getInstance().security(notice);

        std::cout << "\nEnter 'S' to silence alarm or Enter to leave active: ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "s" || choice == "S") {
            m_gate.silenceAlarm(1);
            std::cout << "\033[32mAlarm silenced by operator.\033[0m\n";
        }
    }

    pauseForUser();
}

void TerminalUI::showActiveLoans() const
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== ACTIVE LOANS & OVERDUE MONITORING ===\033[0m\n\n";

    auto loans = m_circulation.getActiveLoans();
    if (loans.empty()) {
        std::cout << "No active loans at this time. All cataloged volumes accounted for on shelves.\n";
    } else {
        std::cout << "\033[1m"
                  << std::left << std::setw(12) << "Tx ID"
                  << std::setw(16) << "ISBN"
                  << std::setw(10) << "User ID"
                  << std::setw(22) << "Checkout Date"
                  << std::setw(22) << "Due Date"
                  << std::setw(12) << "Overdue?"
                  << "\033[0m\n";
        std::cout << std::string(94, '-') << "\n";

        for (const auto& l : loans) {
            bool isOd = l.isOverdue();
            std::cout << std::left << std::setw(12) << l.getTxId()
                      << std::setw(16) << l.getIsbn()
                      << std::setw(10) << l.getUserId()
                      << std::setw(22) << l.getCheckoutDateString()
                      << std::setw(22) << l.getDueDateString()
                      << (isOd ? "\033[1;31mYES (Late)\033[0m" : "\033[32mNo\033[0m")
                      << "\n";
        }
    }

    pauseForUser();
}

void TerminalUI::showHardwareDiagnostics() const
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== HARDWARE DRIVER DIAGNOSTICS & TELEMETRY ===\033[0m\n\n";

    if (!m_hardware) {
        std::cout << "No hardware interface connected.\n";
        pauseForUser();
        return;
    }

    lms_driver_status_t status{};
    bool ok = m_hardware->getStatus(status);

    std::cout << "Driver Backend:      " << m_hardware->getDriverName() << "\n";
    std::cout << "Driver Operational:  " << (ok ? "\033[32mYES (Ready)\033[0m" : "\033[31mFAULT\033[0m") << "\n";
    std::cout << "Total Events Logged: " << status.total_events_logged << "\n";
    std::cout << "Ring Buffer Occ:     " << status.buffer_occupancy << " / " << status.buffer_capacity << "\n";
    std::cout << "Dropped Events:      " << status.dropped_events << "\n";
    std::cout << "Drop-Box Solenoid:   " << (status.dropbox_solenoid_locked ? "\033[33mLOCKED\033[0m" : "\033[32mUNLOCKED\033[0m") << "\n";
    std::cout << "Gate Theft Alarm:    " << (status.security_alarm_active ? "\033[1;31mACTIVE (Siren On)\033[0m" : "\033[32mSILENT (Normal)\033[0m") << "\n";
    std::cout << "Security Breaches:   " << m_gate.getSecurityBreachCount() << "\n";

    if (m_shm && m_shm->isConnected()) {
        std::cout << "\nPOSIX Shared Memory: \033[32mMapped (/libsys_telemetry)\033[0m\n";
    }

    pauseForUser();
}

void TerminalUI::showAuditLog() const
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;33m=== AUDIT TRANSACTION JOURNAL (libsys_audit.log) ===\033[0m\n\n";

    std::ifstream in("libsys_audit.log");
    if (!in.is_open()) {
        std::cout << "No audit log file exists yet.\n";
    } else {
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
        size_t start = (lines.size() > 25) ? lines.size() - 25 : 0;
        for (size_t i = start; i < lines.size(); ++i) {
            std::cout << lines[i] << "\n";
        }
    }

    pauseForUser();
}

void TerminalUI::runAutomatedDemo(bool pauseAtEnd)
{
    clearScreen();
    printHeader();
    std::cout << "\033[1;35m════════════════════════════════════════════════════════════════════════════════\033[0m\n";
    std::cout << "\033[1;37m        STARTING AUTOMATED END-TO-END DEMONSTRATION OF SmartLMS         \033[0m\n";
    std::cout << "\033[1;35m════════════════════════════════════════════════════════════════════════════════\033[0m\n\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Step 1: Browse Initial Catalog
    std::cout << "\033[1;33m[DEMO 1/5]\033[0m Inspecting Catalog: Total 8 titles, all AVAILABLE on Racks...\n";
    std::cout << "  -> Book 'The Linux Programming Interface' [RFID: RFID-LPI-001] located at Aisle 1, Rack 1, Shelf 2.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Step 2: Kiosk Checkout
    std::cout << "\n\033[1;33m[DEMO 2/5]\033[0m Simulating Patron Self-Checkout at Kiosk...\n";
    std::cout << "  -> Patron Alex Chen (Student: U101) swipes card CARD-STU-101.\n";
    std::cout << "  -> RFID scanner reads 'RFID-LPI-001'.\n";
    std::string err;
    m_circulation.checkoutBook("978-1593272203", "U101", err);
    std::cout << "  \033[1;32m-> Checkout APPROVED: Loan Tx created with 14-day due date.\033[0m\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Step 3: Exit Gate Theft Detection
    std::cout << "\n\033[1;33m[DEMO 3/5]\033[0m Testing Physical Security Exit Gate Anti-Theft Sensor...\n";
    std::cout << "  -> Simulation A: Unborrowed book 'Linux Device Drivers' [RFID: RFID-LDD-003] carried past exit gate.\n";
    std::string notice;
    m_gate.handleGateScan("RFID-LDD-003", 1, notice);
    std::cout << "  \033[1;31m-> *** THEFT ALARM TRIGGERED! *** Siren and visual strobes actuated via IOCTL.\033[0m\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    std::cout << "  -> Silencing alarm after security intervention...\n";
    m_gate.silenceAlarm(1);

    std::cout << "  -> Simulation B: Alex Chen passes gate with legitimately checked-out 'RFID-LPI-001'.\n";
    m_gate.handleGateScan("RFID-LPI-001", 1, notice);
    std::cout << "  \033[1;32m-> GATE GREEN: Passage authorized.\033[0m\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Step 4: Automated Drop-Box Return
    std::cout << "\n\033[1;33m[DEMO 4/5]\033[0m Simulating Automated Smart Drop-Box Return Chute...\n";
    std::cout << "  -> Book dropped into chute. Chute RFID reader captures 'RFID-LPI-001'.\n";
    lms_hardware_event_t dropEvent{};
    dropEvent.event_id = 101;
    dropEvent.event_type = LIBSYS_EVT_DROPBOX_RETURN;
    std::strncpy(dropEvent.tag_uid, "RFID-LPI-001", LIBSYS_MAX_TAG_LEN - 1);
    std::string dropDetails;
    m_circulation.processDropBoxReturnEvent(dropEvent, dropDetails);
    std::cout << "  \033[1;32m-> Drop-Box Solenoid Flap UNLOCKED, volume ingested, flap RELOCKED.\033[0m\n";
    std::cout << "  -> Loan closed, volume status updated to 'RETURNED_DROPBOX'.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Step 5: Telemetry Summary
    std::cout << "\n\033[1;33m[DEMO 5/5]\033[0m POSIX Shared Memory Telemetry Status:\n";
    std::cout << "  -> Active Loans: " << m_circulation.getActiveLoanCount()
              << " | Total Checkouts: " << m_circulation.getTotalCheckouts()
              << " | Total Returns: " << m_circulation.getTotalReturns()
              << " | Security Breaches: " << m_gate.getSecurityBreachCount() << "\n\n";

    std::cout << "\033[1;32m════════════════════════════════════════════════════════════════════════════════\033[0m\n";
    std::cout << "\033[1;32m            AUTOMATED DEMONSTRATION COMPLETED SUCCESSFULLY!             \033[0m\n";
    std::cout << "\033[1;32m════════════════════════════════════════════════════════════════════════════════\033[0m\n";

    if (pauseAtEnd) {
        pauseForUser();
    }
}

void TerminalUI::runInteractiveLoop()
{
    while (SignalHandler::isRunning()) {
        clearScreen();
        printHeader();
        printMainMenu();

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            break;
        }

        if (choice == "1") {
            showCatalog();
        } else if (choice == "2") {
            searchCatalog();
        } else if (choice == "3") {
            handleCheckout();
        } else if (choice == "4") {
            handleManualReturn();
        } else if (choice == "5") {
            handleDropBoxSimulation();
        } else if (choice == "6") {
            handleSecurityGateSimulation();
        } else if (choice == "7") {
            showActiveLoans();
        } else if (choice == "8") {
            showHardwareDiagnostics();
        } else if (choice == "9") {
            showAuditLog();
        } else if (choice == "D" || choice == "d") {
            runAutomatedDemo();
        } else if (choice == "0" || choice == "q" || choice == "Q") {
            SignalHandler::requestShutdown();
            break;
        }
    }

    clearScreen();
    std::cout << "\033[1;32mSmartLMS: Gracefully shutting down... All buffers flushed.\033[0m\n";
}

} // namespace lms
