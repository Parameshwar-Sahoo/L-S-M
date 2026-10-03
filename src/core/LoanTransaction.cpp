#include "core/LoanTransaction.hpp"
#include <iomanip>
#include <sstream>

namespace lms {

LoanTransaction::LoanTransaction(std::string txId, std::string isbn, std::string rfidTag,
                                 std::string userId, std::time_t checkoutTime, std::time_t dueTime)
    : m_txId(std::move(txId))
    , m_isbn(std::move(isbn))
    , m_rfidTag(std::move(rfidTag))
    , m_userId(std::move(userId))
    , m_checkoutTime(checkoutTime)
    , m_dueTime(dueTime)
    , m_returnTime(0)
    , m_isReturned(false)
    , m_fineIncurred(0.0)
{
}

bool LoanTransaction::isOverdue(std::time_t now) const
{
    if (m_isReturned) {
        return (m_returnTime > m_dueTime);
    }
    if (now == 0) {
        now = std::time(nullptr);
    }
    return (now > m_dueTime);
}

double LoanTransaction::calculateFine(double dailyRate, std::time_t now) const
{
    if (!isOverdue(now)) {
        return 0.0;
    }

    std::time_t compareTime = m_isReturned ? m_returnTime : (now == 0 ? std::time(nullptr) : now);
    double diffSec = std::difftime(compareTime, m_dueTime);
    if (diffSec <= 0.0) {
        return 0.0;
    }

    // Daily calculation
    double days = diffSec / (24.0 * 3600.0);
    return days * dailyRate;
}

void LoanTransaction::markReturned(std::time_t returnTime, double assessedFine)
{
    m_returnTime = returnTime;
    m_isReturned = true;
    m_fineIncurred = assessedFine;
}

static std::string formatTime(std::time_t t)
{
    if (t == 0) return "N/A";
    std::tm tmStruct{};
    localtime_r(&t, &tmStruct);
    std::ostringstream oss;
    oss << std::put_time(&tmStruct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string LoanTransaction::getCheckoutDateString() const
{
    return formatTime(m_checkoutTime);
}

std::string LoanTransaction::getDueDateString() const
{
    return formatTime(m_dueTime);
}

std::string LoanTransaction::getReturnDateString() const
{
    return formatTime(m_returnTime);
}

} // namespace lms
