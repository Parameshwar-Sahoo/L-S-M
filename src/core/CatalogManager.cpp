#include "core/CatalogManager.hpp"
#include <algorithm>
#include <cctype>

namespace lms {

static std::string toLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

bool CatalogManager::addBook(const Book& book)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_books.find(book.getIsbn()) != m_books.end()) {
        return false;
    }

    m_books[book.getIsbn()] = book;
    if (!book.getRfidTag().empty()) {
        m_rfidIndex[book.getRfidTag()] = book.getIsbn();
    }
    return true;
}

bool CatalogManager::updateBook(const Book& book)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_books.find(book.getIsbn());
    if (it == m_books.end()) {
        return false;
    }

    // Update RFID map if changed
    if (it->second.getRfidTag() != book.getRfidTag()) {
        m_rfidIndex.erase(it->second.getRfidTag());
        if (!book.getRfidTag().empty()) {
            m_rfidIndex[book.getRfidTag()] = book.getIsbn();
        }
    }

    it->second = book;
    return true;
}

bool CatalogManager::removeBook(const std::string& isbn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_books.find(isbn);
    if (it == m_books.end()) {
        return false;
    }

    m_rfidIndex.erase(it->second.getRfidTag());
    m_books.erase(it);
    return true;
}

Book* CatalogManager::findByIsbn(const std::string& isbn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_books.find(isbn);
    if (it != m_books.end()) {
        return &(it->second);
    }
    return nullptr;
}

Book* CatalogManager::findByRfid(const std::string& rfidTag)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto rfidIt = m_rfidIndex.find(rfidTag);
    if (rfidIt == m_rfidIndex.end()) {
        return nullptr;
    }

    auto bookIt = m_books.find(rfidIt->second);
    if (bookIt != m_books.end()) {
        return &(bookIt->second);
    }
    return nullptr;
}

std::vector<Book> CatalogManager::search(const std::string& query) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Book> results;
    std::string lowerQuery = toLower(query);

    for (const auto& pair : m_books) {
        const Book& book = pair.second;
        if (toLower(book.getTitle()).find(lowerQuery) != std::string::npos ||
            toLower(book.getAuthor()).find(lowerQuery) != std::string::npos ||
            toLower(book.getIsbn()).find(lowerQuery) != std::string::npos ||
            toLower(book.getCategory()).find(lowerQuery) != std::string::npos ||
            toLower(book.getRfidTag()).find(lowerQuery) != std::string::npos) {
            results.push_back(book);
        }
    }
    return results;
}

std::vector<Book> CatalogManager::getAllBooks() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Book> results;
    results.reserve(m_books.size());
    for (const auto& pair : m_books) {
        results.push_back(pair.second);
    }
    return results;
}

size_t CatalogManager::getTotalBooks() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_books.size();
}

size_t CatalogManager::getAvailableCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_books) {
        if (pair.second.isAvailable()) {
            count++;
        }
    }
    return count;
}

size_t CatalogManager::getCheckedOutCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_books) {
        if (pair.second.isCheckedOut()) {
            count++;
        }
    }
    return count;
}

void CatalogManager::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_books.clear();
    m_rfidIndex.clear();
}

} // namespace lms
