#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/pool.hpp>

#include "hotel/config/AppConfig.hpp"

namespace hotel::database {

struct ConnectionCheckResult {
    bool pingOk{false};
    std::int64_t huespedesCount{0};
    std::int64_t habitacionesCount{0};
    std::int64_t reservacionesCount{0};
};

class MongoConnectionManager {
public:
    explicit MongoConnectionManager(const hotel::config::AppConfig& config);
    ~MongoConnectionManager() = default;

    MongoConnectionManager(const MongoConnectionManager&) = delete;
    MongoConnectionManager& operator=(const MongoConnectionManager&) = delete;
    MongoConnectionManager(MongoConnectionManager&&) = delete;
    MongoConnectionManager& operator=(MongoConnectionManager&&) = delete;

    // Adquiere un cliente seguro para hilos desde el pool.
    [[nodiscard]] mongocxx::pool::entry acquireClient();

    // Devuelve una referencia a la base de datos configurada (HotelDB).
    [[nodiscard]] mongocxx::database database(mongocxx::client& client) const;

    // Ejecuta el comando ping contra MongoDB Atlas.
    bool ping();

    // Ejecuta ping y una lectura inicial de conteo sobre las 3 colecciones de HotelDB.
    [[nodiscard]] ConnectionCheckResult verifyConnection();

    [[nodiscard]] const std::string& databaseName() const noexcept;

private:
    static std::string buildEffectiveUri(const hotel::config::AppConfig& config);

    std::string databaseName_;
    std::unique_ptr<mongocxx::pool> pool_;
};

}  // namespace hotel::database