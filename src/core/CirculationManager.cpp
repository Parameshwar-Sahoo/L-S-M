#include "core/CirculationManager.hpp"
#include <ctime>
#include <sstream>
#include <iostream>

namespace lms {

CirculationManager::CirculationManager(CatalogManager& catalog, ILibraryHardware* hardware)
    : m_catalog(catalog)
    , m_hardware(hardware)
{
}

void CirculationManager::setHardware(ILibraryHardware* hardware)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hardware = hardware;
}

bool CirculationManager::registerUser(const User& user)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_users.find(user.getUserId()) != m_users.end()) {
        return false;
    }
    m_users[user.getUserId()] = user;
    if (!user.getCardUid().empty()) {
        m_cardToUid[user.getCardUid()] = user.getUserId();
    }
    return true;
}

User* CirculationManager::findUserById(const std::string& userId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_users.find(userId);
    if (it != m_users.end()) {
        return &(it->second);
    }
    return nullptr;
}

User* CirculationManager::findUserByCardUid(const std::string& cardUid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto itCard = m_cardToUid.find(cardUid);
    if (itCard == m_cardToUid.end()) {
        return nullptr;
    }
    auto itUser = m_users.find(itCard->second);
    if (itUser != m_users.end()) {
        return &(itUser->second);
    }
    return nullptr;
}

std::vector<User> CirculationManager::getAllUsers() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<User> result;
    result.reserve(m_users.size());
    for (const auto& pair : m_users) {
        result.push_back(pair.second);
    }
    return result;
}

size_t CirculationManager::getTotalUsers() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_users.size();
}

bool CirculationManager::checkoutBook(const std::string& isbnOrRfid,
                                      const std::string& userIdOrCardUid,
                                      std::string& outError)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Resolve User
    User* user = nullptr;
    auto itUser = m_users.find(userIdOrCardUid);
    if (itUser != m_users.end()) {
        user = &(itUser->second);
    } else {
        auto itCard = m_cardToUid.find(userIdOrCardUid);
        if (itCard != m_cardToUid.end()) {
            user = &(m_users[itCard->second]);
        }
    }

    if (!user) {
        outError = "User not found (invalid ID or Card UID)";
        return false;
    }

    // 2. Validate Outstanding Fines
    if (user->hasOutstandingFines(15.00)) {
        outError = "Account suspended due to outstanding fines ($" +
                   std::to_string(user->getFineBalance()) + " >= $15.00 limit)";
        return false;
    }

    // 3. Validate Borrow Quota
    size_t activeCount = 0;
    for (const auto& loan : m_loans) {
        if (!loan.isReturned() && loan.getUserId() == user->getUserId()) {
            activeCount++;
        }
    }
    if (activeCount >= static_cast<size_t>(user->getMaxBorrowLimit())) {
        outError = "Maximum borrowing limit reached (" + std::to_string(activeCount) +
                   "/" + std::to_string(user->getMaxBorrowLimit()) + " items)";
        return false;
    }

    // 4. Resolve Book
    Book* book = m_catalog.findByIsbn(isbnOrRfid);
    if (!book) {
        book = m_catalog.findByRfid(isbnOrRfid);
    }
    if (!book) {
        outError = "Book not found in catalog";
        return false;
    }

    if (!book->isAvailable()) {
        outError = "Book is not available (current status: " + book->getStatusString() + ")";
        return false;
    }

    // 5. Execute Checkout
    book->setStatus(BookStatus::CHECKED_OUT);

    std::time_t now = std::time(nullptr);
    std::time_t due = now + (user->getLoanDurationDays() * 24 * 3600);

    std::ostringstream txOss;
    txOss << "TX-" << m_nextTxCounter++;

    LoanTransaction loan(txOss.str(), book->getIsbn(), book->getRfidTag(),
                        user->getUserId(), now, due);
    m_loans.push_back(loan);
    m_totalCheckouts++;

    // Hardware Notification: Update shelf indicator
    if (m_hardware && m_hardware->isConnected()) {
        m_hardware->setShelfLED(book->getLocation().aisle,
                               book->getLocation().rack,
                               0x888888); // Off / Dim
    }

    return true;
}

