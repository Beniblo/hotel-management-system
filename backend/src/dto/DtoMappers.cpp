#include "hotel/dto/DtoMappers.hpp"

#include <chrono>

#include "hotel/core/Exceptions.hpp"
#include "hotel/dto/JsonHelpers.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::dto {

// --- Salud del sistema ---

Json::Value buildHealthJson(bool dbConnected, const std::string& dbName) {
    Json::Value root(Json::objectValue);
    root["status"] = dbConnected ? "ok" : "degraded";
    root["timestamp"] = hotel::validators::formatUtcTimestamp(std::chrono::system_clock::now());

    Json::Value dbObj(Json::objectValue);
    dbObj["status"] = dbConnected ? "connected" : "disconnected";
    dbObj["name"] = dbName;
    root["database"] = std::move(dbObj);
    return root;
}

// --- Huespedes ---

hotel::services::CreateHuespedInput parseCreateHuespedDto(const Json::Value& body) {
    requireJsonObject(body);

    hotel::services::CreateHuespedInput dto;
    dto.nombre = requireStringField(body, "nombre");
    dto.correo = requireStringField(body, "correo");
    dto.telefono = requireStringField(body, "telefono");
    if (const auto prefs = optionalStringArrayField(body, "preferencias"); prefs.has_value()) {
        dto.preferencias = *prefs;
    }
    return dto;
}

hotel::services::UpdateHuespedInput parseUpdateHuespedDto(const Json::Value& body) {
    requireJsonObject(body);

    hotel::services::UpdateHuespedInput dto;
    dto.nombre = optionalStringField(body, "nombre");
    dto.correo = optionalStringField(body, "correo");
    dto.telefono = optionalStringField(body, "telefono");
    dto.preferencias = optionalStringArrayField(body, "preferencias");
    dto.activo = optionalBoolField(body, "activo");
    return dto;
}

Json::Value toJson(const hotel::models::Huesped& huesped) {
    Json::Value obj(Json::objectValue);
    obj["id"] = huesped.id;
    obj["nombre"] = huesped.nombre;
    obj["correo"] = huesped.correo;
    obj["telefono"] = huesped.telefono;
    obj["fechaRegistro"] = hotel::validators::formatUtcTimestamp(huesped.fechaRegistro);
    obj["preferencias"] = toJsonStringArray(huesped.preferencias);
    obj["activo"] = huesped.activo;
    obj["actualizadoEn"] = hotel::validators::formatUtcTimestamp(huesped.actualizadoEn);
    return obj;
}

Json::Value toJson(const hotel::repositories::PagedResult<hotel::models::Huesped>& page) {
    Json::Value root(Json::objectValue);
    Json::Value dataArr(Json::arrayValue);
    for (const auto& item : page.items) {
        dataArr.append(toJson(item));
    }
    root["data"] = std::move(dataArr);
    root["meta"] = buildPaginationMeta(page.total, page.page, page.limit);
    return root;
}

// --- Habitaciones ---

hotel::services::CreateHabitacionInput parseCreateHabitacionDto(const Json::Value& body) {
    requireJsonObject(body);

    hotel::services::CreateHabitacionInput dto;
    dto.numero = requireStringField(body, "numero");
    dto.tipo = requireStringField(body, "tipo");
    dto.capacidad = requireInt32Field(body, "capacidad");
    dto.tarifa = requireDoubleField(body, "tarifa");

    if (const auto estadoStr = optionalStringField(body, "estado"); estadoStr.has_value()) {
        const auto parsed = hotel::models::parseEstadoHabitacion(*estadoStr);
        if (!parsed.has_value()) {
            throw hotel::core::ValidationException(
                "Estado de habitacion invalido.",
                {{"estado", "Valores permitidos: disponible, ocupada, mantenimiento, fuera_de_servicio."}});
        }
        dto.estado = *parsed;
    }

    if (const auto servs = optionalStringArrayField(body, "servicios"); servs.has_value()) {
        dto.servicios = *servs;
    }
    return dto;
}

