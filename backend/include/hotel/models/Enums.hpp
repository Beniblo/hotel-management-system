#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace hotel::models {

enum class EstadoHabitacion {
    Disponible,
    Ocupada,
    Mantenimiento,
    FueraDeServicio
};

enum class EstadoReservacion {
    Pendiente,
    Confirmada,
    EnCurso,
    Completada,
    Cancelada
};

[[nodiscard]] std::string toString(EstadoHabitacion estado);
[[nodiscard]] std::optional<EstadoHabitacion> parseEstadoHabitacion(std::string_view value);

[[nodiscard]] std::string toString(EstadoReservacion estado);
[[nodiscard]] std::optional<EstadoReservacion> parseEstadoReservacion(std::string_view value);

// Determina si el estado de una reservacion ocupa la habitacion y bloquea traslapes.
[[nodiscard]] bool isBlockingReservationStatus(EstadoReservacion estado) noexcept;

}  // namespace hotel::models