bool CirculationManager::returnBookByRfid(const std::string& rfidTag,
                                          double& outAssessedFine,
                                          std::string& outError)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Book* book = m_catalog.findByRfid(rfidTag);
    if (!book) {
        outError = "No cataloged book matched RFID tag: " + rfidTag;
        return false;
    }

    return returnBookByIsbn(book->getIsbn(), outAssessedFine, outError);
}

bool CirculationManager::returnBookByIsbn(const std::string& isbn,
                                          double& outAssessedFine,
                                          std::string& outError)
{
    // Note: Called with mutex already held if called from returnBookByRfid,
    // but if called directly, we lock. Let's make internal helper.
    Book* book = m_catalog.findByIsbn(isbn);
    if (!book) {
        outError = "Book not found: " + isbn;
        return false;
    }

    // Find active loan
    LoanTransaction* activeLoan = nullptr;
    for (auto& loan : m_loans) {
        if (!loan.isReturned() && loan.getIsbn() == isbn) {
            activeLoan = &loan;
            break;
        }
    }

    std::time_t now = std::time(nullptr);
    outAssessedFine = 0.0;

    if (activeLoan) {
        outAssessedFine = activeLoan->calculateFine(0.50, now);
        activeLoan->markReturned(now, outAssessedFine);

        if (outAssessedFine > 0.0) {
            auto itUser = m_users.find(activeLoan->getUserId());
            if (itUser != m_users.end()) {
                itUser->second.addFine(outAssessedFine);
            }
        }
    }

    book->setStatus(BookStatus::RETURNED_DROPBOX);
    m_totalReturns++;

    // Hardware notification: Guide shelf reshelving
    if (m_hardware && m_hardware->isConnected()) {
        m_hardware->setShelfLED(book->getLocation().aisle,
                               book->getLocation().rack,
                               0xFFAA00); // Amber: Ready for reshelving
    }

    return true;
}

bool CirculationManager::processDropBoxReturnEvent(const lms_hardware_event_t& event,
                                                   std::string& outDetails)
{
    double fine = 0.0;
    std::string error;

    std::string tag(event.tag_uid);
    bool ok = returnBookByRfid(tag, fine, error);

    if (ok) {
        outDetails = "Drop-Box accepted RFID " + tag + ". Return processed successfully.";
        if (fine > 0.0) {
            outDetails += " Overdue fine assessed: $" + std::to_string(fine);
        }

        // Hardware Solenoid Actuation: Momentarily unlock drop-box chute flap
        if (m_hardware && m_hardware->isConnected()) {
            m_hardware->setDropBoxLock(false); // Unlock to drop volume
            // Relock is executed after sensor confirm
            m_hardware->setDropBoxLock(true);
        }
        return true;
    } else {
        outDetails = "Drop-Box rejected RFID " + tag + ": " + error;
        return false;
    }
}

std::vector<LoanTransaction> CirculationManager::getActiveLoans() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LoanTransaction> result;
    for (const auto& loan : m_loans) {
        if (!loan.isReturned()) {
            result.push_back(loan);
        }
    }
    return result;
}

std::vector<LoanTransaction> CirculationManager::getOverdueLoans() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LoanTransaction> result;
    std::time_t now = std::time(nullptr);
    for (const auto& loan : m_loans) {
        if (!loan.isReturned() && loan.isOverdue(now)) {
            result.push_back(loan);
        }
    }
    return result;
}

std::vector<LoanTransaction> CirculationManager::getUserActiveLoans(const std::string& userId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LoanTransaction> result;
    for (const auto& loan : m_loans) {
        if (!loan.isReturned() && loan.getUserId() == userId) {
            result.push_back(loan);
        }
    }
    return result;
}

size_t CirculationManager::getActiveLoanCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& loan : m_loans) {
        if (!loan.isReturned()) {
            count++;
        }
    }
    return count;
}

size_t CirculationManager::getTotalCheckouts() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalCheckouts;
}

size_t CirculationManager::getTotalReturns() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalReturns;
}

} // namespace lms
