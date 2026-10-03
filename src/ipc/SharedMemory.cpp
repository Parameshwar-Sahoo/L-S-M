#include "ipc/SharedMemory.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <iostream>

namespace lms {

SharedMemoryManager::SharedMemoryManager()
    : m_shmName(LIBSYS_SHM_NAME)
    , m_fd(-1)
    , m_shmPtr(nullptr)
    , m_isMaster(false)
{
}

SharedMemoryManager::~SharedMemoryManager()
{
    closeShm();
}

bool SharedMemoryManager::initMaster(const std::string& shmName)
{
    closeShm();
    m_shmName = shmName;
    m_isMaster = true;

    // Unlink any stale segment first
    ::shm_unlink(m_shmName.c_str());

    m_fd = ::shm_open(m_shmName.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_fd < 0) {
        return false;
    }

    if (::ftruncate(m_fd, sizeof(SharedTelemetry)) != 0) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    void* addr = ::mmap(nullptr, sizeof(SharedTelemetry),
                        PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if (addr == MAP_FAILED) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_shmPtr = static_cast<SharedTelemetry*>(addr);
    std::memset(m_shmPtr, 0, sizeof(SharedTelemetry));
    m_shmPtr->magic = LIBSYS_SHM_MAGIC;
    m_shmPtr->version = 1;
    m_shmPtr->system_healthy = 1;
    m_shmPtr->last_update_epoch_sec = static_cast<uint64_t>(std::time(nullptr));

    return true;
}

bool SharedMemoryManager::initClient(const std::string& shmName)
{
    closeShm();
    m_shmName = shmName;
    m_isMaster = false;

    m_fd = ::shm_open(m_shmName.c_str(), O_RDONLY, 0666);
    if (m_fd < 0) {
        return false;
    }

    void* addr = ::mmap(nullptr, sizeof(SharedTelemetry),
                        PROT_READ, MAP_SHARED, m_fd, 0);
    if (addr == MAP_FAILED) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_shmPtr = static_cast<SharedTelemetry*>(addr);
    if (m_shmPtr->magic != LIBSYS_SHM_MAGIC) {
        closeShm();
        return false;
    }

    return true;
}

void SharedMemoryManager::closeShm()
{
    if (m_shmPtr && m_shmPtr != MAP_FAILED) {
        ::munmap(m_shmPtr, sizeof(SharedTelemetry));
        m_shmPtr = nullptr;
    }
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_isMaster && !m_shmName.empty()) {
        ::shm_unlink(m_shmName.c_str());
        m_isMaster = false;
    }
}

bool SharedMemoryManager::update(const SharedTelemetry& telemetry)
{
    if (!m_shmPtr || !m_isMaster) {
        return false;
    }

    uint32_t origMagic = m_shmPtr->magic;
    uint32_t origVer = m_shmPtr->version;

    *m_shmPtr = telemetry;
    m_shmPtr->magic = (origMagic == LIBSYS_SHM_MAGIC) ? origMagic : LIBSYS_SHM_MAGIC;
    m_shmPtr->version = (origVer != 0) ? origVer : 1;
    m_shmPtr->last_update_epoch_sec = static_cast<uint64_t>(std::time(nullptr));

    return true;
}

bool SharedMemoryManager::read(SharedTelemetry& outTelemetry) const
{
    if (!m_shmPtr) {
        return false;
    }
    if (m_shmPtr->magic != LIBSYS_SHM_MAGIC) {
        return false;
    }

    outTelemetry = *m_shmPtr;
    return true;
}

} // namespace lms
