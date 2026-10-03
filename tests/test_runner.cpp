#include "core/Book.hpp"
#include "core/User.hpp"
#include "core/LoanTransaction.hpp"
#include "core/CatalogManager.hpp"
#include "core/CirculationManager.hpp"
#include "core/SecurityGateManager.hpp"
#include "hal/SimulatedHardware.hpp"
#include "ipc/SharedMemory.hpp"
#include "utils/DataStore.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <thread>
#include <chrono>

using namespace lms;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        std::cerr << "\033[31m[FAILED]\033[0m " << __FILE__ << ":" << __LINE__ \
                  << " Assertion failed: " #cond << std::endl; \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))

#define RUN_TEST(fn) do { \
    std::cout << "\033[36m[ RUN      ]\033[0m " #fn << std::endl; \
    int failedBefore = g_testsFailed; \
    fn(); \
    if (g_testsFailed == failedBefore) { \
        std::cout << "\033[32m[       OK ]\033[0m " #fn << std::endl; \
        g_testsPassed++; \
    } \
} while(0)

// TEST 1: Book Model & State Transitions
void test_book_model()
{
    BookLocation loc{1, 2, 3, "QA76.73"};
    Book book("978-0001", "RFID-001", "Sample Title", "Sample Author",
              "Tech", 2024, loc, BookStatus::AVAILABLE);

    ASSERT_EQ(book.getIsbn(), "978-0001");
    ASSERT_EQ(book.getRfidTag(), "RFID-001");
    ASSERT_TRUE(book.isAvailable());
    ASSERT_EQ(book.getEslColor(), 0x00FF00); // Green for available

    book.setStatus(BookStatus::CHECKED_OUT);
    ASSERT_FALSE(book.isAvailable());
    ASSERT_TRUE(book.isCheckedOut());
    ASSERT_EQ(book.getStatusString(), "CHECKED_OUT");

    book.setStatus(BookStatus::RESERVED);
    ASSERT_EQ(book.getEslColor(), 0x0088FF); // Blue for reserved
}

// TEST 2: User Roles & Quota Rules
void test_user_roles()
{
    User student("U01", "CARD-01", "Alice", "alice@edu", UserRole::STUDENT);
    ASSERT_EQ(student.getMaxBorrowLimit(), 3);
    ASSERT_EQ(student.getLoanDurationDays(), 14);
    ASSERT_FALSE(student.hasOutstandingFines());

    student.addFine(5.0);
    ASSERT_EQ(student.getFineBalance(), 5.0);
    ASSERT_FALSE(student.hasOutstandingFines(10.0));

    student.addFine(10.0);
    ASSERT_TRUE(student.hasOutstandingFines(10.0));

    ASSERT_TRUE(student.payFine(5.0));
    ASSERT_EQ(student.getFineBalance(), 10.0);

    User faculty("F01", "CARD-F01", "Prof. Bob", "bob@edu", UserRole::FACULTY);
    ASSERT_EQ(faculty.getMaxBorrowLimit(), 10);
    ASSERT_EQ(faculty.getLoanDurationDays(), 30);
}

// TEST 3: Loan Transaction & Overdue Math
void test_loan_transaction()
{
    std::time_t now = std::time(nullptr);
    std::time_t duePast = now - (3 * 24 * 3600); // 3 days ago

    LoanTransaction loan("TX-1", "978-0001", "RFID-001", "U01", duePast - 86400, duePast);
    ASSERT_TRUE(loan.isOverdue(now));

    double fine = loan.calculateFine(0.50, now);
    ASSERT_TRUE(fine >= 1.49 && fine <= 1.51); // 3 days * $0.50 = $1.50

    loan.markReturned(now, fine);
    ASSERT_TRUE(loan.isReturned());
    ASSERT_EQ(loan.getFineIncurred(), fine);
}

