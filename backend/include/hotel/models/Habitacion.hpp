#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "hotel/models/Enums.hpp"

namespace hotel::models {

struct Habitacion {
    std::string id;
    std::string numero;
    std::string tipo;
    std::int32_t capacidad{1};
    double tarifa{0.0};
    EstadoHabitacion estado{EstadoHabitacion::Disponible};
    std::vector<std::string> servicios;
    std::chrono::system_clock::time_point actualizadoEn{};
};

struct HabitacionDisponible {
    Habitacion habitacion;
    std::int32_t noches{0};
    double costoEstimado{0.0};
};

}  // namespace hotel::models