#include "core/User.hpp"

namespace lms {

User::User(std::string userId, std::string cardUid, std::string name,
           std::string email, UserRole role, double fineBalance)
    : m_userId(std::move(userId))
    , m_cardUid(std::move(cardUid))
    , m_name(std::move(name))
    , m_email(std::move(email))
    , m_role(role)
    , m_fineBalance(fineBalance)
{
}

int User::getMaxBorrowLimit() const
{
    switch (m_role) {
    case UserRole::STUDENT:   return 3;
    case UserRole::FACULTY:   return 10;
    case UserRole::LIBRARIAN: return 20;
    case UserRole::GUEST:     return 1;
    default:                  return 2;
    }
}

int User::getLoanDurationDays() const
{
    switch (m_role) {
    case UserRole::STUDENT:   return 14;
    case UserRole::FACULTY:   return 30;
    case UserRole::LIBRARIAN: return 60;
    case UserRole::GUEST:     return 7;
    default:                  return 14;
    }
}

void User::addFine(double amount)
{
    if (amount > 0.0) {
        m_fineBalance += amount;
    }
}

bool User::payFine(double amount)
{
    if (amount <= 0.0 || amount > m_fineBalance) {
        return false;
    }
    m_fineBalance -= amount;
    return true;
}

bool User::hasOutstandingFines(double threshold) const
{
    return m_fineBalance >= threshold;
}

std::string User::getRoleString() const
{
    return roleToString(m_role);
}

std::string User::roleToString(UserRole role)
{
    switch (role) {
    case UserRole::STUDENT:   return "STUDENT";
    case UserRole::FACULTY:   return "FACULTY";
    case UserRole::LIBRARIAN: return "LIBRARIAN";
    case UserRole::GUEST:     return "GUEST";
    default:                  return "UNKNOWN";
    }
}

UserRole User::stringToRole(const std::string& str)
{
    if (str == "STUDENT")   return UserRole::STUDENT;
    if (str == "FACULTY")   return UserRole::FACULTY;
    if (str == "LIBRARIAN") return UserRole::LIBRARIAN;
    if (str == "GUEST")     return UserRole::GUEST;
    return UserRole::STUDENT;
}

} // namespace lms
