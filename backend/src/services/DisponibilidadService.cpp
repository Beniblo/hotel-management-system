#include "hotel/services/DisponibilidadService.hpp"

#include "hotel/core/Exceptions.hpp"
#include "hotel/models/Enums.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::services {

DisponibilidadService::DisponibilidadService(
    hotel::repositories::HabitacionRepository& habitacionRepository,
    hotel::repositories::ReservacionRepository& reservacionRepository)
    : habitacionRepo_(habitacionRepository), reservacionRepo_(reservacionRepository) {}

DisponibilidadResult DisponibilidadService::consultar(const DisponibilidadQuery& query) {
    const auto noches = hotel::validators::calculateNights(query.fechaEntrada, query.fechaSalida);

    if (query.personas.has_value() && (*query.personas < 1 || *query.personas > 20)) {
        throw hotel::core::ValidationException(
            "Parametro 'personas' invalido.",
            {{"personas", "El numero de personas debe estar entre 1 y 20."}});
    }

    std::optional<std::string> normalizedTipo;
    if (query.tipo.has_value()) {
        const std::string cleaned = hotel::validators::toLower(hotel::validators::trim(*query.tipo));
        if (!cleaned.empty()) {
            if (!hotel::validators::isValidRoomType(cleaned)) {
                throw hotel::core::ValidationException(
                    "Tipo de habitacion invalido.",
                    {{"tipo", "Valores permitidos: sencilla, doble, triple, suite o familiar."}});
            }
            normalizedTipo = cleaned;
        }
    }

    const auto candidatas = habitacionRepo_.findAllUnpaged(
        hotel::models::EstadoHabitacion::Disponible, normalizedTipo, query.personas);

    const auto blockedRoomIds =
        reservacionRepo_.findBlockedHabitacionIds(query.fechaEntrada, query.fechaSalida);

    DisponibilidadResult result;
    result.fechaEntrada = query.fechaEntrada;
    result.fechaSalida = query.fechaSalida;
    result.noches = noches;

    for (const auto& hab : candidatas) {
        if (blockedRoomIds.find(hab.id) != blockedRoomIds.end()) {
            continue;
        }

        hotel::models::HabitacionDisponible disponible;
        disponible.habitacion = hab;
        disponible.noches = noches;
        disponible.costoEstimado = hotel::validators::calculateReservationTotal(noches, hab.tarifa);
        result.habitaciones.push_back(std::move(disponible));
    }

    return result;
}

}  // namespace hotel::services