#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "hotel/models/Enums.hpp"

namespace hotel::models {

struct Reservacion {
    std::string id;
    std::string huespedId;
    std::string habitacionId;
    std::string fechaEntrada;  // Formato estricto YYYY-MM-DD (normalizado a 00:00:00Z en BSON)
    std::string fechaSalida;   // Formato estricto YYYY-MM-DD (normalizado a 00:00:00Z en BSON)
    std::int32_t noches{0};
    std::int32_t personas{1};
    EstadoReservacion estado{EstadoReservacion::Confirmada};
    double tarifaAplicada{0.0};
    double total{0.0};
    std::chrono::system_clock::time_point creadoEn{};
    std::chrono::system_clock::time_point actualizadoEn{};
};

}  // namespace hotel::models