#include "hotel/controllers/DisponibilidadController.hpp"

#include <optional>
#include <string>
#include <vector>

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

}  // namespace

DisponibilidadController::DisponibilidadController(
    hotel::services::DisponibilidadService& disponibilidadService)
    : disponibilidadService_(disponibilidadService) {}

void DisponibilidadController::consultar(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto fechaEntrada = getQueryParam(req, "fechaEntrada");
        const auto fechaSalida = getQueryParam(req, "fechaSalida");

        std::vector<hotel::core::FieldError> missingFields;
        if (!fechaEntrada.has_value()) {
            missingFields.push_back(
                {"fechaEntrada", "El parametro de consulta 'fechaEntrada' es obligatorio (YYYY-MM-DD)."});
        }
        if (!fechaSalida.has_value()) {
            missingFields.push_back(
                {"fechaSalida", "El parametro de consulta 'fechaSalida' es obligatorio (YYYY-MM-DD)."});
        }
        if (!missingFields.empty()) {
            throw hotel::core::ValidationException(
                "Faltan parametros obligatorios para consultar disponibilidad.",
                std::move(missingFields));
        }

        hotel::services::DisponibilidadQuery query;
        query.fechaEntrada = *fechaEntrada;
        query.fechaSalida = *fechaSalida;
        query.tipo = getQueryParam(req, "tipo");

        if (const auto personasRaw = getQueryParam(req, "personas"); personasRaw.has_value()) {
            query.personas = hotel::dto::parseOptionalInt32Query(*personasRaw, "personas");
        }

        const auto result = disponibilidadService_.consultar(query);
        auto response = drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(result));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

}  // namespace hotel::controllers