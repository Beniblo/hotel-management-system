#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <bsoncxx/document/view.hpp>

#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/models/Enums.hpp"
#include "hotel/models/Habitacion.hpp"
#include "hotel/repositories/RepositoryCommon.hpp"

namespace hotel::repositories {

struct HabitacionFilter {
    std::optional<std::string> tipo;
    std::optional<hotel::models::EstadoHabitacion> estado;
    std::optional<std::int32_t> capacidadMin;
    std::optional<double> tarifaMax;
    std::int32_t page{1};
    std::int32_t limit{20};
};

class HabitacionRepository {
public:
    explicit HabitacionRepository(hotel::database::MongoConnectionManager& connectionManager);

    [[nodiscard]] hotel::models::Habitacion create(const hotel::models::Habitacion& habitacion);
    [[nodiscard]] PagedResult<hotel::models::Habitacion> findAll(const HabitacionFilter& filter);
    [[nodiscard]] std::vector<hotel::models::Habitacion> findAllUnpaged(
        const std::optional<hotel::models::EstadoHabitacion>& estado = std::nullopt,
        const std::optional<std::string>& tipo = std::nullopt,
        const std::optional<std::int32_t>& capacidadMin = std::nullopt);
    [[nodiscard]] std::optional<hotel::models::Habitacion> findById(const std::string& id);
    [[nodiscard]] std::optional<hotel::models::Habitacion> findByNumero(const std::string& numero);
    [[nodiscard]] std::optional<hotel::models::Habitacion> update(
        const hotel::models::Habitacion& habitacion);
    [[nodiscard]] bool updateEstado(const std::string& id,
                                    hotel::models::EstadoHabitacion estado,
                                    std::chrono::system_clock::time_point actualizadoEn);
    [[nodiscard]] bool deleteById(const std::string& id);

private:
    static hotel::models::Habitacion fromDocument(const bsoncxx::document::view& doc);

    hotel::database::MongoConnectionManager& connectionManager_;
    static constexpr const char* kCollectionName = "habitaciones";
};

}  // namespace hotel::repositories