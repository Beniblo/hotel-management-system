#include "hotel/services/ReporteService.hpp"

#include <unordered_set>

#include "hotel/models/Enums.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::services {

using hotel::models::EstadoHabitacion;
using hotel::models::EstadoReservacion;

ReporteService::ReporteService(hotel::repositories::HabitacionRepository& habitacionRepository,
                               hotel::repositories::ReservacionRepository& reservacionRepository)
    : habitacionRepo_(habitacionRepository), reservacionRepo_(reservacionRepository) {}

hotel::models::ResumenReporte ReporteService::generarResumen(const std::string& fechaInicio,
                                                             const std::string& fechaFin) {
    hotel::validators::validateReportDateRange(fechaInicio, fechaFin);

    const auto habitaciones = habitacionRepo_.findAllUnpaged();
    const auto reservaciones = reservacionRepo_.findIntersectingPeriod(fechaInicio, fechaFin);

    hotel::models::ResumenReporte resumen;
    resumen.fechaInicio = fechaInicio;
    resumen.fechaFin = fechaFin;
    resumen.totalHabitaciones = static_cast<std::int64_t>(habitaciones.size());

    // Inicializar todos los estados para una salida JSON consistente
    resumen.reservacionesPorEstado["pendiente"] = 0;
    resumen.reservacionesPorEstado["confirmada"] = 0;
    resumen.reservacionesPorEstado["en_curso"] = 0;
    resumen.reservacionesPorEstado["completada"] = 0;
    resumen.reservacionesPorEstado["cancelada"] = 0;

    std::unordered_set<std::string> operativeRoomIds;
    for (const auto& hab : habitaciones) {
        if (hab.estado == EstadoHabitacion::Mantenimiento ||
            hab.estado == EstadoHabitacion::FueraDeServicio) {
            ++resumen.habitacionesNoOperativas;
        } else {
            operativeRoomIds.insert(hab.id);
        }
    }

    std::unordered_set<std::string> occupiedRoomIds;
    double ingresosAcumulados = 0.0;

    for (const auto& r : reservaciones) {
        ++resumen.totalReservaciones;
        const std::string estadoKey = hotel::models::toString(r.estado);
        ++resumen.reservacionesPorEstado[estadoKey];

        if (r.estado != EstadoReservacion::Cancelada) {
            ingresosAcumulados += r.total;
            if (operativeRoomIds.find(r.habitacionId) != operativeRoomIds.end()) {
                occupiedRoomIds.insert(r.habitacionId);
            }
        }
    }

    resumen.habitacionesOcupadas = static_cast<std::int64_t>(occupiedRoomIds.size());
    resumen.habitacionesDisponibles =
        static_cast<std::int64_t>(operativeRoomIds.size()) - resumen.habitacionesOcupadas;

    if (resumen.totalHabitaciones > 0) {
        resumen.porcentajeOcupacion = hotel::validators::roundCurrency(
            (static_cast<double>(resumen.habitacionesOcupadas) * 100.0) /
            static_cast<double>(resumen.totalHabitaciones));
    } else {
        resumen.porcentajeOcupacion = 0.0;
    }

    resumen.ingresosEstimados = hotel::validators::roundCurrency(ingresosAcumulados);
    return resumen;
}

}  // namespace hotel::services