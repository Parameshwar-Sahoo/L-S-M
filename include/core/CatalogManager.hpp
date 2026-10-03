#ifndef CATALOG_MANAGER_HPP
#define CATALOG_MANAGER_HPP

#include "Book.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

namespace lms {

class CatalogManager {
public:
    CatalogManager() = default;

    bool addBook(const Book& book);
    bool updateBook(const Book& book);
    bool removeBook(const std::string& isbn);

    Book* findByIsbn(const std::string& isbn);
    Book* findByRfid(const std::string& rfidTag);

    std::vector<Book> search(const std::string& query) const;
    std::vector<Book> getAllBooks() const;

    size_t getTotalBooks() const;
    size_t getAvailableCount() const;
    size_t getCheckedOutCount() const;

    void clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Book> m_books;          // Key: ISBN
    std::unordered_map<std::string, std::string> m_rfidIndex; // Key: RFID Tag -> ISBN
};

} // namespace lms

#endif // CATALOG_MANAGER_HPP
