#include "hotel/core/AppContext.hpp"

#include <utility>

namespace hotel::core {

AppContext::AppContext(hotel::config::AppConfig config)
    : config_(std::move(config)),
      dbManager_(config_),
      huespedRepo_(dbManager_),
      habitacionRepo_(dbManager_),
      reservacionRepo_(dbManager_),
      roomLockManager_(),
      huespedService_(huespedRepo_, reservacionRepo_),
      habitacionService_(habitacionRepo_, reservacionRepo_),
      reservacionService_(reservacionRepo_, huespedRepo_, habitacionRepo_, roomLockManager_),
      disponibilidadService_(habitacionRepo_, reservacionRepo_),
      reporteService_(habitacionRepo_, reservacionRepo_) {}

const hotel::config::AppConfig& AppContext::config() const noexcept {
    return config_;
}

hotel::database::MongoConnectionManager& AppContext::dbManager() noexcept {
    return dbManager_;
}

hotel::services::HuespedService& AppContext::huespedService() noexcept {
    return huespedService_;
}

hotel::services::HabitacionService& AppContext::habitacionService() noexcept {
    return habitacionService_;
}

hotel::services::ReservacionService& AppContext::reservacionService() noexcept {
    return reservacionService_;
}

hotel::services::DisponibilidadService& AppContext::disponibilidadService() noexcept {
    return disponibilidadService_;
}

hotel::services::ReporteService& AppContext::reporteService() noexcept {
    return reporteService_;
}

}  // namespace hotel::core