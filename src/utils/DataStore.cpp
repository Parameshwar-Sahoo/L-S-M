#include "utils/DataStore.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace lms {

void DataStore::seedSampleData(CatalogManager& catalog, CirculationManager& circulation)
{
    // Seed Books
    catalog.addBook(Book(
        "978-1593272203", "RFID-LPI-001",
        "The Linux Programming Interface", "Michael Kerrisk",
        "Systems Programming", 2010,
        BookLocation{1, 1, 2, "QA76.76.O63 K47 2010"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-0201704310", "RFID-MCD-002",
        "Modern C++ Design", "Andrei Alexandrescu",
        "C++ Architecture", 2001,
        BookLocation{1, 2, 1, "QA76.73.C153 A49 2001"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-0596005900", "RFID-LDD-003",
        "Linux Device Drivers (3rd Ed)", "Jonathan Corbet, Alessandro Rubini, Greg Kroah-Hartman",
        "Kernel Engineering", 2005,
        BookLocation{1, 3, 3, "QA76.76.O63 C66 2005"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-1985086593", "RFID-OSTEP-004",
        "Operating Systems: Three Easy Pieces", "Remzi H. Arpaci-Dusseau, Andrea C. Arpaci-Dusseau",
        "Operating Systems", 2018,
        BookLocation{2, 1, 1, "QA76.76.O63 A77 2018"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-0201633610", "RFID-GOF-005",
        "Design Patterns: Elements of Reusable Object-Oriented Software", "Erich Gamma, Richard Helm, Ralph Johnson, John Vlissides",
        "Software Engineering", 1994,
        BookLocation{2, 2, 2, "QA76.64 .D47 1994"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-1491903995", "RFID-EMC-006",
        "Effective Modern C++", "Scott Meyers",
        "C++ Standards", 2014,
        BookLocation{1, 2, 2, "QA76.73.C153 M49 2014"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-0134092669", "RFID-CSAPP-007",
        "Computer Systems: A Programmer's Perspective", "Randal E. Bryant, David R. O'Hallaron",
        "Computer Architecture", 2015,
        BookLocation{2, 3, 1, "QA76.5 .B795 2015"}, BookStatus::AVAILABLE
    ));

    catalog.addBook(Book(
        "978-0321637734", "RFID-APUE-008",
        "Advanced Programming in the UNIX Environment", "W. Richard Stevens, Stephen A. Rago",
        "UNIX System Calls", 2013,
        BookLocation{1, 1, 1, "QA76.76.O63 S754 2013"}, BookStatus::AVAILABLE
    ));

    // Seed Users
    circulation.registerUser(User(
        "U101", "CARD-STU-101", "Alex Chen",
        "alex.chen@university.edu", UserRole::STUDENT, 0.0
    ));

    circulation.registerUser(User(
        "U102", "CARD-STU-102", "Priya Sharma",
        "priya.sharma@university.edu", UserRole::STUDENT, 0.0
    ));

    circulation.registerUser(User(
        "F201", "CARD-FAC-201", "Dr. Elena Vance",
        "elena.vance@university.edu", UserRole::FACULTY, 0.0
    ));

    circulation.registerUser(User(
        "L301", "CARD-LIB-301", "Marcus Brody",
        "marcus.brody@university.edu", UserRole::LIBRARIAN, 0.0
    ));

    circulation.registerUser(User(
        "G401", "CARD-GST-401", "Arthur Pendelton",
        "arthur.p@guest.org", UserRole::GUEST, 0.0
    ));
}

bool DataStore::saveCatalogToJson(const CatalogManager& catalog, const std::string& filePath)
{
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    auto books = catalog.getAllBooks();
    out << "[\n";
    for (size_t i = 0; i < books.size(); ++i) {
        const auto& b = books[i];
        out << "  {\n"
            << "    \"isbn\": \"" << b.getIsbn() << "\",\n"
            << "    \"rfid_tag\": \"" << b.getRfidTag() << "\",\n"
            << "    \"title\": \"" << b.getTitle() << "\",\n"
            << "    \"author\": \"" << b.getAuthor() << "\",\n"
            << "    \"category\": \"" << b.getCategory() << "\",\n"
            << "    \"year\": " << b.getPublicationYear() << ",\n"
            << "    \"status\": \"" << b.getStatusString() << "\",\n"
            << "    \"location\": {\n"
            << "      \"aisle\": " << b.getLocation().aisle << ",\n"
            << "      \"rack\": " << b.getLocation().rack << ",\n"
            << "      \"shelf\": " << b.getLocation().shelf << ",\n"
            << "      \"call_number\": \"" << b.getLocation().callNumber << "\"\n"
            << "    }\n"
            << "  }" << (i + 1 < books.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return true;
}

bool DataStore::saveUsersToJson(const CirculationManager& circulation, const std::string& filePath)
{
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    auto users = circulation.getAllUsers();
    out << "[\n";
    for (size_t i = 0; i < users.size(); ++i) {
        const auto& u = users[i];
        out << "  {\n"
            << "    \"user_id\": \"" << u.getUserId() << "\",\n"
            << "    \"card_uid\": \"" << u.getCardUid() << "\",\n"
            << "    \"name\": \"" << u.getName() << "\",\n"
            << "    \"email\": \"" << u.getEmail() << "\",\n"
            << "    \"role\": \"" << u.getRoleString() << "\",\n"
            << "    \"fine_balance\": " << u.getFineBalance() << "\n"
            << "  }" << (i + 1 < users.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return true;
}

} // namespace lms
