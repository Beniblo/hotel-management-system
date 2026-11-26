#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace hotel::models {

struct ResumenReporte {
    std::string fechaInicio;
    std::string fechaFin;
    std::int64_t totalReservaciones{0};
    std::map<std::string, std::int64_t> reservacionesPorEstado;
    std::int64_t totalHabitaciones{0};
    std::int64_t habitacionesOcupadas{0};
    std::int64_t habitacionesDisponibles{0};
    std::int64_t habitacionesNoOperativas{0};
    double porcentajeOcupacion{0.0};
    double ingresosEstimados{0.0};
};

}  // namespace hotel::models