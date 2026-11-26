#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <bsoncxx/document/view.hpp>

#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/models/Huesped.hpp"
#include "hotel/repositories/RepositoryCommon.hpp"

namespace hotel::repositories {

struct HuespedFilter {
    std::optional<std::string> nombre;
    std::optional<std::string> correo;
    std::optional<bool> activo;
    std::int32_t page{1};
    std::int32_t limit{20};
};

class HuespedRepository {
public:
    explicit HuespedRepository(hotel::database::MongoConnectionManager& connectionManager);

    [[nodiscard]] hotel::models::Huesped create(const hotel::models::Huesped& huesped);
    [[nodiscard]] PagedResult<hotel::models::Huesped> findAll(const HuespedFilter& filter);
    [[nodiscard]] std::optional<hotel::models::Huesped> findById(const std::string& id);
    [[nodiscard]] std::optional<hotel::models::Huesped> findByEmail(const std::string& correo);
    [[nodiscard]] std::optional<hotel::models::Huesped> update(const hotel::models::Huesped& huesped);
    [[nodiscard]] bool setActive(const std::string& id,
                                 bool activo,
                                 std::chrono::system_clock::time_point actualizadoEn);
    [[nodiscard]] bool deleteById(const std::string& id);

private:
    static hotel::models::Huesped fromDocument(const bsoncxx::document::view& doc);

    hotel::database::MongoConnectionManager& connectionManager_;
    static constexpr const char* kCollectionName = "huespedes";
};

}  // namespace hotel::repositories