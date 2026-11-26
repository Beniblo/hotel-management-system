#include "hotel/services/HuespedService.hpp"

#include <chrono>

#include "hotel/core/Exceptions.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::services {

HuespedService::HuespedService(hotel::repositories::HuespedRepository& huespedRepository,
                               hotel::repositories::ReservacionRepository& reservacionRepository)
    : huespedRepo_(huespedRepository), reservacionRepo_(reservacionRepository) {}

hotel::models::Huesped HuespedService::crear(const CreateHuespedInput& input) {
    hotel::validators::validateHuespedData(
        input.nombre, input.correo, input.telefono, input.preferencias);

    const std::string normalizedEmail = hotel::validators::normalizeEmail(input.correo);
    if (huespedRepo_.findByEmail(normalizedEmail).has_value()) {
        throw hotel::core::ConflictException(
            "Ya existe un huesped registrado con ese correo electronico.", "DUPLICATE_EMAIL");
    }

    const auto now = std::chrono::system_clock::now();
    hotel::models::Huesped nuevo;
    nuevo.nombre = hotel::validators::trim(input.nombre);
    nuevo.correo = normalizedEmail;
    nuevo.telefono = hotel::validators::trim(input.telefono);
    nuevo.preferencias = hotel::validators::normalizeStringList(input.preferencias);
    nuevo.activo = true;
    nuevo.fechaRegistro = now;
    nuevo.actualizadoEn = now;

    return huespedRepo_.create(nuevo);
}

hotel::repositories::PagedResult<hotel::models::Huesped> HuespedService::listar(
    const hotel::repositories::HuespedFilter& filter) {
    hotel::validators::validatePagination(filter.page, filter.limit);
    return huespedRepo_.findAll(filter);
}

hotel::models::Huesped HuespedService::obtenerPorId(const std::string& id) {
    hotel::validators::requireValidObjectId(id, "huespedId");
    const auto found = huespedRepo_.findById(id);
    if (!found.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro el huesped solicitado.", "HUESPED_NOT_FOUND");
    }
    return *found;
}

hotel::models::Huesped HuespedService::actualizar(const std::string& id,
                                                  const UpdateHuespedInput& input) {
    hotel::validators::requireValidObjectId(id, "huespedId");
    if (!input.hasAnyField()) {
        throw hotel::core::ValidationException(
            "Debes proporcionar al menos un campo para actualizar el huesped.",
            {{"body", "Se requiere al menos un campo modificable (nombre, correo, telefono, preferencias o activo)."}});
    }

    hotel::models::Huesped actual = obtenerPorId(id);

    const std::string nextNombre =
        input.nombre.has_value() ? hotel::validators::trim(*input.nombre) : actual.nombre;
    const std::string nextCorreo =
        input.correo.has_value() ? hotel::validators::normalizeEmail(*input.correo) : actual.correo;
    const std::string nextTelefono =
        input.telefono.has_value() ? hotel::validators::trim(*input.telefono) : actual.telefono;
    const std::vector<std::string> nextPreferencias =
        input.preferencias.has_value()
            ? hotel::validators::normalizeStringList(*input.preferencias)
            : actual.preferencias;
    const bool nextActivo = input.activo.value_or(actual.activo);

    hotel::validators::validateHuespedData(nextNombre, nextCorreo, nextTelefono, nextPreferencias);

    if (nextCorreo != actual.correo) {
        const auto existingByEmail = huespedRepo_.findByEmail(nextCorreo);
        if (existingByEmail.has_value() && existingByEmail->id != actual.id) {
            throw hotel::core::ConflictException(
                "Ya existe otro huesped registrado con ese correo electronico.",
                "DUPLICATE_EMAIL");
        }
    }

    if (actual.activo && !nextActivo) {
        if (reservacionRepo_.countByHuespedId(actual.id, true) > 0) {
            throw hotel::core::ConflictException(
                "No se puede desactivar el huesped porque tiene reservaciones activas o pendientes.",
                "HUESPED_HAS_ACTIVE_RESERVATIONS");
        }
    }

    actual.nombre = nextNombre;
    actual.correo = nextCorreo;
    actual.telefono = nextTelefono;
    actual.preferencias = nextPreferencias;
    actual.activo = nextActivo;
    actual.actualizadoEn = std::chrono::system_clock::now();

    const auto updated = huespedRepo_.update(actual);
    if (!updated.has_value()) {
        throw hotel::core::NotFoundException(
            "No se encontro el huesped solicitado.", "HUESPED_NOT_FOUND");
    }
    return *updated;
}

DeleteHuespedResult HuespedService::eliminar(const std::string& id, bool fisico) {
    hotel::models::Huesped actual = obtenerPorId(id);

    if (fisico) {
        if (reservacionRepo_.countByHuespedId(actual.id, false) > 0) {
            throw hotel::core::ConflictException(
                "No se puede eliminar fisicamente el huesped porque tiene reservaciones registradas.",
                "HUESPED_HAS_RESERVATIONS");
        }

        if (!huespedRepo_.deleteById(actual.id)) {
            throw hotel::core::NotFoundException(
                "No se encontro el huesped solicitado.", "HUESPED_NOT_FOUND");
        }

        DeleteHuespedResult result;
        result.eliminacionFisica = true;
        return result;
    }

    if (reservacionRepo_.countByHuespedId(actual.id, true) > 0) {
        throw hotel::core::ConflictException(
            "No se puede dar de baja al huesped porque tiene reservaciones activas o pendientes.",
            "HUESPED_HAS_ACTIVE_RESERVATIONS");
    }

    const auto now = std::chrono::system_clock::now();
    if (!huespedRepo_.setActive(actual.id, false, now)) {
        throw hotel::core::NotFoundException(
            "No se encontro el huesped solicitado.", "HUESPED_NOT_FOUND");
    }

    actual.activo = false;
    actual.actualizadoEn = now;

    DeleteHuespedResult result;
    result.eliminacionFisica = false;
    result.huespedDesactivado = actual;
    return result;
}

}  // namespace hotel::services