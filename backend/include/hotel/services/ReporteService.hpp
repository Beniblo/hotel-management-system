#pragma once

#include <string>

#include "hotel/models/ResumenReporte.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"

namespace hotel::services {

class ReporteService {
public:
    ReporteService(hotel::repositories::HabitacionRepository& habitacionRepository,
                   hotel::repositories::ReservacionRepository& reservacionRepository);

    [[nodiscard]] hotel::models::ResumenReporte generarResumen(const std::string& fechaInicio,
                                                               const std::string& fechaFin);

private:
    hotel::repositories::HabitacionRepository& habitacionRepo_;
    hotel::repositories::ReservacionRepository& reservacionRepo_;
};

}  // namespace hotel::services