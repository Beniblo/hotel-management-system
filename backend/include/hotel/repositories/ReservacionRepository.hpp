#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <bsoncxx/document/view.hpp>

#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/models/Enums.hpp"
#include "hotel/models/Reservacion.hpp"
#include "hotel/repositories/RepositoryCommon.hpp"

namespace hotel::repositories {

struct ReservacionFilter {
    std::optional<std::string> huespedId;
    std::optional<std::string> habitacionId;
    std::optional<hotel::models::EstadoReservacion> estado;
    std::optional<std::string> fechaDesde;  // YYYY-MM-DD
    std::optional<std::string> fechaHasta;  // YYYY-MM-DD
    std::int32_t page{1};
    std::int32_t limit{20};
};

class ReservacionRepository {
public:
    explicit ReservacionRepository(hotel::database::MongoConnectionManager& connectionManager);

    [[nodiscard]] hotel::models::Reservacion create(const hotel::models::Reservacion& reservacion);
    [[nodiscard]] PagedResult<hotel::models::Reservacion> findAll(const ReservacionFilter& filter);
    [[nodiscard]] std::optional<hotel::models::Reservacion> findById(const std::string& id);
    [[nodiscard]] std::optional<hotel::models::Reservacion> update(
        const hotel::models::Reservacion& reservacion);
    [[nodiscard]] bool updateEstado(const std::string& id,
                                    hotel::models::EstadoReservacion estado,
                                    std::chrono::system_clock::time_point actualizadoEn);
    [[nodiscard]] bool deleteById(const std::string& id);

    // Verifica si existe alguna reservacion en estado bloqueante (pendiente, confirmada, en_curso)
    // para la misma habitacion que se traslape con [fechaEntrada, fechaSalida).
    [[nodiscard]] bool hasOverlappingBlockingReservation(
        const std::string& habitacionId,
        const std::string& fechaEntrada,
        const std::string& fechaSalida,
        const std::optional<std::string>& excludeReservacionId = std::nullopt);

    // Devuelve los IDs de habitaciones que tienen reservaciones bloqueantes en [fechaEntrada, fechaSalida).
    [[nodiscard]] std::unordered_set<std::string> findBlockedHabitacionIds(
        const std::string& fechaEntrada,
        const std::string& fechaSalida);

    // Devuelve todas las reservaciones que intersectan el periodo inclusivo [fechaInicio, fechaFin]
    // para el calculo de metricas administrativas en ReporteService.
    [[nodiscard]] std::vector<hotel::models::Reservacion> findIntersectingPeriod(
        const std::string& fechaInicio,
        const std::string& fechaFin);

    // Conteos de integridad referencial para bajas controladas de huespedes y habitaciones.
    [[nodiscard]] std::int64_t countByHuespedId(const std::string& huespedId,
                                                bool onlyBlocking = false);
    [[nodiscard]] std::int64_t countByHabitacionId(const std::string& habitacionId,
                                                   bool onlyBlocking = false);

private:
    static hotel::models::Reservacion fromDocument(const bsoncxx::document::view& doc);

    hotel::database::MongoConnectionManager& connectionManager_;
    static constexpr const char* kCollectionName = "reservaciones";
};

}  // namespace hotel::repositories