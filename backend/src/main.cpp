#include <exception>
#include <memory>
#include <string>

#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>

#include "hotel/config/AppConfig.hpp"
#include "hotel/controllers/HabitacionController.hpp"
#include "hotel/controllers/HealthController.hpp"
#include "hotel/controllers/HuespedController.hpp"
#include "hotel/controllers/ReservacionController.hpp"
#include "hotel/core/AppContext.hpp"
#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"

namespace {

trantor::Logger::LogLevel toTrantorLogLevel(const std::string& level) {
    if (level == "TRACE") return trantor::Logger::kTrace;
    if (level == "DEBUG") return trantor::Logger::kDebug;
    if (level == "WARN")  return trantor::Logger::kWarn;
    if (level == "ERROR") return trantor::Logger::kError;
    return trantor::Logger::kInfo;
}

}  // namespace

int main() {
    using hotel::config::AppConfig;
    using hotel::controllers::HabitacionController;
    using hotel::controllers::HealthController;
    using hotel::controllers::HuespedController;
    using hotel::controllers::ReservacionController;
    using hotel::core::AppContext;
    using hotel::core::AppException;
    using hotel::core::ErrorMapper;
    using hotel::core::Logger;
    using hotel::core::NotFoundException;

    try {
        // 1. Cargar y validar configuracion segura (fail-fast)
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando API REST Hotel Management System (Drogon " +
                     std::string(drogon::getVersion()) + ")...");

        // 2. Construir contenedor de dependencias (pool mongocxx, repositorios y servicios)
        AppContext context(config);

        // 3. Verificacion inicial de conectividad con MongoDB Atlas (sin exponer secretos)
        try {
            if (context.dbManager().ping()) {
                Logger::info("[OK] Conexion inicial con MongoDB Atlas verificada (base '" +
                             config.mongodbDbName + "').");
            }
        } catch (const AppException& ex) {
            Logger::warn("Atlas no respondio en el ping inicial [" + ex.errorCode() +
                         "]: el servidor iniciara y /api/v1/health reportara HTTP 503.");
        }

        // 4. Registrar controladores con inyeccion de dependencias
        auto& app = drogon::app();
        app.setLogLevel(toTrantorLogLevel(config.logLevel));
        app.setThreadNum(4);
        app.setClientMaxBodySize(1024 * 1024);  // Limite de 1 MB por solicitud (OWASP API4)
        app.addListener(config.serverHost, config.serverPort);

        auto notFoundResp = ErrorMapper::toHttpResponse(
            NotFoundException("La ruta solicitada no existe en la API.", "ROUTE_NOT_FOUND"),
            "route-not-found");
        app.setCustom404Page(notFoundResp);

        app.registerController(std::make_shared<HealthController>(context.dbManager()));
        app.registerController(std::make_shared<HuespedController>(context.huespedService()));
        app.registerController(std::make_shared<HabitacionController>(context.habitacionService()));
        app.registerController(
            std::make_shared<ReservacionController>(context.reservacionService()));

        Logger::info("Servidor HTTP Drogon escuchando en http://" + config.serverHost + ":" +
                     std::to_string(config.serverPort) + "/api/v1");
        Logger::info("Presiona Ctrl + C para detener el servidor.");

        // 5. Iniciar bucle principal HTTP (bloqueante hasta senal de cierre)
        app.run();
        return 0;
    } catch (const AppException& ex) {
        Logger::error("Error de configuracion/inicio -> HTTP " +
                      std::to_string(ex.httpStatus()) + " [" + ex.errorCode() + "]: " +
                      ex.publicMessage());
        return 1;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico al iniciar el servidor: ") + ex.what());
        return 1;
    }
}