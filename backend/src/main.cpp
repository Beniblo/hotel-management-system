#include <exception>
#include <string>

#include <drogon/drogon.h>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/database/MongoConnectionManager.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::AppException;
    using hotel::core::Logger;
    using hotel::database::MongoConnectionManager;

    try {
        // 1. Cargar configuracion segura desde variables de entorno / backend/.env
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion de conexion real a MongoDB Atlas (Parte 3B)...");
        Logger::info("Drogon version: " + std::string(drogon::getVersion()));

        // 2. Conexion real a Atlas con mongocxx::pool + ping + consulta inicial en HotelDB
        MongoConnectionManager dbManager(config);
        const auto check = dbManager.verifyConnection();
        if (!check.pingOk) {
            Logger::error("El ping inicial contra MongoDB Atlas fallo.");
            return 1;
        }

        // 3. Prueba de error controlado simulando servidor inaccesible (timeout corto)
        AppConfig invalidConfig = config;
        invalidConfig.mongodbUri = "mongodb://127.0.0.1:27999/";
        invalidConfig.mongodbConnectTimeoutMs = 1000;
        invalidConfig.mongodbServerSelectionTimeoutMs = 1000;

        try {
            MongoConnectionManager failingManager(invalidConfig);
            failingManager.ping();
            Logger::warn("La prueba de conexion invalida inesperadamente tuvo exito.");
        } catch (const AppException& ex) {
            Logger::info("Prueba de error controlado exitosa -> HTTP " +
                         std::to_string(ex.httpStatus()) + " [" + ex.errorCode() + "]: " +
                         ex.publicMessage());
        }

        Logger::info("[OK] Parte 3B verificada correctamente (conexion real a Atlas y fallo controlado).");
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Fallo de aplicacion -> HTTP " + std::to_string(ex.httpStatus()) +
                      " [" + ex.errorCode() + "]: " + ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico al iniciar: ") + ex.what());
        return 1;
    }
}