#include "hotel/services/HabitacionService.hpp"

#include <chrono>
#include <cmath>

#include "hotel/core/Exceptions.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::services {

HabitacionService::HabitacionService(
    hotel::repositories::HabitacionRepository& habitacionRepository,
    hotel::repositories::ReservacionRepository& reservacionRepository)
    : habitacionRepo_(habitacionRepository), reservacionRepo_(reservacionRepository) {}

void HabitacionService::ensureCanTransitionToUnavailable(
    const std::string& habitacionId,
    hotel::models::EstadoHabitacion targetState) {
    if (targetState == hotel::models::EstadoHabitacion::Mantenimiento ||
        targetState == hotel::models::EstadoHabitacion::FueraDeServicio) {
        if (reservacionRepo_.countByHabitacionId(habitacionId, true) > 0) {
            throw hotel::core::ConflictException(
                "No se puede poner la habitacion en mantenimiento o fuera de servicio mientras tenga reservaciones activas o pendientes.",
                "ROOM_HAS_ACTIVE_RESERVATIONS");
        }
    }
}

hotel::models::Habitacion HabitacionService::crear(const CreateHabitacionInput& input) {
    hotel::validators::validateHabitacionData(
        input.numero, input.tipo, input.capacidad, input.tarifa, input.servicios);

    const std::string normalizedNumero = hotel::validators::normalizeRoomNumber(input.numero);
    if (habitacionRepo_.findByNumero(normalizedNumero).has_value()) {
        throw hotel::core::ConflictException(
            "Ya existe una habitacion registrada con ese numero.", "DUPLICATE_ROOM_NUMBER");
    }

    hotel::models::Habitacion nueva;
    nueva.numero = normalizedNumero;
    nueva.tipo = hotel::validators::toLower(hotel::validators::trim(input.tipo));
    nueva.capacidad = input.capacidad;
    nueva.tarifa = hotel::validators::roundCurrency(input.tarifa);
    nueva.estado = input.estado;
    nueva.servicios = hotel::validators::normalizeStringList(input.servicios);
    nueva.actualizadoEn = std::chrono::system_clock::now();

    return habitacionRepo_.create(nueva);
}

hotel::repositories::PagedResult<hotel::models::Habitacion> HabitacionService::listar(
    const hotel::repositories::HabitacionFilter& filter) {
    hotel::validators::validatePagination(filter.page, filter.limit);

    if (filter.tipo.has_value() && !hotel::validators::trim(*filter.tipo).empty() &&
        !hotel::validators::isValidRoomType(*filter.tipo)) {
        throw hotel::core::BadRequestException(
            "El filtro 'tipo' debe ser: sencilla, doble, triple, suite o familiar.",
            "INVALID_ROOM_TYPE");
    }
    if (filter.capacidadMin.has_value() && *filter.capacidadMin < 1) {
        throw hotel::core::BadRequestException(
            "El filtro 'capacidadMin' debe ser un entero mayor o igual a 1.",
            "INVALID_FILTER");
    }
    if (filter.tarifaMax.has_value() &&
        (!std::isfinite(*filter.tarifaMax) || *filter.tarifaMax <= 0.0)) {
        throw hotel::core::BadRequestException(
            "El filtro 'tarifaMax' debe ser un monto positivo.", "INVALID_FILTER");
    }

    return habitacionRepo_.findAll(filter);
}

hotel::models::Habitacion HabitacionService::obtenerPorId(const std::string& id) {
    hotel::validators::requireValidObjectId(id, "habitacionId");
    const auto found = habitacionRepo_.findById(id);
    if (!found.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro la habitacion solicitada.", "ROOM_NOT_FOUND");
    }
    return *found;
}

hotel::models::Habitacion HabitacionService::actualizar(const std::string& id,
                                                        const UpdateHabitacionInput& input) {
    hotel::validators::requireValidObjectId(id, "habitacionId");
    if (!input.hasAnyField()) {
        throw hotel::core::ValidationException(
            "Debes proporcionar al menos un campo para actualizar la habitacion.",
            {{"body", "Se requiere al menos un campo modificable (numero, tipo, capacidad, tarifa, estado o servicios)."}});
    }

    hotel::models::Habitacion actual = obtenerPorId(id);

    const std::string nextNumero =
        input.numero.has_value() ? hotel::validators::normalizeRoomNumber(*input.numero) : actual.numero;
    const std::string nextTipo =
        input.tipo.has_value() ? hotel::validators::toLower(hotel::validators::trim(*input.tipo))
                               : actual.tipo;
    const std::int32_t nextCapacidad = input.capacidad.value_or(actual.capacidad);
    const double nextTarifa =
        input.tarifa.has_value() ? hotel::validators::roundCurrency(*input.tarifa) : actual.tarifa;
    const auto nextEstado = input.estado.value_or(actual.estado);
    const std::vector<std::string> nextServicios =
        input.servicios.has_value()
            ? hotel::validators::normalizeStringList(*input.servicios)
            : actual.servicios;

    hotel::validators::validateHabitacionData(
        nextNumero, nextTipo, nextCapacidad, nextTarifa, nextServicios);

    if (nextNumero != actual.numero) {
        const auto existingByNumero = habitacionRepo_.findByNumero(nextNumero);
        if (existingByNumero.has_value() && existingByNumero->id != actual.id) {
            throw hotel::core::ConflictException(
                "Ya existe otra habitacion registrada con ese numero.", "DUPLICATE_ROOM_NUMBER");
        }
    }

    if (nextEstado != actual.estado) {
        ensureCanTransitionToUnavailable(actual.id, nextEstado);
    }

    actual.numero = nextNumero;
    actual.tipo = nextTipo;
    actual.capacidad = nextCapacidad;
    actual.tarifa = nextTarifa;
    actual.estado = nextEstado;
    actual.servicios = nextServicios;
    actual.actualizadoEn = std::chrono::system_clock::now();

    const auto updated = habitacionRepo_.update(actual);
    if (!updated.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro la habitacion solicitada.", "ROOM_NOT_FOUND");
    }
    return *updated;
}

hotel::models::Habitacion HabitacionService::cambiarEstado(
    const std::string& id,
    hotel::models::EstadoHabitacion nuevoEstado) {
    hotel::models::Habitacion actual = obtenerPorId(id);

    if (nuevoEstado != actual.estado) {
        ensureCanTransitionToUnavailable(actual.id, nuevoEstado);
    }

    const auto now = std::chrono::system_clock::now();
    if (!habitacionRepo_.updateEstado(actual.id, nuevoEstado, now)) {
        throw hotel::core::NotFoundException(
            "No se encontro la habitacion solicitada.", "ROOM_NOT_FOUND");
    }

    actual.estado = nuevoEstado;
    actual.actualizadoEn = now;
    return actual;
}

void HabitacionService::eliminar(const std::string& id) {
    hotel::models::Habitacion actual = obtenerPorId(id);

    if (reservacionRepo_.countByHabitacionId(actual.id, false) > 0) {
        throw hotel::core::ConflictException(
            "No se puede eliminar la habitacion porque tiene reservaciones registradas.",
            "ROOM_HAS_RESERVATIONS");
    }

    if (!habitacionRepo_.deleteById(actual.id)) {
        throw hotel::core::NotFoundException(
            "No se encontro la habitacion solicitada.", "ROOM_NOT_FOUND");
    }
}

}  // namespace hotel::services