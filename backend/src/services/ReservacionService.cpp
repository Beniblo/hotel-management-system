#include "hotel/services/ReservacionService.hpp"

#include <chrono>

#include "hotel/core/Exceptions.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::services {

using hotel::models::EstadoHabitacion;
using hotel::models::EstadoReservacion;

ReservacionService::ReservacionService(
    hotel::repositories::ReservacionRepository& reservacionRepository,
    hotel::repositories::HuespedRepository& huespedRepository,
    hotel::repositories::HabitacionRepository& habitacionRepository,
    RoomLockManager& roomLockManager)
    : reservacionRepo_(reservacionRepository),
      huespedRepo_(huespedRepository),
      habitacionRepo_(habitacionRepository),
      roomLockManager_(roomLockManager) {}

bool ReservacionService::isValidStateTransition(EstadoReservacion current,
                                                EstadoReservacion target) noexcept {
    if (current == target) {
        return true;
    }

    switch (current) {
        case EstadoReservacion::Pendiente:
            return target == EstadoReservacion::Confirmada ||
                   target == EstadoReservacion::EnCurso ||
                   target == EstadoReservacion::Cancelada;
        case EstadoReservacion::Confirmada:
            return target == EstadoReservacion::EnCurso ||
                   target == EstadoReservacion::Completada ||
                   target == EstadoReservacion::Cancelada;
        case EstadoReservacion::EnCurso:
            return target == EstadoReservacion::Completada ||
                   target == EstadoReservacion::Cancelada;
        case EstadoReservacion::Completada:
        case EstadoReservacion::Cancelada:
            return false;
    }
    return false;
}

hotel::models::Reservacion ReservacionService::crear(const CreateReservacionInput& input) {
    hotel::validators::validateReservacionData(
        input.huespedId, input.habitacionId, input.fechaEntrada, input.fechaSalida, input.personas);

    if (input.estado != EstadoReservacion::Pendiente &&
        input.estado != EstadoReservacion::Confirmada) {
        throw hotel::core::ValidationException(
            "El estado inicial de una reservacion solo puede ser 'pendiente' o 'confirmada'.",
            {{"estado", "Valores permitidos al crear: pendiente, confirmada."}});
    }

    const auto huesped = huespedRepo_.findById(input.huespedId);
    if (!huesped.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro el huesped especificado.", "HUESPED_NOT_FOUND");
    }
    if (!huesped->activo) {
        throw hotel::core::ConflictException(
            "No se puede crear una reservacion para un huesped inactivo.", "HUESPED_INACTIVE");
    }

    const auto noches = hotel::validators::calculateNights(input.fechaEntrada, input.fechaSalida);

    // Seccion critica por habitacion para eliminar condiciones de carrera en reservas simultaneas.
    const auto roomLock = roomLockManager_.lock(input.habitacionId);

    const auto habitacion = habitacionRepo_.findById(input.habitacionId);
    if (!habitacion.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro la habitacion especificada.", "ROOM_NOT_FOUND");
    }
    if (habitacion->estado != EstadoHabitacion::Disponible) {
        throw hotel::core::ConflictException(
            "La habitacion no se encuentra disponible operativamente para nuevas reservaciones.",
            "ROOM_NOT_AVAILABLE");
    }
    if (input.personas > habitacion->capacidad) {
        throw hotel::core::ValidationException(
            "El numero de personas excede la capacidad maxima de la habitacion.",
            {{"personas", "Capacidad maxima de la habitacion: " +
                              std::to_string(habitacion->capacidad) + " personas."}},
            "CAPACITY_EXCEEDED");
    }

    if (reservacionRepo_.hasOverlappingBlockingReservation(
            habitacion->id, input.fechaEntrada, input.fechaSalida)) {
        throw hotel::core::ConflictException(
            "La habitacion ya cuenta con una reservacion traslapada en las fechas solicitadas.",
            "RESERVATION_OVERLAP");
    }

    const auto now = std::chrono::system_clock::now();
    hotel::models::Reservacion nueva;
    nueva.huespedId = huesped->id;
    nueva.habitacionId = habitacion->id;
    nueva.fechaEntrada = input.fechaEntrada;
    nueva.fechaSalida = input.fechaSalida;
    nueva.noches = noches;
    nueva.personas = input.personas;
    nueva.estado = input.estado;
    nueva.tarifaAplicada = hotel::validators::roundCurrency(habitacion->tarifa);
    nueva.total = hotel::validators::calculateReservationTotal(noches, nueva.tarifaAplicada);
    nueva.creadoEn = now;
    nueva.actualizadoEn = now;

    return reservacionRepo_.create(nueva);
}