// TEST 4: CatalogManager Indexing & Lookups
void test_catalog_manager()
{
    CatalogManager catalog;
    Book b1("ISBN-1", "RFID-1", "Linux Kernel Development", "Robert Love", "Kernel", 2010, BookLocation{1,1,1,"QA1"});
    Book b2("ISBN-2", "RFID-2", "C++ Primer", "Stanley Lippman", "C++", 2012, BookLocation{1,1,2,"QA2"});

    ASSERT_TRUE(catalog.addBook(b1));
    ASSERT_TRUE(catalog.addBook(b2));
    ASSERT_FALSE(catalog.addBook(b1)); // Duplicate ISBN rejected

    ASSERT_NE(catalog.findByIsbn("ISBN-1"), nullptr);
    ASSERT_NE(catalog.findByRfid("RFID-2"), nullptr);
    ASSERT_EQ(catalog.findByRfid("RFID-NONEXISTENT"), nullptr);

    auto searchResults = catalog.search("Primer");
    ASSERT_EQ(searchResults.size(), 1);
    ASSERT_EQ(searchResults[0].getIsbn(), "ISBN-2");

    ASSERT_EQ(catalog.getTotalBooks(), 2);
    ASSERT_EQ(catalog.getAvailableCount(), 2);

    ASSERT_TRUE(catalog.removeBook("ISBN-1"));
    ASSERT_EQ(catalog.findByRfid("RFID-1"), nullptr);
    ASSERT_EQ(catalog.getTotalBooks(), 1);
}

// TEST 5: CirculationManager Checkout & Return Workflows
void test_circulation_flow()
{
    CatalogManager catalog;
    SimulatedHardware hardware;
    hardware.connect();

    CirculationManager circ(catalog, &hardware);
    DataStore::seedSampleData(catalog, circ);

    std::string err;
    // Checkout: The Linux Programming Interface to Alex Chen (U101)
    bool ok = circ.checkoutBook("978-1593272203", "U101", err);
    ASSERT_TRUE(ok);
    ASSERT_EQ(circ.getActiveLoanCount(), 1);

    Book* book = catalog.findByIsbn("978-1593272203");
    ASSERT_NE(book, nullptr);
    ASSERT_TRUE(book->isCheckedOut());

    // Try checking out the same book again -> Must fail
    ok = circ.checkoutBook("978-1593272203", "U102", err);
    ASSERT_FALSE(ok);

    // Return via RFID
    double fine = 0.0;
    ok = circ.returnBookByRfid(book->getRfidTag(), fine, err);
    ASSERT_TRUE(ok);
    ASSERT_EQ(circ.getActiveLoanCount(), 0);
    ASSERT_EQ(book->getStatus(), BookStatus::RETURNED_DROPBOX);
}

// TEST 6: Drop-Box Automated Hardware Return
void test_dropbox_return()
{
    CatalogManager catalog;
    SimulatedHardware hardware;
    hardware.connect();
    CirculationManager circ(catalog, &hardware);
    DataStore::seedSampleData(catalog, circ);

    std::string err;
    ASSERT_TRUE(circ.checkoutBook("978-0201704310", "U101", err));

    lms_hardware_event_t dropEvent{};
    dropEvent.event_id = 1;
    dropEvent.event_type = LIBSYS_EVT_DROPBOX_RETURN;
    dropEvent.gate_or_kiosk_id = 1;
    std::strncpy(dropEvent.tag_uid, "RFID-MCD-002", LIBSYS_MAX_TAG_LEN - 1);

    std::string details;
    bool ok = circ.processDropBoxReturnEvent(dropEvent, details);
    ASSERT_TRUE(ok);
    ASSERT_EQ(circ.getActiveLoanCount(), 0);
}

// TEST 7: Security Gate Anti-Theft Verification
void test_security_gate()
{
    CatalogManager catalog;
    SimulatedHardware hardware;
    hardware.connect();
    CirculationManager circ(catalog, &hardware);
    DataStore::seedSampleData(catalog, circ);

    SecurityGateManager gate(catalog, circ, &hardware);

    std::string notice;
    // 1. Unborrowed book passes gate -> Must sound alarm
    bool auth = gate.handleGateScan("RFID-LDD-003", 1, notice);
    ASSERT_FALSE(auth);
    ASSERT_TRUE(gate.isAlarmActive());
    ASSERT_EQ(gate.getSecurityBreachCount(), 1);

    gate.silenceAlarm(1);
    ASSERT_FALSE(gate.isAlarmActive());

    // 2. Checkout book properly
    std::string err;
    ASSERT_TRUE(circ.checkoutBook("978-0596005900", "U101", err));

    // 3. Now same book passes gate -> Authorized passage, no alarm
    auth = gate.handleGateScan("RFID-LDD-003", 1, notice);
    ASSERT_TRUE(auth);
    ASSERT_FALSE(gate.isAlarmActive());
}

