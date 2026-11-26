#include "hotel/controllers/ReporteController.hpp"

#include <optional>
#include <string>
#include <vector>

#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/dto/DtoMappers.hpp"

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

ReporteController::ReporteController(hotel::services::ReporteService& reporteService)
    : reporteService_(reporteService) {}

void ReporteController::resumen(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const auto fechaInicio = getQueryParam(req, "fechaInicio");
        const auto fechaFin = getQueryParam(req, "fechaFin");

        std::vector<hotel::core::FieldError> missingFields;
        if (!fechaInicio.has_value()) {
            missingFields.push_back(
                {"fechaInicio", "El parametro de consulta 'fechaInicio' es obligatorio (YYYY-MM-DD)."});
        }
        if (!fechaFin.has_value()) {
            missingFields.push_back(
                {"fechaFin", "El parametro de consulta 'fechaFin' es obligatorio (YYYY-MM-DD)."});
        }
        if (!missingFields.empty()) {
            throw hotel::core::ValidationException(
                "Faltan parametros obligatorios para generar el reporte.",
                std::move(missingFields));
        }

        const auto resumenReporte = reporteService_.generarResumen(*fechaInicio, *fechaFin);
        auto response =
            drogon::HttpResponse::newHttpJsonResponse(hotel::dto::toJson(resumenReporte));
        response->setStatusCode(drogon::k200OK);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

}  // namespace hotel::controllers