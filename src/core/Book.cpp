#include "core/Book.hpp"
#include <sstream>

namespace lms {

std::string BookLocation::toString() const
{
    std::ostringstream oss;
    oss << "Aisle " << aisle << ", Rack " << rack << ", Shelf " << shelf
        << " [" << callNumber << "]";
    return oss.str();
}

Book::Book(std::string isbn, std::string rfidTag, std::string title,
           std::string author, std::string category, uint16_t year,
           BookLocation location, BookStatus status)
    : m_isbn(std::move(isbn))
    , m_rfidTag(std::move(rfidTag))
    , m_title(std::move(title))
    , m_author(std::move(author))
    , m_category(std::move(category))
    , m_publicationYear(year)
    , m_location(std::move(location))
    , m_status(status)
    , m_eslColor(0x00FF00)
{
    setStatus(status);
}

void Book::setStatus(BookStatus status)
{
    m_status = status;
    switch (m_status) {
    case BookStatus::AVAILABLE:
        m_eslColor = 0x00FF00; // Green
        break;
    case BookStatus::RESERVED:
        m_eslColor = 0x0088FF; // Blue
        break;
    case BookStatus::CHECKED_OUT:
        m_eslColor = 0x888888; // Grey / Off
        break;
    case BookStatus::OVERDUE:
        m_eslColor = 0xFF0000; // Red
        break;
    case BookStatus::RETURNED_DROPBOX:
        m_eslColor = 0xFFAA00; // Amber
        break;
    case BookStatus::UNDER_RESHELVING:
        m_eslColor = 0x9933FF; // Purple
        break;
    case BookStatus::LOST_ARCHIVED:
        m_eslColor = 0x000000; // Off
        break;
    }
}

std::string Book::getStatusString() const
{
    return statusToString(m_status);
}

std::string Book::statusToString(BookStatus status)
{
    switch (status) {
    case BookStatus::AVAILABLE:        return "AVAILABLE";
    case BookStatus::RESERVED:         return "RESERVED";
    case BookStatus::CHECKED_OUT:      return "CHECKED_OUT";
    case BookStatus::OVERDUE:          return "OVERDUE";
    case BookStatus::RETURNED_DROPBOX: return "RETURNED_DROPBOX";
    case BookStatus::UNDER_RESHELVING: return "UNDER_RESHELVING";
    case BookStatus::LOST_ARCHIVED:    return "LOST_ARCHIVED";
    default:                           return "UNKNOWN";
    }
}

BookStatus Book::stringToStatus(const std::string& str)
{
    if (str == "AVAILABLE")        return BookStatus::AVAILABLE;
    if (str == "RESERVED")         return BookStatus::RESERVED;
    if (str == "CHECKED_OUT")      return BookStatus::CHECKED_OUT;
    if (str == "OVERDUE")          return BookStatus::OVERDUE;
    if (str == "RETURNED_DROPBOX") return BookStatus::RETURNED_DROPBOX;
    if (str == "UNDER_RESHELVING") return BookStatus::UNDER_RESHELVING;
    if (str == "LOST_ARCHIVED")    return BookStatus::LOST_ARCHIVED;
    return BookStatus::AVAILABLE;
}

} // namespace lms
