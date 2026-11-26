#include <chrono>
#include <exception>
#include <string>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/HuespedRepository.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::AppException;
    using hotel::core::ConflictException;
    using hotel::core::Logger;
    using hotel::database::MongoConnectionManager;
    using hotel::models::EstadoHabitacion;
    using hotel::models::Habitacion;
    using hotel::models::Huesped;
    using hotel::repositories::HabitacionFilter;
    using hotel::repositories::HabitacionRepository;
    using hotel::repositories::HuespedFilter;
    using hotel::repositories::HuespedRepository;

    std::string tempHuespedId;
    std::string tempHabitacionId;

    try {
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion CRUD de repositorios contra Atlas (Parte 4B)...");

        MongoConnectionManager dbManager(config);
        HuespedRepository huespedRepo(dbManager);
        HabitacionRepository habitacionRepo(dbManager);

        const auto now = std::chrono::system_clock::now();

        // 1. Verificar ciclo CRUD + indice unico E11000 en huespedes
        Huesped h;
        h.nombre = "Huesped Prueba 4B";
        h.correo = "prueba.4b@ejemplo.com";
        h.telefono = "4779990042";
        h.fechaRegistro = now;
        h.preferencias = {"vista jardin"};
        h.activo = true;
        h.actualizadoEn = now;

        const auto createdH = huespedRepo.create(h);
        tempHuespedId = createdH.id;
        Logger::info("[Huespedes] insert_one OK -> id=" + tempHuespedId);

        const auto foundH = huespedRepo.findById(tempHuespedId);
        if (!foundH.has_value() || foundH->correo != "prueba.4b@ejemplo.com") {
            throw std::runtime_error("findById en huespedes no devolvio el registro esperado.");
        }

        // Probar rechazo por indice unico ux_huespedes_correo (E11000 -> HTTP 409)
        bool duplicateEmailRejected = false;
        try {
            static_cast<void>(huespedRepo.create(h));
        } catch (const ConflictException& ex) {
            duplicateEmailRejected = (ex.httpStatus() == 409);
            Logger::info("[Huespedes] Duplicado E11000 capturado OK -> HTTP " +
                         std::to_string(ex.httpStatus()) + " [" + ex.errorCode() + "]");
        }
        if (!duplicateEmailRejected) {
            throw std::runtime_error("El indice unico de correo no rechazo el duplicado.");
        }

        Huesped updatedH = *foundH;
        updatedH.telefono = "4779990099";
        updatedH.actualizadoEn = std::chrono::system_clock::now();
        if (!huespedRepo.update(updatedH).has_value()) {
            throw std::runtime_error("update en huespedes fallo.");
        }

        const auto huespedesPage = huespedRepo.findAll(HuespedFilter{});
        Logger::info("[Huespedes] findAll OK -> total=" + std::to_string(huespedesPage.total));

        if (!huespedRepo.deleteById(tempHuespedId)) {
            throw std::runtime_error("deleteById en huespedes fallo.");
        }
        tempHuespedId.clear();
        Logger::info("[Huespedes] delete_one OK (limpieza verificada).");

        // 2. Verificar ciclo CRUD + indice unico E11000 en habitaciones
        Habitacion hab;
        hab.numero = "999-4B";
        hab.tipo = "doble";
        hab.capacidad = 2;
        hab.tarifa = 1550.50;
        hab.estado = EstadoHabitacion::Disponible;
        hab.servicios = {"wifi", "tv"};
        hab.actualizadoEn = now;

        const auto createdHab = habitacionRepo.create(hab);
        tempHabitacionId = createdHab.id;
        Logger::info("[Habitaciones] insert_one OK -> id=" + tempHabitacionId);

        bool duplicateRoomRejected = false;
        try {
            static_cast<void>(habitacionRepo.create(hab));
        } catch (const ConflictException& ex) {
            duplicateRoomRejected = (ex.httpStatus() == 409);
            Logger::info("[Habitaciones] Duplicado E11000 capturado OK -> HTTP " +
                         std::to_string(ex.httpStatus()) + " [" + ex.errorCode() + "]");
        }
        if (!duplicateRoomRejected) {
            throw std::runtime_error("El indice unico de numero de habitacion no rechazo el duplicado.");
        }

        if (!habitacionRepo.updateEstado(
                tempHabitacionId, EstadoHabitacion::Mantenimiento, std::chrono::system_clock::now())) {
            throw std::runtime_error("updateEstado en habitaciones fallo.");
        }

        const auto habitacionesPage = habitacionRepo.findAll(HabitacionFilter{});
        Logger::info("[Habitaciones] findAll OK -> total=" + std::to_string(habitacionesPage.total));

        if (!habitacionRepo.deleteById(tempHabitacionId)) {
            throw std::runtime_error("deleteById en habitaciones fallo.");
        }
        tempHabitacionId.clear();
        Logger::info("[Habitaciones] delete_one OK (limpieza verificada).");

        Logger::info("[OK] Parte 4B verificada correctamente (HuespedRepository y HabitacionRepository operativos).");
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Error controlado -> HTTP " + std::to_string(ex.httpStatus()) +
                      " [" + ex.errorCode() + "]: " + ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico en Parte 4B: ") + ex.what());
        return 1;
    }
}