hotel::services::UpdateHabitacionInput parseUpdateHabitacionDto(const Json::Value& body) {
    requireJsonObject(body);

    hotel::services::UpdateHabitacionInput dto;
    dto.numero = optionalStringField(body, "numero");
    dto.tipo = optionalStringField(body, "tipo");
    dto.capacidad = optionalInt32Field(body, "capacidad");
    dto.tarifa = optionalDoubleField(body, "tarifa");

    if (const auto estadoStr = optionalStringField(body, "estado"); estadoStr.has_value()) {
        const auto parsed = hotel::models::parseEstadoHabitacion(*estadoStr);
        if (!parsed.has_value()) {
            throw hotel::core::ValidationException(
                "Estado de habitacion invalido.",
                {{"estado", "Valores permitidos: disponible, ocupada, mantenimiento, fuera_de_servicio."}});
        }
        dto.estado = *parsed;
    }

    dto.servicios = optionalStringArrayField(body, "servicios");
    return dto;
}

hotel::models::EstadoHabitacion parseUpdateEstadoHabitacionDto(const Json::Value& body) {
    requireJsonObject(body);

    const std::string estadoStr = requireStringField(body, "estado");
    const auto parsed = hotel::models::parseEstadoHabitacion(estadoStr);
    if (!parsed.has_value()) {
        throw hotel::core::ValidationException(
            "Estado de habitacion invalido.",
            {{"estado", "Valores permitidos: disponible, ocupada, mantenimiento, fuera_de_servicio."}});
    }
    return *parsed;
}

Json::Value toJson(const hotel::models::Habitacion& habitacion) {
    Json::Value obj(Json::objectValue);
    obj["id"] = habitacion.id;
    obj["numero"] = habitacion.numero;
    obj["tipo"] = habitacion.tipo;
    obj["capacidad"] = habitacion.capacidad;
    obj["tarifa"] = hotel::validators::roundCurrency(habitacion.tarifa);
    obj["estado"] = hotel::models::toString(habitacion.estado);
    obj["servicios"] = toJsonStringArray(habitacion.servicios);
    obj["actualizadoEn"] = hotel::validators::formatUtcTimestamp(habitacion.actualizadoEn);
    return obj;
}

Json::Value toJson(const hotel::repositories::PagedResult<hotel::models::Habitacion>& page) {
    Json::Value root(Json::objectValue);
    Json::Value dataArr(Json::arrayValue);
    for (const auto& item : page.items) {
        dataArr.append(toJson(item));
    }
    root["data"] = std::move(dataArr);
    root["meta"] = buildPaginationMeta(page.total, page.page, page.limit);
    return root;
}

// --- Reservaciones ---

hotel::services::CreateReservacionInput parseCreateReservacionDto(const Json::Value& body) {
    requireJsonObject(body);

    hotel::services::CreateReservacionInput dto;
    dto.huespedId = requireStringField(body, "huespedId");
    dto.habitacionId = requireStringField(body, "habitacionId");
    dto.fechaEntrada = requireStringField(body, "fechaEntrada");
    dto.fechaSalida = requireStringField(body, "fechaSalida");
    dto.personas = requireInt32Field(body, "personas");

    if (const auto estadoStr = optionalStringField(body, "estado"); estadoStr.has_value()) {
        const auto parsed = hotel::models::parseEstadoReservacion(*estadoStr);
        if (!parsed.has_value()) {
            throw hotel::core::ValidationException(
                "Estado de reservacion invalido.",
                {{"estado", "Valores permitidos al crear: pendiente, confirmada."}});
        }
        dto.estado = *parsed;
    }

    return dto;
}

hotel::services::UpdateReservacionInput parseUpdateReservacionDto(const Json::Value& body) {
    requireJsonObject(body);

    hotel::services::UpdateReservacionInput dto;
    dto.habitacionId = optionalStringField(body, "habitacionId");
    dto.fechaEntrada = optionalStringField(body, "fechaEntrada");
    dto.fechaSalida = optionalStringField(body, "fechaSalida");
    dto.personas = optionalInt32Field(body, "personas");

    if (const auto estadoStr = optionalStringField(body, "estado"); estadoStr.has_value()) {
        const auto parsed = hotel::models::parseEstadoReservacion(*estadoStr);
        if (!parsed.has_value()) {
            throw hotel::core::ValidationException(
                "Estado de reservacion invalido.",
                {{"estado", "Valores permitidos: pendiente, confirmada, en_curso, completada, cancelada."}});
        }
        dto.estado = *parsed;
    }

    return dto;
}

