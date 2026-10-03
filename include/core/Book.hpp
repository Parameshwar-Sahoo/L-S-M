#ifndef BOOK_HPP
#define BOOK_HPP

#include <string>
#include <cstdint>

namespace lms {

enum class BookStatus {
    AVAILABLE,
    RESERVED,
    CHECKED_OUT,
    OVERDUE,
    RETURNED_DROPBOX,
    UNDER_RESHELVING,
    LOST_ARCHIVED
};

struct BookLocation {
    uint16_t aisle{1};
    uint16_t rack{1};
    uint16_t shelf{1};
    std::string callNumber{"QA76.73.C153"};

    std::string toString() const;
};

class Book {
public:
    Book() = default;
    Book(std::string isbn, std::string rfidTag, std::string title,
         std::string author, std::string category, uint16_t year,
         BookLocation location, BookStatus status = BookStatus::AVAILABLE);

    const std::string& getIsbn() const { return m_isbn; }
    const std::string& getRfidTag() const { return m_rfidTag; }
    const std::string& getTitle() const { return m_title; }
    const std::string& getAuthor() const { return m_author; }
    const std::string& getCategory() const { return m_category; }
    uint16_t getPublicationYear() const { return m_publicationYear; }
    const BookLocation& getLocation() const { return m_location; }
    BookStatus getStatus() const { return m_status; }
    uint32_t getEslColor() const { return m_eslColor; }

    void setStatus(BookStatus status);
    void setLocation(const BookLocation& loc) { m_location = loc; }
    void setEslColor(uint32_t color) { m_eslColor = color; }
    void setRfidTag(const std::string& tag) { m_rfidTag = tag; }

    bool isAvailable() const { return m_status == BookStatus::AVAILABLE; }
    bool isCheckedOut() const { return m_status == BookStatus::CHECKED_OUT || m_status == BookStatus::OVERDUE; }

    std::string getStatusString() const;
    static std::string statusToString(BookStatus status);
    static BookStatus stringToStatus(const std::string& str);

private:
    std::string m_isbn;
    std::string m_rfidTag;
    std::string m_title;
    std::string m_author;
    std::string m_category;
    uint16_t m_publicationYear{2024};
    BookLocation m_location;
    BookStatus m_status{BookStatus::AVAILABLE};
    uint32_t m_eslColor{0x00FF00}; // Default Green for available
};

} // namespace lms

#endif // BOOK_HPP
