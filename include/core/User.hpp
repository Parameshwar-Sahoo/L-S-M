#ifndef USER_HPP
#define USER_HPP

#include <string>

namespace lms {

enum class UserRole {
    STUDENT,
    FACULTY,
    LIBRARIAN,
    GUEST
};

class User {
public:
    User() = default;
    User(std::string userId, std::string cardUid, std::string name,
         std::string email, UserRole role = UserRole::STUDENT, double fineBalance = 0.0);

    const std::string& getUserId() const { return m_userId; }
    const std::string& getCardUid() const { return m_cardUid; }
    const std::string& getName() const { return m_name; }
    const std::string& getEmail() const { return m_email; }
    UserRole getRole() const { return m_role; }
    double getFineBalance() const { return m_fineBalance; }

    int getMaxBorrowLimit() const;
    int getLoanDurationDays() const;

    void addFine(double amount);
    bool payFine(double amount);
    bool hasOutstandingFines(double threshold = 10.0) const;

    std::string getRoleString() const;
    static std::string roleToString(UserRole role);
    static UserRole stringToRole(const std::string& str);

private:
    std::string m_userId;
    std::string m_cardUid;
    std::string m_name;
    std::string m_email;
    UserRole m_role{UserRole::STUDENT};
    double m_fineBalance{0.0};
};

} // namespace lms

#endif // USER_HPP
