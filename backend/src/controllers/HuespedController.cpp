#include "hotel/controllers/HuespedController.hpp"

#include <optional>
#include <string>

#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/dto/DtoMappers.hpp"
#include "hotel/dto/JsonHelpers.hpp"

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

HuespedController::HuespedController(hotel::services::HuespedService& huespedService)
    : huespedService_(huespedService) {}

void HuespedController::list(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        hotel::repositories::HuespedFilter filter;
        filter.nombre = getQueryParam(req, "nombre");
        filter.correo = getQueryParam(req, "correo");

        if (const auto activoRaw = getQueryParam(req, "activo"); activoRaw.has_value()) {
            filter.activo = hotel::dto::parseOptionalBoolQuery(*activoRaw, "activo");
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

        const auto pageResult = huespedService_.listar(filter);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(pageResult));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HuespedController::create(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto input = hotel::dto::parseCreateHuespedDto(jsonBody);
        const auto created = huespedService_.crear(input);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(created));
        response->setStatusCode(drogon::k201Created);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HuespedController::getById(const drogon::HttpRequestPtr& /*req*/,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto found = huespedService_.obtenerPorId(id);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(found));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HuespedController::update(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto& jsonBody = requireJsonBody(req);
        const auto input = hotel::dto::parseUpdateHuespedDto(jsonBody);
        const auto updated = huespedService_.actualizar(id, input);

        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(updated));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

void HuespedController::remove(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                               std::string id) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        bool fisico = false;
        if (const auto fisicoRaw = getQueryParam(req, "fisico"); fisicoRaw.has_value()) {
            fisico = hotel::dto::parseOptionalBoolQuery(*fisicoRaw, "fisico").value_or(false);
        }

        const auto result = huespedService_.eliminar(id, fisico);
        if (result.eliminacionFisica) {
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k204NoContent);
            response->addHeader("X-Request-Id", requestId);
            callback(response);
            return;
        }

        auto response = drogon::HttpResponse::newHttpJsonResponse(
            hotel::dto::toJson(*result.huespedDesactivado));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

}  // namespace hotel::controllers