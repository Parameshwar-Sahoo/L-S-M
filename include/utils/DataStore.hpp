#ifndef DATA_STORE_HPP
#define DATA_STORE_HPP

#include "core/CatalogManager.hpp"
#include "core/CirculationManager.hpp"
#include <string>

namespace lms {

class DataStore {
public:
    static void seedSampleData(CatalogManager& catalog, CirculationManager& circulation);
    static bool saveCatalogToJson(const CatalogManager& catalog, const std::string& filePath = "data/catalog.json");
    static bool loadCatalogFromJson(CatalogManager& catalog, const std::string& filePath = "data/catalog.json");
    static bool saveUsersToJson(const CirculationManager& circulation, const std::string& filePath = "data/users.json");
    static bool loadUsersFromJson(CirculationManager& circulation, const std::string& filePath = "data/users.json");
};

} // namespace lms

#endif // DATA_STORE_HPP