hotel::repositories::PagedResult<hotel::models::Reservacion> ReservacionService::listar(
    const hotel::repositories::ReservacionFilter& filter) {
    hotel::validators::validatePagination(filter.page, filter.limit);

    if (filter.huespedId.has_value() && !filter.huespedId->empty()) {
        hotel::validators::requireValidObjectId(*filter.huespedId, "huespedId");
    }
    if (filter.habitacionId.has_value() && !filter.habitacionId->empty()) {
        hotel::validators::requireValidObjectId(*filter.habitacionId, "habitacionId");
    }
    if (filter.fechaDesde.has_value() && !filter.fechaDesde->empty() &&
        !hotel::validators::isValidIsoDate(*filter.fechaDesde)) {
        throw hotel::core::ValidationException(
            "Fecha inicial de filtro invalida.",
            {{"fechaDesde", "Debe tener formato valido YYYY-MM-DD."}});
    }
    if (filter.fechaHasta.has_value() && !filter.fechaHasta->empty() &&
        !hotel::validators::isValidIsoDate(*filter.fechaHasta)) {
        throw hotel::core::ValidationException(
            "Fecha final de filtro invalida.",
            {{"fechaHasta", "Debe tener formato valido YYYY-MM-DD."}});
    }

    return reservacionRepo_.findAll(filter);
}

hotel::models::Reservacion ReservacionService::obtenerPorId(const std::string& id) {
    hotel::validators::requireValidObjectId(id, "reservacionId");
    const auto found = reservacionRepo_.findById(id);
    if (!found.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro la reservacion solicitada.", "RESERVATION_NOT_FOUND");
    }
    return *found;
}

