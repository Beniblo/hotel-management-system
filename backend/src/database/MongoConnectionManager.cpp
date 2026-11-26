#include "hotel/database/MongoConnectionManager.hpp"

#include <exception>
#include <string>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"

namespace hotel::database {

namespace {

// Inicializacion unica y segura para hilos de mongocxx::instance por proceso.
void ensureMongoInstance() {
    static mongocxx::instance instance{};
    static_cast<void>(instance);
}

void appendQueryParam(std::string& uri, const std::string& key, const std::string& value) {
    if (uri.find(key + "=") != std::string::npos) {
        return;
    }

    if (uri.find('?') == std::string::npos) {
        const auto schemePos = uri.find("://");
        const auto slashAfterHost = (schemePos != std::string::npos)
                                        ? uri.find('/', schemePos + 3)
                                        : std::string::npos;
        if (slashAfterHost == std::string::npos) {
            uri.push_back('/');
        }
        uri.push_back('?');
    } else if (uri.back() != '?' && uri.back() != '&') {
        uri.push_back('&');
    }

    uri += key + "=" + value;
}

[[noreturn]] void translateMongoException(const mongocxx::exception& ex) {
    const std::string details = hotel::core::Logger::sanitize(ex.what());
    hotel::core::Logger::error("Error de controlador mongocxx: " + details);

    // Codigo 13 (Unauthorized) o fallo de autenticacion en Atlas
    if (ex.code().value() == 13 ||
        details.find("not authorized") != std::string::npos ||
        details.find("Authentication failed") != std::string::npos) {
        throw hotel::core::DatabaseAuthorizationException(
            "Acceso denegado por MongoDB Atlas: verifica usuario, contraseña o permisos sobre HotelDB.");
    }

    throw hotel::core::DatabaseUnavailableException(
        "No se pudo establecer comunicacion con MongoDB Atlas.");
}

}  // namespace

std::string MongoConnectionManager::buildEffectiveUri(const hotel::config::AppConfig& config) {
    std::string uri = config.mongodbUri;
    appendQueryParam(uri, "maxPoolSize", std::to_string(config.mongodbMaxPoolSize));
    appendQueryParam(uri, "connectTimeoutMS", std::to_string(config.mongodbConnectTimeoutMs));
    appendQueryParam(
        uri, "serverSelectionTimeoutMS", std::to_string(config.mongodbServerSelectionTimeoutMs));
    return uri;
}

MongoConnectionManager::MongoConnectionManager(const hotel::config::AppConfig& config)
    : databaseName_(config.mongodbDbName) {
    ensureMongoInstance();

    try {
        const std::string effectiveUri = buildEffectiveUri(config);
        mongocxx::uri uri{effectiveUri};
        pool_ = std::make_unique<mongocxx::pool>(uri);

        hotel::core::Logger::info("Pool mongocxx inicializado para base '" + databaseName_ +
                                  "' (URI: " + hotel::config::AppConfig::maskUri(config.mongodbUri) +
                                  ", maxPoolSize=" + std::to_string(config.mongodbMaxPoolSize) + ").");
    } catch (const mongocxx::exception& ex) {
        translateMongoException(ex);
    } catch (const std::exception& ex) {
        hotel::core::Logger::error("Fallo al inicializar mongocxx::pool: " +
                                   hotel::core::Logger::sanitize(ex.what()));
        throw hotel::core::DatabaseUnavailableException(
            "Configuracion o conexion inicial con MongoDB Atlas invalida.");
    }
}

mongocxx::pool::entry MongoConnectionManager::acquireClient() {
    if (!pool_) {
        throw hotel::core::DatabaseUnavailableException(
            "El pool de conexiones a MongoDB no esta inicializado.");
    }
    try {
        return pool_->acquire();
    } catch (const mongocxx::exception& ex) {
        translateMongoException(ex);
    } catch (const std::exception& ex) {
        hotel::core::Logger::error("Error al adquirir cliente del pool: " +
                                   hotel::core::Logger::sanitize(ex.what()));
        throw hotel::core::DatabaseUnavailableException(
            "No hay conexiones disponibles hacia MongoDB Atlas.");
    }
}

mongocxx::database MongoConnectionManager::database(mongocxx::client& client) const {
    return client[databaseName_];
}

bool MongoConnectionManager::ping() {
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    try {
        auto client = acquireClient();
        auto db = database(*client);
        const auto pingCmd = make_document(kvp("ping", 1));
        const auto result = db.run_command(pingCmd.view());
        static_cast<void>(result);

        hotel::core::Logger::info("Ping a MongoDB Atlas exitoso en base '" + databaseName_ + "'.");
        return true;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        translateMongoException(ex);
    } catch (const std::exception& ex) {
        hotel::core::Logger::error("Error inesperado durante ping a Atlas: " +
                                   hotel::core::Logger::sanitize(ex.what()));
        throw hotel::core::DatabaseUnavailableException(
            "Fallo de comunicacion al verificar salud de MongoDB Atlas.");
    }
}

ConnectionCheckResult MongoConnectionManager::verifyConnection() {
    using bsoncxx::builder::basic::make_document;

    ConnectionCheckResult summary;
    summary.pingOk = ping();

    try {
        auto client = acquireClient();
        auto db = database(*client);
        const auto emptyFilter = make_document();

        summary.huespedesCount = db["huespedes"].count_documents(emptyFilter.view());
        summary.habitacionesCount = db["habitaciones"].count_documents(emptyFilter.view());
        summary.reservacionesCount = db["reservaciones"].count_documents(emptyFilter.view());

        hotel::core::Logger::info(
            "Consulta inicial en '" + databaseName_ + "' exitosa -> huespedes: " +
            std::to_string(summary.huespedesCount) + ", habitaciones: " +
            std::to_string(summary.habitacionesCount) + ", reservaciones: " +
            std::to_string(summary.reservacionesCount) + ".");
        return summary;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        translateMongoException(ex);
    } catch (const std::exception& ex) {
        hotel::core::Logger::error("Error inesperado en verificacion inicial de HotelDB: " +
                                   hotel::core::Logger::sanitize(ex.what()));
        throw hotel::core::DatabaseUnavailableException(
            "No fue posible consultar las colecciones iniciales en HotelDB.");
    }
}

const std::string& MongoConnectionManager::databaseName() const noexcept {
    return databaseName_;
}

}  // namespace hotel::database