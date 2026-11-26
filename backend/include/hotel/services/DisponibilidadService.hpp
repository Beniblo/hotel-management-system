#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "hotel/models/Habitacion.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"

namespace hotel::services {

struct DisponibilidadQuery {
    std::string fechaEntrada;  // YYYY-MM-DD
    std::string fechaSalida;   // YYYY-MM-DD
    std::optional<std::int32_t> personas;
    std::optional<std::string> tipo;
};

struct DisponibilidadResult {
    std::string fechaEntrada;
    std::string fechaSalida;
    std::int32_t noches{0};
    std::vector<hotel::models::HabitacionDisponible> habitaciones;
};

class DisponibilidadService {
public:
    DisponibilidadService(hotel::repositories::HabitacionRepository& habitacionRepository,
                          hotel::repositories::ReservacionRepository& reservacionRepository);

    [[nodiscard]] DisponibilidadResult consultar(const DisponibilidadQuery& query);

private:
    hotel::repositories::HabitacionRepository& habitacionRepo_;
    hotel::repositories::ReservacionRepository& reservacionRepo_;
};

}  // namespace hotel::services