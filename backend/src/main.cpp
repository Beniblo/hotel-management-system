#include <exception>
#include <string>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/models/Enums.hpp"
#include "hotel/models/Habitacion.hpp"
#include "hotel/models/Huesped.hpp"
#include "hotel/models/Reservacion.hpp"
#include "hotel/models/ResumenReporte.hpp"
#include "hotel/validators/InputValidators.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::AppException;
    using hotel::core::Logger;
    using namespace hotel::models;
    using namespace hotel::validators;

    try {
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion de modelos puros y validadores (Parte 4A)...");

        // 1. Validacion de entidades de dominio
        validateHuespedData("Ana Lopez", "Ana.Lopez@Ejemplo.COM", "477-000-0001", {"piso alto"});
        validateHabitacionData("201", "suite", 4, 2600.0, {"wifi", "balcon"});

        // 2. Calculo de noches y total
        const auto noches = calculateNights("2026-12-05", "2026-12-08");
        const auto total = calculateReservationTotal(noches, 1450.0);

        // 3. Regla matematica de traslapes [E1, S1) vs [E2, S2)
        const bool traslapeReal = datesOverlap("2026-12-05", "2026-12-08", "2026-12-07", "2026-12-10");
        const bool bordePermitido = datesOverlap("2026-12-05", "2026-12-08", "2026-12-08", "2026-12-10");

        // 4. Conversion ida y vuelta de fecha UTC sin desfase de zona horaria
        const auto utcMs = isoDateToUtcMidnightMs("2026-12-05");
        const auto roundTripDate = utcMsToIsoDate(utcMs);

        // 5. Enums de estado
        const auto estadoHab = parseEstadoHabitacion("disponible");
        const auto estadoRes = parseEstadoReservacion("confirmada");

        if (noches != 3 || total != 4350.0 || !traslapeReal || bordePermitido ||
            roundTripDate != "2026-12-05" || !estadoHab.has_value() || !estadoRes.has_value() ||
            !isBlockingReservationStatus(*estadoRes)) {
            Logger::error("Fallo alguna asercion de verificacion en Parte 4A.");
            return 1;
        }

        Logger::info("Noches calculadas: " + std::to_string(noches) +
                     ", Total: " + std::to_string(total) +
                     ", Fecha UTC ida/vuelta: " + roundTripDate);
        Logger::info("Traslape real: true | Borde check-out=check-in: false (permitido)");
        Logger::info("[OK] Parte 4A verificada correctamente (models/ y validators/ operativos).");
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Error controlado -> HTTP " + std::to_string(ex.httpStatus()) +
                      " [" + ex.errorCode() + "]: " + ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico: ") + ex.what());
        return 1;
    }
}