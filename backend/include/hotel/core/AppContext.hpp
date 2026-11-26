#pragma once

#include "hotel/config/AppConfig.hpp"
#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/repositories/HabitacionRepository.hpp"
#include "hotel/repositories/HuespedRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"
#include "hotel/services/DisponibilidadService.hpp"
#include "hotel/services/HabitacionService.hpp"
#include "hotel/services/HuespedService.hpp"
#include "hotel/services/ReporteService.hpp"
#include "hotel/services/ReservacionService.hpp"
#include "hotel/services/RoomLockManager.hpp"

namespace hotel::core {

class AppContext {
public:
    explicit AppContext(hotel::config::AppConfig config);
    ~AppContext() = default;

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;
    AppContext(AppContext&&) = delete;
    AppContext& operator=(AppContext&&) = delete;

    [[nodiscard]] const hotel::config::AppConfig& config() const noexcept;
    [[nodiscard]] hotel::database::MongoConnectionManager& dbManager() noexcept;

    [[nodiscard]] hotel::services::HuespedService& huespedService() noexcept;
    [[nodiscard]] hotel::services::HabitacionService& habitacionService() noexcept;
    [[nodiscard]] hotel::services::ReservacionService& reservacionService() noexcept;
    [[nodiscard]] hotel::services::DisponibilidadService& disponibilidadService() noexcept;
    [[nodiscard]] hotel::services::ReporteService& reporteService() noexcept;

private:
    // Orden estricto de declaracion: las dependencias base se construyen primero y se destruyen al final.
    hotel::config::AppConfig config_;
    hotel::database::MongoConnectionManager dbManager_;

    hotel::repositories::HuespedRepository huespedRepo_;
    hotel::repositories::HabitacionRepository habitacionRepo_;
    hotel::repositories::ReservacionRepository reservacionRepo_;
    hotel::services::RoomLockManager roomLockManager_;

    hotel::services::HuespedService huespedService_;
    hotel::services::HabitacionService habitacionService_;
    hotel::services::ReservacionService reservacionService_;
    hotel::services::DisponibilidadService disponibilidadService_;
    hotel::services::ReporteService reporteService_;
};

}  // namespace hotel::core