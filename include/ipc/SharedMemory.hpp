#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cstdint>
#include <string>

namespace lms {

#define LIBSYS_SHM_NAME "/libsys_telemetry"
#define LIBSYS_SHM_MAGIC 0x4C494253 // "LIBS"

#pragma pack(push, 1)
struct SharedTelemetry {
    uint32_t magic;                      // 0x4C494253
    uint32_t version;                    // Version 1
    uint32_t active_loans_count;         // Current active loans
    uint32_t total_checkouts_cumulative; // Cumulative checkouts since boot
    uint32_t total_returns_cumulative;   // Cumulative returns processed
    uint32_t security_breaches_total;    // Cumulative theft gate alerts
    uint32_t registered_users_count;     // Total patron count
    uint32_t catalog_total_books;        // Total cataloged items
    uint8_t  driver_connected;           // 1 if kernel driver active, 0 if mock
    uint8_t  dropbox_unlocked;           // 1 if solenoid flap currently released
    uint8_t  alarm_sounding;             // 1 if siren currently sounding
    uint8_t  system_healthy;             // 1 if all daemons normal
    uint64_t last_update_epoch_sec;      // POSIX epoch timestamp of last update
};
#pragma pack(pop)

class SharedMemoryManager {
public:
    SharedMemoryManager();
    ~SharedMemoryManager();

    bool initMaster(const std::string& shmName = LIBSYS_SHM_NAME);
    bool initClient(const std::string& shmName = LIBSYS_SHM_NAME);
    void closeShm();

    bool update(const SharedTelemetry& telemetry);
    bool read(SharedTelemetry& outTelemetry) const;

    bool isMaster() const { return m_isMaster; }
    bool isConnected() const { return m_shmPtr != nullptr; }

private:
    std::string m_shmName;
    int m_fd;
    SharedTelemetry* m_shmPtr;
    bool m_isMaster;
};

} // namespace lms

#endif // SHARED_MEMORY_HPP
