#ifndef CIRCULATION_MANAGER_HPP
#define CIRCULATION_MANAGER_HPP

#include "CatalogManager.hpp"
#include "User.hpp"
#include "LoanTransaction.hpp"
#include "hal/ILibraryHardware.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>

namespace lms {

class CirculationManager {
public:
    explicit CirculationManager(CatalogManager& catalog, ILibraryHardware* hardware = nullptr);

    void setHardware(ILibraryHardware* hardware);

    bool registerUser(const User& user);
    User* findUserById(const std::string& userId);
    User* findUserByCardUid(const std::string& cardUid);
    std::vector<User> getAllUsers() const;
    size_t getTotalUsers() const;

    bool checkoutBook(const std::string& isbnOrRfid,
                      const std::string& userIdOrCardUid,
                      std::string& outError);

    bool returnBookByRfid(const std::string& rfidTag,
                          double& outAssessedFine,
                          std::string& outError);

    bool returnBookByIsbn(const std::string& isbn,
                          double& outAssessedFine,
                          std::string& outError);

    bool processDropBoxReturnEvent(const lms_hardware_event_t& event,
                                  std::string& outDetails);

    std::vector<LoanTransaction> getActiveLoans() const;
    std::vector<LoanTransaction> getOverdueLoans() const;
    std::vector<LoanTransaction> getUserActiveLoans(const std::string& userId) const;

    size_t getActiveLoanCount() const;
    size_t getTotalCheckouts() const;
    size_t getTotalReturns() const;

private:
    CatalogManager& m_catalog;
    ILibraryHardware* m_hardware;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, User> m_users;             // Key: User ID
    std::unordered_map<std::string, std::string> m_cardToUid;  // Key: Card UID -> User ID
    std::vector<LoanTransaction> m_loans;

    size_t m_totalCheckouts{0};
    size_t m_totalReturns{0};
    uint32_t m_nextTxCounter{1001};
};

} // namespace lms

#endif // CIRCULATION_MANAGER_HPP
