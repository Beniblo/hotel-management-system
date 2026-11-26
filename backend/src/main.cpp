#include <atomic>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/HuespedRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"
#include "hotel/services/DisponibilidadService.hpp"
#include "hotel/services/ReporteService.hpp"
#include "hotel/services/ReservacionService.hpp"
#include "hotel/services/RoomLockManager.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::AppException;
    using hotel::core::ConflictException;
    using hotel::core::Logger;
    using hotel::database::MongoConnectionManager;
    using hotel::models::EstadoReservacion;
    using hotel::repositories::HabitacionRepository;
    using hotel::repositories::HuespedRepository;
    using hotel::repositories::ReservacionRepository;
    using hotel::services::CreateReservacionInput;
    using hotel::services::DisponibilidadQuery;
    using hotel::services::DisponibilidadService;
    using hotel::services::ReporteService;
    using hotel::services::ReservacionService;
    using hotel::services::RoomLockManager;
    using hotel::services::UpdateReservacionInput;

    try {
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion del nucleo hotelero completo (Parte 4E)...");

        MongoConnectionManager dbManager(config);
        HuespedRepository huespedRepo(dbManager);
        HabitacionRepository habitacionRepo(dbManager);
        ReservacionRepository reservacionRepo(dbManager);
        RoomLockManager lockManager;

        ReservacionService reservacionService(
            reservacionRepo, huespedRepo, habitacionRepo, lockManager);
        DisponibilidadService disponibilidadService(habitacionRepo, reservacionRepo);
        ReporteService reporteService(habitacionRepo, reservacionRepo);

        const auto carlos = huespedRepo.findByEmail("carlos.mendoza@ejemplo.com");
        const auto hab102 = habitacionRepo.findByNumero("102");
        const auto hab201 = habitacionRepo.findByNumero("201");
        if (!carlos.has_value() || !hab102.has_value() || !hab201.has_value()) {
            throw std::runtime_error(
                "No se encontraron los datos semilla esperados (carlos.mendoza, hab 102 y 201).");
        }

        // 1. Verificar DisponibilidadService (hab 102 ocupada 2026-12-05..08)
        DisponibilidadQuery dispQuery;
        dispQuery.fechaEntrada = "2026-12-06";
        dispQuery.fechaSalida = "2026-12-08";
        const auto disp = disponibilidadService.consultar(dispQuery);
        if (disp.noches != 2 || disp.habitaciones.size() != 2) {
            throw std::runtime_error(
                "DisponibilidadService no devolvio las 2 habitaciones esperadas (101 y 201).");
        }
        Logger::info("[Disponibilidad] 2026-12-06..08 -> noches=" + std::to_string(disp.noches) +
                     ", habitaciones disponibles=" + std::to_string(disp.habitaciones.size()));

        // 2. Verificar rechazo de traslape en hab 102
        CreateReservacionInput traslapeInput;
        traslapeInput.huespedId = carlos->id;
        traslapeInput.habitacionId = hab102->id;
        traslapeInput.fechaEntrada = "2026-12-06";
        traslapeInput.fechaSalida = "2026-12-09";
        traslapeInput.personas = 2;

        bool overlapBlocked = false;
        try {
            static_cast<void>(reservacionService.crear(traslapeInput));
        } catch (const ConflictException& ex) {
            overlapBlocked = (ex.httpStatus() == 409 && ex.errorCode() == "RESERVATION_OVERLAP");
            Logger::info("[ReservacionService] Traslape bloqueado OK -> HTTP 409 [" +
                         ex.errorCode() + "]");
        }
        if (!overlapBlocked) {
            throw std::runtime_error("ReservacionService no bloqueo el traslape en hab 102.");
        }

        // 3. Prueba de concurrencia real: 4 hilos compitiendo por hab 201 en 2027-02-10..12
        std::atomic<int> successCount{0};
        std::atomic<int> conflictCount{0};
        std::mutex idMutex;
        std::string winnerId;
        double winnerTotal = 0.0;

        auto worker = [&]() {
            CreateReservacionInput req;
            req.huespedId = carlos->id;
            req.habitacionId = hab201->id;
            req.fechaEntrada = "2027-02-10";
            req.fechaSalida = "2027-02-12";
            req.personas = 2;
            req.estado = EstadoReservacion::Confirmada;

            try {
                const auto created = reservacionService.crear(req);
                ++successCount;
                std::lock_guard<std::mutex> lock(idMutex);
                winnerId = created.id;
                winnerTotal = created.total;
            } catch (const ConflictException& ex) {
                if (ex.errorCode() == "RESERVATION_OVERLAP") {
                    ++conflictCount;
                }
            }
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back(worker);
        }
        for (auto& t : threads) {
            t.join();
        }

        if (successCount.load() != 1 || conflictCount.load() != 3 || winnerId.empty() ||
            winnerTotal != 5200.0) {
            throw std::runtime_error(
                "La prueba de concurrencia fallo (exitos=" + std::to_string(successCount.load()) +
                ", conflictos=" + std::to_string(conflictCount.load()) + ").");
        }
        Logger::info("[Concurrencia] 4 hilos simultaneos sobre hab 201 -> 1 exito (total=" +
                     std::to_string(winnerTotal) + ") y 3 conflictos HTTP 409.");

        // 4. Actualizar fechas (recalculo automatico), probar proteccion de delete, cancelar y limpiar
        UpdateReservacionInput patchReq;
        patchReq.fechaSalida = "2027-02-13";  // 3 noches * 2600 = 7800
        const auto updated = reservacionService.actualizar(winnerId, patchReq);
        if (updated.noches != 3 || updated.total != 7800.0) {
            throw std::runtime_error("actualizar en ReservacionService no recalculo el total.");
        }

        bool activeDeleteBlocked = false;
        try {
            reservacionService.eliminar(winnerId);
        } catch (const ConflictException& ex) {
            activeDeleteBlocked = (ex.httpStatus() == 409);
        }
        if (!activeDeleteBlocked) {
            throw std::runtime_error("Se permitio eliminar fisicamente una reservacion confirmada.");
        }

        const auto cancelada = reservacionService.cancelar(winnerId);
        if (cancelada.estado != EstadoReservacion::Cancelada) {
            throw std::runtime_error("cancelar en ReservacionService fallo.");
        }
        reservacionService.eliminar(winnerId);
        Logger::info("[ReservacionService] actualizar, proteccion de borrado, cancelar y eliminar OK.");

        // 5. Verificar ReporteService para diciembre 2026
        const auto reporte = reporteService.generarResumen("2026-12-01", "2026-12-31");
        if (reporte.totalHabitaciones < 3 || reporte.habitacionesOcupadas < 1 ||
            reporte.ingresosEstimados < 4350.0) {
            throw std::runtime_error("ReporteService devolvio metricas inesperadas para dic 2026.");
        }
        Logger::info("[ReporteService] dic 2026 -> reservas=" +
                     std::to_string(reporte.totalReservaciones) + ", ocupadas=" +
                     std::to_string(reporte.habitacionesOcupadas) + "/" +
                     std::to_string(reporte.totalHabitaciones) + " (" +
                     std::to_string(reporte.porcentajeOcupacion) + "%), ingresos=" +
                     std::to_string(reporte.ingresosEstimados));

        Logger::info("[OK] Parte 4E verificada correctamente (Fase 4 completa: nucleo hotelero C++ operativo).");
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Error controlado -> HTTP " + std::to_string(ex.httpStatus()) +
                      " [" + ex.errorCode() + "]: " + ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico en Parte 4E: ") + ex.what());
        return 1;
    }
}