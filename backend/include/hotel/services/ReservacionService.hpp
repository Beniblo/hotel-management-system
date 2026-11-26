#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "hotel/models/Enums.hpp"
#include "hotel/models/Reservacion.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/HuespedRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"
#include "hotel/services/RoomLockManager.hpp"

namespace hotel::services {

struct CreateReservacionInput {
    std::string huespedId;
    std::string habitacionId;
    std::string fechaEntrada;  // YYYY-MM-DD
    std::string fechaSalida;   // YYYY-MM-DD
    std::int32_t personas{1};
    hotel::models::EstadoReservacion estado{hotel::models::EstadoReservacion::Confirmada};
};

struct UpdateReservacionInput {
    std::optional<std::string> habitacionId;
    std::optional<std::string> fechaEntrada;
    std::optional<std::string> fechaSalida;
    std::optional<std::int32_t> personas;
    std::optional<hotel::models::EstadoReservacion> estado;

    [[nodiscard]] bool hasAnyField() const noexcept {
        return habitacionId.has_value() || fechaEntrada.has_value() || fechaSalida.has_value() ||
               personas.has_value() || estado.has_value();
    }
};

class ReservacionService {
public:
    ReservacionService(hotel::repositories::ReservacionRepository& reservacionRepository,
                       hotel::repositories::HuespedRepository& huespedRepository,
                       hotel::repositories::HabitacionRepository& habitacionRepository,
                       RoomLockManager& roomLockManager);

    [[nodiscard]] hotel::models::Reservacion crear(const CreateReservacionInput& input);
    [[nodiscard]] hotel::repositories::PagedResult<hotel::models::Reservacion> listar(
        const hotel::repositories::ReservacionFilter& filter);
    [[nodiscard]] hotel::models::Reservacion obtenerPorId(const std::string& id);
    [[nodiscard]] hotel::models::Reservacion actualizar(const std::string& id,
                                                        const UpdateReservacionInput& input);
    [[nodiscard]] hotel::models::Reservacion cancelar(const std::string& id);
    void eliminar(const std::string& id);

private:
    static bool isValidStateTransition(hotel::models::EstadoReservacion current,
                                       hotel::models::EstadoReservacion target) noexcept;

    hotel::repositories::ReservacionRepository& reservacionRepo_;
    hotel::repositories::HuespedRepository& huespedRepo_;
    hotel::repositories::HabitacionRepository& habitacionRepo_;
    RoomLockManager& roomLockManager_;
};

}  // namespace hotel::services