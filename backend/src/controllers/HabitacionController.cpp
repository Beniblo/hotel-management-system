#include "hotel/controllers/HabitacionController.hpp"

#include <optional>
#include <string>

#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/dto/DtoMappers.hpp"
#include "hotel/dto/JsonHelpers.hpp"
#include "hotel/models/Enums.hpp"

namespace hotel::controllers {

namespace {

std::optional<std::string> getQueryParam(const drogon::HttpRequestPtr& req,
                                         const std::string& key) {
    const auto& params = req->getParameters();
    const auto it = params.find(key);
    if (it == params.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second;
}

const Json::Value& requireJsonBody(const drogon::HttpRequestPtr& req) {
    const auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        throw hotel::core::BadRequestException(
            "El cuerpo debe ser JSON valido con cabecera Content-Type: application/json.",
            "INVALID_JSON_BODY");
    }
    return *jsonPtr;
}

}  // namespace

HabitacionController::HabitacionController(hotel::services::HabitacionService& habitacionService)
    : habitacionService_(habitacionService) {}

void HabitacionController::list(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        hotel::repositories::HabitacionFilter filter;
        filter.tipo = getQueryParam(req, "tipo");

        if (const auto estadoRaw = getQueryParam(req, "estado"); estadoRaw.has_value()) {
            const auto parsed = hotel::models::parseEstadoHabitacion(*estadoRaw);
            if (!parsed.has_value()) {
                throw hotel::core::BadRequestException(
                    "El parametro 'estado' debe ser: disponible, ocupada, mantenimiento o fuera_de_servicio.",
                    "INVALID_QUERY_PARAM");
            }
            filter.estado = *parsed;
        }
        if (const auto capRaw = getQueryParam(req, "capacidadMin"); capRaw.has_value()) {
            filter.capacidadMin = hotel::dto::parseOptionalInt32Query(*capRaw, "capacidadMin");
        }
        if (const auto tarifaRaw = getQueryParam(req, "tarifaMax"); tarifaRaw.has_value()) {
            filter.tarifaMax = hotel::dto::parseOptionalDoubleQuery(*tarifaRaw, "tarifaMax");
        }
        if (const auto pageRaw = getQueryParam(req, "page"); pageRaw.has_value()) {
            if (const auto parsed = hotel::dto::parseOptionalInt32Query(*pageRaw, "page");
                parsed.has_value()) {
                filter.page = *parsed;
            }
        }
        if (const auto limitRaw = getQueryParam(req, "limit"); limitRaw.has_value()) {
            if (const auto parsed = hotel::dto::parseOptionalInt32Query(*limitRaw, "limit");
                parsed.has_value()) {
                filter.limit = *parsed;
            }
        }

        const auto pageResult = habitacionService_.listar(filter);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(pageResult));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HabitacionController::create(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto input = hotel::dto::parseCreateHabitacionDto(jsonBody);
        const auto created = habitacionService_.crear(input);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(created));
        response->setStatusCode(drogon::k201Created);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HabitacionController::getById(
    const drogon::HttpRequestPtr& /*req*/,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto found = habitacionService_.obtenerPorId(id);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(found));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HabitacionController::update(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto input = hotel::dto::parseUpdateHabitacionDto(jsonBody);
        const auto updated = habitacionService_.actualizar(id, input);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(updated));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HabitacionController::updateEstado(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto nuevoEstado = hotel::dto::parseUpdateEstadoHabitacionDto(jsonBody);
        const auto updated = habitacionService_.cambiarEstado(id, nuevoEstado);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(updated));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HabitacionController::remove(
    const drogon::HttpRequestPtr& /*req*/,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        habitacionService_.eliminar(id);
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k204NoContent);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

}  // namespace hotel::controllers