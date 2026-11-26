#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/HuespedRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::AppException;
    using hotel::core::Logger;
    using hotel::database::MongoConnectionManager;
    using hotel::models::EstadoReservacion;
    using hotel::models::Reservacion;
    using hotel::repositories::HabitacionRepository;
    using hotel::repositories::HuespedRepository;
    using hotel::repositories::ReservacionFilter;
    using hotel::repositories::ReservacionRepository;

    std::string tempReservacionId;

    try {
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion de ReservacionRepository contra Atlas (Parte 4C)...");

        MongoConnectionManager dbManager(config);
        HuespedRepository huespedRepo(dbManager);
        HabitacionRepository habitacionRepo(dbManager);
        ReservacionRepository reservacionRepo(dbManager);

        // 1. Obtener entidades semilla existentes en HotelDB
        const auto ana = huespedRepo.findByEmail("ana.lopez@ejemplo.com");
        const auto hab101 = habitacionRepo.findByNumero("101");
        const auto hab102 = habitacionRepo.findByNumero("102");
        if (!ana.has_value() || !hab101.has_value() || !hab102.has_value()) {
            throw std::runtime_error(
                "No se encontraron los datos semilla esperados (ana.lopez@ejemplo.com, hab 101 y 102).");
        }

        // 2. Verificar consulta de traslapes sobre la reservacion semilla (hab 102: 2026-12-05 a 2026-12-08)
        const bool traslapeDetectado = reservacionRepo.hasOverlappingBlockingReservation(
            hab102->id, "2026-12-06", "2026-12-09");
        const bool bordeLibre = !reservacionRepo.hasOverlappingBlockingReservation(
            hab102->id, "2026-12-08", "2026-12-10");
        if (!traslapeDetectado || !bordeLibre) {
            throw std::runtime_error("Fallo la verificacion de traslape/borde en hab 102.");
        }
        Logger::info("[Traslapes] Hab 102 (2026-12-06..09): bloqueada | Borde (2026-12-08..10): libre.");

        // 3. Verificar findBlockedHabitacionIds y findIntersectingPeriod
        const auto blockedIds = reservacionRepo.findBlockedHabitacionIds("2026-12-06", "2026-12-07");
        if (blockedIds.find(hab102->id) == blockedIds.end()) {
            throw std::runtime_error("findBlockedHabitacionIds no incluyo la habitacion 102.");
        }

        const auto periodoDic = reservacionRepo.findIntersectingPeriod("2026-12-01", "2026-12-31");
        if (periodoDic.empty()) {
            throw std::runtime_error("findIntersectingPeriod no devolvio la reservacion semilla.");
        }
        Logger::info("[Consultas] Habitaciones bloqueadas (dic 06-07): " +
                     std::to_string(blockedIds.size()) +
                     " | Reservaciones en dic 2026: " + std::to_string(periodoDic.size()));

        // 4. Ciclo CRUD completo y autolimpiable en hab 101
        const auto now = std::chrono::system_clock::now();
        Reservacion nueva;
        nueva.huespedId = ana->id;
        nueva.habitacionId = hab101->id;
        nueva.fechaEntrada = "2027-01-10";
        nueva.fechaSalida = "2027-01-13";
        nueva.noches = 3;
        nueva.personas = 1;
        nueva.estado = EstadoReservacion::Confirmada;
        nueva.tarifaAplicada = hab101->tarifa;
        nueva.total = hab101->tarifa * 3.0;
        nueva.creadoEn = now;
        nueva.actualizadoEn = now;

        const auto creada = reservacionRepo.create(nueva);
        tempReservacionId = creada.id;
        Logger::info("[Reservaciones] insert_one OK -> id=" + tempReservacionId);

        const auto leida = reservacionRepo.findById(tempReservacionId);
        if (!leida.has_value() || leida->noches != 3 || leida->fechaEntrada != "2027-01-10") {
            throw std::runtime_error("findById no devolvio los datos esperados.");
        }

        // Excluir el propio ID al simular reprogramacion
        if (reservacionRepo.hasOverlappingBlockingReservation(
                hab101->id, "2027-01-10", "2027-01-13", tempReservacionId)) {
            throw std::runtime_error("hasOverlappingBlockingReservation fallo al excluir el propio ID.");
        }

        // Cancelar y comprobar que ya no bloquea
        if (!reservacionRepo.updateEstado(
                tempReservacionId, EstadoReservacion::Cancelada, std::chrono::system_clock::now())) {
            throw std::runtime_error("updateEstado a Cancelada fallo.");
        }
        if (reservacionRepo.hasOverlappingBlockingReservation(
                hab101->id, "2027-01-10", "2027-01-13")) {
            throw std::runtime_error("Una reservacion cancelada no debe bloquear fechas.");
        }

        ReservacionFilter filtro;
        filtro.habitacionId = hab101->id;
        filtro.estado = EstadoReservacion::Cancelada;
        const auto filtradas = reservacionRepo.findAll(filtro);
        if (filtradas.total < 1) {
            throw std::runtime_error("findAll con filtros combinados no encontro la reservacion.");
        }

        const auto totalPorHuesped = reservacionRepo.countByHuespedId(ana->id);
        const auto bloqueantesHab102 = reservacionRepo.countByHabitacionId(hab102->id, true);
        Logger::info("[Conteos] Reservas de Ana: " + std::to_string(totalPorHuesped) +
                     " | Bloqueantes en hab 102: " + std::to_string(bloqueantesHab102));

        // Limpieza
        if (!reservacionRepo.deleteById(tempReservacionId)) {
            throw std::runtime_error("deleteById en reservaciones fallo.");
        }
        tempReservacionId.clear();
        Logger::info("[Reservaciones] delete_one OK (limpieza verificada).");

        Logger::info("[OK] Parte 4C verificada correctamente (ReservacionRepository operativo).");
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Error controlado -> HTTP " + std::to_string(ex.httpStatus()) +
                      " [" + ex.errorCode() + "]: " + ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico en Parte 4C: ") + ex.what());
        return 1;
    }
}