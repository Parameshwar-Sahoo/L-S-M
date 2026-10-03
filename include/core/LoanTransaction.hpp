#ifndef LOAN_TRANSACTION_HPP
#define LOAN_TRANSACTION_HPP

#include <string>
#include <ctime>

namespace lms {

class LoanTransaction {
public:
    LoanTransaction() = default;
    LoanTransaction(std::string txId, std::string isbn, std::string rfidTag,
                    std::string userId, std::time_t checkoutTime, std::time_t dueTime);

    const std::string& getTxId() const { return m_txId; }
    const std::string& getIsbn() const { return m_isbn; }
    const std::string& getRfidTag() const { return m_rfidTag; }
    const std::string& getUserId() const { return m_userId; }
    std::time_t getCheckoutTime() const { return m_checkoutTime; }
    std::time_t getDueTime() const { return m_dueTime; }
    std::time_t getReturnTime() const { return m_returnTime; }
    bool isReturned() const { return m_isReturned; }
    double getFineIncurred() const { return m_fineIncurred; }

    bool isOverdue(std::time_t now = 0) const;
    double calculateFine(double dailyRate = 0.50, std::time_t now = 0) const;
    void markReturned(std::time_t returnTime, double assessedFine = 0.0);

    std::string getCheckoutDateString() const;
    std::string getDueDateString() const;
    std::string getReturnDateString() const;

private:
    std::string m_txId;
    std::string m_isbn;
    std::string m_rfidTag;
    std::string m_userId;
    std::time_t m_checkoutTime{0};
    std::time_t m_dueTime{0};
    std::time_t m_returnTime{0};
    bool m_isReturned{false};
    double m_fineIncurred{0.0};
};

} // namespace lms

#endif // LOAN_TRANSACTION_HPP
