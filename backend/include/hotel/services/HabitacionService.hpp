#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "hotel/models/Enums.hpp"
#include "hotel/models/Habitacion.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"

namespace hotel::services {

struct CreateHabitacionInput {
    std::string numero;
    std::string tipo;
    std::int32_t capacidad{1};
    double tarifa{0.0};
    hotel::models::EstadoHabitacion estado{hotel::models::EstadoHabitacion::Disponible};
    std::vector<std::string> servicios;
};

struct UpdateHabitacionInput {
    std::optional<std::string> numero;
    std::optional<std::string> tipo;
    std::optional<std::int32_t> capacidad;
    std::optional<double> tarifa;
    std::optional<hotel::models::EstadoHabitacion> estado;
    std::optional<std::vector<std::string>> servicios;

    [[nodiscard]] bool hasAnyField() const noexcept {
        return numero.has_value() || tipo.has_value() || capacidad.has_value() ||
               tarifa.has_value() || estado.has_value() || servicios.has_value();
    }
};

class HabitacionService {
public:
    HabitacionService(hotel::repositories::HabitacionRepository& habitacionRepository,
                      hotel::repositories::ReservacionRepository& reservacionRepository);

    [[nodiscard]] hotel::models::Habitacion crear(const CreateHabitacionInput& input);
    [[nodiscard]] hotel::repositories::PagedResult<hotel::models::Habitacion> listar(
        const hotel::repositories::HabitacionFilter& filter);
    [[nodiscard]] hotel::models::Habitacion obtenerPorId(const std::string& id);
    [[nodiscard]] hotel::models::Habitacion actualizar(const std::string& id,
                                                       const UpdateHabitacionInput& input);
    [[nodiscard]] hotel::models::Habitacion cambiarEstado(
        const std::string& id,
        hotel::models::EstadoHabitacion nuevoEstado);
    void eliminar(const std::string& id);

private:
    void ensureCanTransitionToUnavailable(const std::string& habitacionId,
                                          hotel::models::EstadoHabitacion targetState);

    hotel::repositories::HabitacionRepository& habitacionRepo_;
    hotel::repositories::ReservacionRepository& reservacionRepo_;
};

}  // namespace hotel::services