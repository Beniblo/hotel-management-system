#pragma once

#include <string>

#include <json/json.h>

#include "hotel/models/Enums.hpp"
#include "hotel/models/Habitacion.hpp"
#include "hotel/models/Huesped.hpp"
#include "hotel/models/Reservacion.hpp"
#include "hotel/models/ResumenReporte.hpp"
#include "hotel/repositories/RepositoryCommon.hpp"
#include "hotel/services/DisponibilidadService.hpp"
#include "hotel/services/HabitacionService.hpp"
#include "hotel/services/HuespedService.hpp"
#include "hotel/services/ReservacionService.hpp"

namespace hotel::dto {

// --- Salud del sistema (/api/v1/health) ---
[[nodiscard]] Json::Value buildHealthJson(bool dbConnected, const std::string& dbName);

// --- Huespedes ---
[[nodiscard]] hotel::services::CreateHuespedInput parseCreateHuespedDto(const Json::Value& body);
[[nodiscard]] hotel::services::UpdateHuespedInput parseUpdateHuespedDto(const Json::Value& body);
[[nodiscard]] Json::Value toJson(const hotel::models::Huesped& huesped);
[[nodiscard]] Json::Value toJson(
    const hotel::repositories::PagedResult<hotel::models::Huesped>& page);

// --- Habitaciones ---
[[nodiscard]] hotel::services::CreateHabitacionInput parseCreateHabitacionDto(
    const Json::Value& body);
[[nodiscard]] hotel::services::UpdateHabitacionInput parseUpdateHabitacionDto(
    const Json::Value& body);
[[nodiscard]] hotel::models::EstadoHabitacion parseUpdateEstadoHabitacionDto(
    const Json::Value& body);
[[nodiscard]] Json::Value toJson(const hotel::models::Habitacion& habitacion);
[[nodiscard]] Json::Value toJson(
    const hotel::repositories::PagedResult<hotel::models::Habitacion>& page);

// --- Reservaciones ---
[[nodiscard]] hotel::services::CreateReservacionInput parseCreateReservacionDto(
    const Json::Value& body);
[[nodiscard]] hotel::services::UpdateReservacionInput parseUpdateReservacionDto(
    const Json::Value& body);
[[nodiscard]] Json::Value toJson(const hotel::models::Reservacion& reservacion);
[[nodiscard]] Json::Value toJson(
    const hotel::repositories::PagedResult<hotel::models::Reservacion>& page);

// --- Disponibilidad y Reportes ---
[[nodiscard]] Json::Value toJson(const hotel::services::DisponibilidadResult& disponibilidad);
[[nodiscard]] Json::Value toJson(const hotel::models::ResumenReporte& reporte);

}  // namespace hotel::dto