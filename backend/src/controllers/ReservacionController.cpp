#include "hotel/controllers/ReservacionController.hpp"

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

ReservacionController::ReservacionController(
    hotel::services::ReservacionService& reservacionService)
    : reservacionService_(reservacionService) {}

void ReservacionController::list(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        hotel::repositories::ReservacionFilter filter;
        filter.huespedId = getQueryParam(req, "huespedId");
        filter.habitacionId = getQueryParam(req, "habitacionId");
        filter.fechaDesde = getQueryParam(req, "fechaDesde");
        filter.fechaHasta = getQueryParam(req, "fechaHasta");

        if (const auto estadoRaw = getQueryParam(req, "estado"); estadoRaw.has_value()) {
            const auto parsed = hotel::models::parseEstadoReservacion(*estadoRaw);
            if (!parsed.has_value()) {
                throw hotel::core::BadRequestException(
                    "El parametro 'estado' debe ser: pendiente, confirmada, en_curso, completada o cancelada.",
                    "INVALID_QUERY_PARAM");
            }
            filter.estado = *parsed;
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

        const auto pageResult = reservacionService_.listar(filter);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(pageResult));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void ReservacionController::create(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto input = hotel::dto::parseCreateReservacionDto(jsonBody);
        const auto created = reservacionService_.crear(input);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(created));
        response->setStatusCode(drogon::k201Created);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void ReservacionController::getById(
    const drogon::HttpRequestPtr& /*req*/,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto found = reservacionService_.obtenerPorId(id);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(found));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void ReservacionController::update(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto input = hotel::dto::parseUpdateReservacionDto(jsonBody);
        const auto updated = reservacionService_.actualizar(id, input);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(updated));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void ReservacionController::cancel(
    const drogon::HttpRequestPtr& /*req*/,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto cancelled = reservacionService_.cancelar(id);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(cancelled));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void ReservacionController::remove(
    const drogon::HttpRequestPtr& /*req*/,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        reservacionService_.eliminar(id);
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k204NoContent);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

}  // namespace hotel::controllers