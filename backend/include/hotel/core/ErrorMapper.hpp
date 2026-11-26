#pragma once

#include <exception>
#include <string>

#include <drogon/HttpResponse.h>
#include <json/json.h>

#include "hotel/core/Exceptions.hpp"

namespace hotel::core {

class ErrorMapper {
public:
    // Genera un identificador unico ligero para correlacionar respuestas HTTP con logs internos.
    [[nodiscard]] static std::string generateRequestId();

    // Construye el cuerpo JSON estandarizado de error sin exponer datos internos.
    [[nodiscard]] static Json::Value buildErrorJson(const AppException& ex,
                                                    const std::string& requestId);

    // Convierte una AppException directamente a HttpResponsePtr con estado HTTP y X-Request-Id.
    [[nodiscard]] static drogon::HttpResponsePtr toHttpResponse(const AppException& ex,
                                                                const std::string& requestId);

    // Captura y traduce cualquier excepcion activa dentro de un bloque catch (...).
    [[nodiscard]] static drogon::HttpResponsePtr fromCurrentException(
        const std::string& requestId = "");
};

}  // namespace hotel::core