// TEST 8: SimulatedHardware & IOCTL Emulation
void test_simulated_hardware()
{
    SimulatedHardware hw;
    ASSERT_TRUE(hw.connect());
    ASSERT_TRUE(hw.isConnected());

    lms_hardware_event_t ev{};
    ev.event_id = 42;
    std::strncpy(ev.tag_uid, "TEST-TAG-99", LIBSYS_MAX_TAG_LEN - 1);
    ASSERT_TRUE(hw.simulateScan(ev));

    lms_hardware_event_t readEv{};
    ASSERT_TRUE(hw.readEvent(readEv, 100));
    ASSERT_EQ(readEv.event_id, 42);
    ASSERT_EQ(std::string(readEv.tag_uid), "TEST-TAG-99");

    // Actuators
    ASSERT_TRUE(hw.setDropBoxLock(false));
    bool locked = true;
    ASSERT_TRUE(hw.getDropBoxLock(locked));
    ASSERT_FALSE(locked);

    ASSERT_TRUE(hw.setSecurityGateAlarm(true));
    bool alarm = false;
    ASSERT_TRUE(hw.getSecurityGateAlarm(alarm));
    ASSERT_TRUE(alarm);
}

// TEST 9: POSIX Shared Memory Telemetry
void test_shared_memory()
{
    SharedMemoryManager master;
    std::string shmTestName = "/libsys_test_shm";
    ASSERT_TRUE(master.initMaster(shmTestName));

    SharedTelemetry tx{};
    tx.magic = LIBSYS_SHM_MAGIC;
    tx.version = 1;
    tx.active_loans_count = 5;
    tx.total_checkouts_cumulative = 12;
    tx.total_returns_cumulative = 7;
    tx.security_breaches_total = 1;
    tx.system_healthy = 1;

    ASSERT_TRUE(master.update(tx));

    SharedMemoryManager client;
    ASSERT_TRUE(client.initClient(shmTestName));

    SharedTelemetry rx{};
    ASSERT_TRUE(client.read(rx));
    ASSERT_EQ(rx.magic, LIBSYS_SHM_MAGIC);
    ASSERT_EQ(rx.active_loans_count, 5);
    ASSERT_EQ(rx.total_checkouts_cumulative, 12);
    ASSERT_EQ(rx.security_breaches_total, 1);

    client.closeShm();
    master.closeShm();
}

// TEST 10: Multi-threaded Concurrency Stress Test
void test_concurrency_stress()
{
    CatalogManager catalog;
    SimulatedHardware hardware;
    hardware.connect();
    CirculationManager circ(catalog, &hardware);
    DataStore::seedSampleData(catalog, circ);

    const int threadCount = 4;
    std::vector<std::thread> workers;

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&circ, t]() {
            for (int i = 0; i < 20; ++i) {
                std::string err;
                std::string isbn = (i % 2 == 0) ? "978-1985086593" : "978-0201633610";
                std::string user = (t % 2 == 0) ? "U101" : "U102";

                if (circ.checkoutBook(isbn, user, err)) {
                    double fine = 0.0;
                    circ.returnBookByIsbn(isbn, fine, err);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    // Must finish cleanly without deadlocks or crashes
    ASSERT_TRUE(true);
}

int main()
{
    std::cout << "\033[1;34m========================================================\033[0m\n";
    std::cout << "\033[1;34m   SmartLMS Comprehensive Automated Test Suite          \033[0m\n";
    std::cout << "\033[1;34m========================================================\033[0m\n\n";

    RUN_TEST(test_book_model);
    RUN_TEST(test_user_roles);
    RUN_TEST(test_loan_transaction);
    RUN_TEST(test_catalog_manager);
    RUN_TEST(test_circulation_flow);
    RUN_TEST(test_dropbox_return);
    RUN_TEST(test_security_gate);
    RUN_TEST(test_simulated_hardware);
    RUN_TEST(test_shared_memory);
    RUN_TEST(test_concurrency_stress);

    std::cout << "\n\033[1;34m========================================================\033[0m\n";
    std::cout << "\033[1;32m   TEST SUITE EXECUTION COMPLETE                        \033[0m\n";
    std::cout << "   Passed: " << g_testsPassed << "  |  Failed: " << g_testsFailed << "\n";
    std::cout << "\033[1;34m========================================================\033[0m\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
