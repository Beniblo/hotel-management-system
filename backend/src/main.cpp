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
#include "hotel/services/HabitacionService.hpp"
#include "hotel/services/HuespedService.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::AppException;
    using hotel::core::ConflictException;
    using hotel::core::Logger;
    using hotel::database::MongoConnectionManager;
    using hotel::models::EstadoHabitacion;
    using hotel::repositories::HabitacionRepository;
    using hotel::repositories::HuespedRepository;
    using hotel::repositories::ReservacionRepository;
    using hotel::services::CreateHabitacionInput;
    using hotel::services::CreateHuespedInput;
    using hotel::services::HabitacionService;
    using hotel::services::HuespedService;
    using hotel::services::UpdateHabitacionInput;
    using hotel::services::UpdateHuespedInput;

    try {
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion de HuespedService y HabitacionService (Parte 4D)...");

        MongoConnectionManager dbManager(config);
        HuespedRepository huespedRepo(dbManager);
        HabitacionRepository habitacionRepo(dbManager);
        ReservacionRepository reservacionRepo(dbManager);

        HuespedService huespedService(huespedRepo, reservacionRepo);
        HabitacionService habitacionService(habitacionRepo, reservacionRepo);

        // 1. Verificar reglas de HuespedService
        CreateHuespedInput nuevoHuesped;
        nuevoHuesped.nombre = "  Luis Prueba 4D  ";
        nuevoHuesped.correo = "  LUIS.4D@Ejemplo.COM ";
        nuevoHuesped.telefono = "477-555-0140";
        nuevoHuesped.preferencias = {"piso bajo", "piso bajo", "vista ciudad"};

        const auto creadoH = huespedService.crear(nuevoHuesped);
        if (creadoH.correo != "luis.4d@ejemplo.com" || creadoH.preferencias.size() != 2) {
            throw std::runtime_error("La normalizacion de huesped no funciono como se esperaba.");
        }
        Logger::info("[HuespedService] crear OK -> id=" + creadoH.id + ", correo=" + creadoH.correo);

        // Probar rechazo de correo duplicado a nivel de servicio
        bool dupEmailCaught = false;
        try {
            static_cast<void>(huespedService.crear(nuevoHuesped));
        } catch (const ConflictException& ex) {
            dupEmailCaught = (ex.httpStatus() == 409);
        }
        if (!dupEmailCaught) {
            throw std::runtime_error("HuespedService no rechazo correo duplicado con 409.");
        }

        // Actualizacion parcial
        UpdateHuespedInput patchH;
        patchH.telefono = "477-555-0199";
        const auto actualizadoH = huespedService.actualizar(creadoH.id, patchH);
        if (actualizadoH.telefono != "477-555-0199") {
            throw std::runtime_error("actualizar en HuespedService fallo.");
        }

        // Baja logica (activo = false)
        const auto bajaLogica = huespedService.eliminar(creadoH.id, false);
        if (bajaLogica.eliminacionFisica || !bajaLogica.huespedDesactivado.has_value() ||
            bajaLogica.huespedDesactivado->activo) {
            throw std::runtime_error("La baja logica de HuespedService no desactivo el registro.");
        }
        Logger::info("[HuespedService] baja logica OK -> activo=false");

        // Eliminacion fisica controlada (sin reservaciones asociadas)
        const auto bajaFisica = huespedService.eliminar(creadoH.id, true);
        if (!bajaFisica.eliminacionFisica) {
            throw std::runtime_error("La eliminacion fisica de HuespedService fallo.");
        }
        Logger::info("[HuespedService] eliminacion fisica controlada OK.");

        // Verificar proteccion de integridad sobre Ana Lopez (tiene reservacion semilla activa)
        const auto ana = huespedRepo.findByEmail("ana.lopez@ejemplo.com");
        if (!ana.has_value()) {
            throw std::runtime_error("No se encontro huesped semilla ana.lopez@ejemplo.com.");
        }
        bool anaProtected = false;
        try {
            static_cast<void>(huespedService.eliminar(ana->id, false));
        } catch (const ConflictException& ex) {
            anaProtected = (ex.httpStatus() == 409);
            Logger::info("[HuespedService] Proteccion de huesped con reserva activa OK -> HTTP 409 [" +
                         ex.errorCode() + "]");
        }
        if (!anaProtected) {
            throw std::runtime_error("Se permitio desactivar un huesped con reservacion activa.");
        }

        // 2. Verificar reglas de HabitacionService
        CreateHabitacionInput nuevaHab;
        nuevaHab.numero = " 404-d ";
        nuevaHab.tipo = "Suite";
        nuevaHab.capacidad = 3;
        nuevaHab.tarifa = 2199.996;
        nuevaHab.servicios = {"wifi", "jacuzzi", "WIFI"};

        const auto creadaHab = habitacionService.crear(nuevaHab);
        if (creadaHab.numero != "404-D" || creadaHab.tipo != "suite" ||
            creadaHab.tarifa != 2200.0 || creadaHab.servicios.size() != 2) {
            throw std::runtime_error("La normalizacion de habitacion no funciono como se esperaba.");
        }
        Logger::info("[HabitacionService] crear OK -> numero=" + creadaHab.numero +
                     ", tarifa=" + std::to_string(creadaHab.tarifa));

        // Cambio rapido de estado
        const auto enMant =
            habitacionService.cambiarEstado(creadaHab.id, EstadoHabitacion::Mantenimiento);
        if (enMant.estado != EstadoHabitacion::Mantenimiento) {
            throw std::runtime_error("cambiarEstado en HabitacionService fallo.");
        }

        // Actualizacion parcial
        UpdateHabitacionInput patchHab;
        patchHab.tarifa = 2350.0;
        patchHab.estado = EstadoHabitacion::Disponible;
        const auto actualizadaHab = habitacionService.actualizar(creadaHab.id, patchHab);
        if (actualizadaHab.tarifa != 2350.0 || actualizadaHab.estado != EstadoHabitacion::Disponible) {
            throw std::runtime_error("actualizar en HabitacionService fallo.");
        }

        // Eliminacion fisica controlada (sin reservaciones)
        habitacionService.eliminar(creadaHab.id);
        Logger::info("[HabitacionService] cambiarEstado, actualizar y eliminar OK.");

        // Verificar proteccion sobre habitacion 102 (tiene reservacion confirmada)
        const auto hab102 = habitacionRepo.findByNumero("102");
        if (!hab102.has_value()) {
            throw std::runtime_error("No se encontro habitacion semilla 102.");
        }
        bool hab102StateProtected = false;
        try {
            static_cast<void>(
                habitacionService.cambiarEstado(hab102->id, EstadoHabitacion::Mantenimiento));
        } catch (const ConflictException& ex) {
            hab102StateProtected = (ex.httpStatus() == 409);
            Logger::info("[HabitacionService] Proteccion de estado en hab 102 OK -> HTTP 409 [" +
                         ex.errorCode() + "]");
        }
        if (!hab102StateProtected) {
            throw std::runtime_error("Se permitio poner en mantenimiento la hab 102 con reserva activa.");
        }

        Logger::info("[OK] Parte 4D verificada correctamente (HuespedService y HabitacionService operativos).");
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Error controlado -> HTTP " + std::to_string(ex.httpStatus()) +
                      " [" + ex.errorCode() + "]: " + ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico en Parte 4D: ") + ex.what());
        return 1;
    }
}