Json::Value toJson(const hotel::models::Reservacion& reservacion) {
    Json::Value obj(Json::objectValue);
    obj["id"] = reservacion.id;
    obj["huespedId"] = reservacion.huespedId;
    obj["habitacionId"] = reservacion.habitacionId;
    obj["fechaEntrada"] = reservacion.fechaEntrada;
    obj["fechaSalida"] = reservacion.fechaSalida;
    obj["noches"] = reservacion.noches;
    obj["personas"] = reservacion.personas;
    obj["estado"] = hotel::models::toString(reservacion.estado);
    obj["tarifaAplicada"] = hotel::validators::roundCurrency(reservacion.tarifaAplicada);
    obj["total"] = hotel::validators::roundCurrency(reservacion.total);
    obj["creadoEn"] = hotel::validators::formatUtcTimestamp(reservacion.creadoEn);
    obj["actualizadoEn"] = hotel::validators::formatUtcTimestamp(reservacion.actualizadoEn);
    return obj;
}

Json::Value toJson(const hotel::repositories::PagedResult<hotel::models::Reservacion>& page) {
    Json::Value root(Json::objectValue);
    Json::Value dataArr(Json::arrayValue);
    for (const auto& item : page.items) {
        dataArr.append(toJson(item));
    }
    root["data"] = std::move(dataArr);
    root["meta"] = buildPaginationMeta(page.total, page.page, page.limit);
    return root;
}

// --- Disponibilidad y Reportes ---

Json::Value toJson(const hotel::services::DisponibilidadResult& disponibilidad) {
    Json::Value root(Json::objectValue);
    root["fechaEntrada"] = disponibilidad.fechaEntrada;
    root["fechaSalida"] = disponibilidad.fechaSalida;
    root["noches"] = disponibilidad.noches;
    root["totalDisponibles"] = static_cast<Json::Int64>(disponibilidad.habitaciones.size());

    Json::Value habitacionesArr(Json::arrayValue);
    for (const auto& item : disponibilidad.habitaciones) {
        Json::Value entry = toJson(item.habitacion);
        entry["noches"] = item.noches;
        entry["costoEstimado"] = hotel::validators::roundCurrency(item.costoEstimado);
        habitacionesArr.append(std::move(entry));
    }
    root["habitaciones"] = std::move(habitacionesArr);
    return root;
}

Json::Value toJson(const hotel::models::ResumenReporte& reporte) {
    Json::Value root(Json::objectValue);
    root["fechaInicio"] = reporte.fechaInicio;
    root["fechaFin"] = reporte.fechaFin;
    root["totalReservaciones"] = static_cast<Json::Int64>(reporte.totalReservaciones);

    Json::Value porEstado(Json::objectValue);
    for (const auto& [estado, cantidad] : reporte.reservacionesPorEstado) {
        porEstado[estado] = static_cast<Json::Int64>(cantidad);
    }
    root["reservacionesPorEstado"] = std::move(porEstado);

    root["totalHabitaciones"] = static_cast<Json::Int64>(reporte.totalHabitaciones);
    root["habitacionesOcupadas"] = static_cast<Json::Int64>(reporte.habitacionesOcupadas);
    root["habitacionesDisponibles"] = static_cast<Json::Int64>(reporte.habitacionesDisponibles);
    root["habitacionesNoOperativas"] = static_cast<Json::Int64>(reporte.habitacionesNoOperativas);
    root["porcentajeOcupacion"] = hotel::validators::roundCurrency(reporte.porcentajeOcupacion);
    root["ingresosEstimados"] = hotel::validators::roundCurrency(reporte.ingresosEstimados);
    return root;
}

}  // namespace hotel::dto