hotel::models::Reservacion ReservacionService::actualizar(const std::string& id,
                                                          const UpdateReservacionInput& input) {
    hotel::validators::requireValidObjectId(id, "reservacionId");
    if (!input.hasAnyField()) {
        throw hotel::core::ValidationException(
            "Debes proporcionar al menos un campo para actualizar la reservacion.",
            {{"body", "Campos modificables: habitacionId, fechaEntrada, fechaSalida, personas o estado."}});
    }

    if (input.habitacionId.has_value()) {
        hotel::validators::requireValidObjectId(*input.habitacionId, "habitacionId");
    }

    const auto preview = obtenerPorId(id);
    const std::string targetHabitacionId = input.habitacionId.value_or(preview.habitacionId);

    // Bloqueo de una o dos habitaciones en orden consistente antes de revalidar y escribir.
    const auto roomLock = roomLockManager_.lockTwo(preview.habitacionId, targetHabitacionId);

    hotel::models::Reservacion actual = obtenerPorId(id);

    if (actual.estado == EstadoReservacion::Completada ||
        actual.estado == EstadoReservacion::Cancelada) {
        throw hotel::core::ConflictException(
            "No se puede modificar una reservacion que ya fue completada o cancelada.",
            "INVALID_STATE_TRANSITION");
    }

    const std::string nextHabitacionId = input.habitacionId.value_or(actual.habitacionId);
    const std::string nextFechaEntrada = input.fechaEntrada.value_or(actual.fechaEntrada);
    const std::string nextFechaSalida = input.fechaSalida.value_or(actual.fechaSalida);
    const std::int32_t nextPersonas = input.personas.value_or(actual.personas);
    const EstadoReservacion nextEstado = input.estado.value_or(actual.estado);

    if (!isValidStateTransition(actual.estado, nextEstado)) {
        throw hotel::core::ConflictException(
            "Transicion de estado no permitida para la reservacion.",
            "INVALID_STATE_TRANSITION");
    }

    hotel::validators::validateReservacionData(
        actual.huespedId, nextHabitacionId, nextFechaEntrada, nextFechaSalida, nextPersonas);
    const auto noches = hotel::validators::calculateNights(nextFechaEntrada, nextFechaSalida);

    const auto habitacion = habitacionRepo_.findById(nextHabitacionId);
    if (!habitacion.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro la habitacion especificada.", "ROOM_NOT_FOUND");
    }

    const bool roomChanged = (nextHabitacionId != actual.habitacionId);
    if (roomChanged) {
        if (habitacion->estado != EstadoHabitacion::Disponible) {
            throw hotel::core::ConflictException(
                "La nueva habitacion seleccionada no se encuentra disponible operativamente.",
                "ROOM_NOT_AVAILABLE");
        }
    } else if (habitacion->estado == EstadoHabitacion::Mantenimiento ||
               habitacion->estado == EstadoHabitacion::FueraDeServicio) {
        throw hotel::core::ConflictException(
            "La habitacion se encuentra en mantenimiento o fuera de servicio.",
            "ROOM_NOT_AVAILABLE");
    }

    if (nextPersonas > habitacion->capacidad) {
        throw hotel::core::ValidationException(
            "El numero de personas excede la capacidad maxima de la habitacion.",
            {{"personas", "Capacidad maxima de la habitacion: " +
                              std::to_string(habitacion->capacidad) + " personas."}},
            "CAPACITY_EXCEEDED");
    }

    if (hotel::models::isBlockingReservationStatus(nextEstado)) {
        if (reservacionRepo_.hasOverlappingBlockingReservation(
                nextHabitacionId, nextFechaEntrada, nextFechaSalida, actual.id)) {
            throw hotel::core::ConflictException(
                "La modificacion genera un traslape con otra reservacion existente.",
                "RESERVATION_OVERLAP");
        }
    }

    const double nextTarifa =
        roomChanged ? hotel::validators::roundCurrency(habitacion->tarifa) : actual.tarifaAplicada;

    actual.habitacionId = nextHabitacionId;
    actual.fechaEntrada = nextFechaEntrada;
    actual.fechaSalida = nextFechaSalida;
    actual.noches = noches;
    actual.personas = nextPersonas;
    actual.estado = nextEstado;
    actual.tarifaAplicada = nextTarifa;
    actual.total = hotel::validators::calculateReservationTotal(noches, nextTarifa);
    actual.actualizadoEn = std::chrono::system_clock::now();

    const auto updated = reservacionRepo_.update(actual);
    if (!updated.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro la reservacion solicitada.", "RESERVATION_NOT_FOUND");
    }
    return *updated;
}

hotel::models::Reservacion ReservacionService::cancelar(const std::string& id) {
    const auto preview = obtenerPorId(id);
    const auto roomLock = roomLockManager_.lock(preview.habitacionId);

    hotel::models::Reservacion actual = obtenerPorId(id);
    if (actual.estado == EstadoReservacion::Completada ||
        actual.estado == EstadoReservacion::Cancelada) {
        throw hotel::core::ConflictException(
            "No se puede cancelar una reservacion que ya esta completada o cancelada.",
            "INVALID_STATE_TRANSITION");
    }

    const auto now = std::chrono::system_clock::now();
    if (!reservacionRepo_.updateEstado(actual.id, EstadoReservacion::Cancelada, now)) {
        throw hotel::core::NotFoundException(
            "No se encontro la reservacion solicitada.", "RESERVATION_NOT_FOUND");
    }

    actual.estado = EstadoReservacion::Cancelada;
    actual.actualizadoEn = now;
    return actual;
}

void ReservacionService::eliminar(const std::string& id) {
    const auto preview = obtenerPorId(id);
    const auto roomLock = roomLockManager_.lock(preview.habitacionId);

    const auto actual = obtenerPorId(id);
    if (actual.estado == EstadoReservacion::Confirmada ||
        actual.estado == EstadoReservacion::EnCurso) {
        throw hotel::core::ConflictException(
            "No se puede eliminar fisicamente una reservacion confirmada o en curso; cancelala primero.",
            "RESERVATION_ACTIVE_DELETE_FORBIDDEN");
    }

    if (!reservacionRepo_.deleteById(actual.id)) {
        throw hotel::core::NotFoundException(
            "No se encontro la reservacion solicitada.", "RESERVATION_NOT_FOUND");
    }
}

}  // namespace hotel::services