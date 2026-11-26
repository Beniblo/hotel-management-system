#include "hotel/middleware/SecurityMiddleware.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_set>

#include <json/json.h>

#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::middleware {

namespace {

std::unordered_set<std::string> parseAllowedOrigins(const std::string& rawOrigins) {
    std::unordered_set<std::string> origins;
    std::size_t start = 0;

    while (start <= rawOrigins.size()) {
        const std::size_t comma = rawOrigins.find(',', start);
        const std::size_t len =
            (comma == std::string::npos) ? (rawOrigins.size() - start) : (comma - start);
        std::string item = hotel::validators::trim(std::string_view(rawOrigins).substr(start, len));

        while (!item.empty() && item.back() == '/') {
            item.pop_back();
        }
        if (!item.empty()) {
            origins.insert(std::move(item));
        }

        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }

    return origins;
}

bool isOriginAllowed(const std::unordered_set<std::string>& allowedOrigins,
                     const std::string& origin) {
    std::string cleaned = hotel::validators::trim(origin);
    while (!cleaned.empty() && cleaned.back() == '/') {
        cleaned.pop_back();
    }
    return allowedOrigins.find(cleaned) != allowedOrigins.end();
}

bool endsWith(std::string_view text, std::string_view suffix) noexcept {
    return text.size() >= suffix.size() &&
           text.substr(text.size() - suffix.size()) == suffix;
}

bool isJsonContentType(const std::string& contentType) {
    const std::string lower = hotel::validators::toLower(hotel::validators::trim(contentType));
    return lower.rfind("application/json", 0) == 0 ||
           lower.find("+json") != std::string::npos;
}

bool containsSuspiciousQueryKey(const drogon::HttpRequestPtr& req) {
    for (const auto& [key, value] : req->getParameters()) {
        if (key.find('$') != std::string::npos || key.find('\0') != std::string::npos) {
            return true;
        }
        if (value.find('\0') != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsDollarKeyRecursive(const Json::Value& node, int depth = 0) {
    if (depth > 16) {
        return true;  // Evita anidamiento abusivo
    }
    if (node.isObject()) {
        for (const auto& key : node.getMemberNames()) {
            if (!key.empty() && key.front() == '$') {
                return true;
            }
            if (containsDollarKeyRecursive(node[key], depth + 1)) {
                return true;
            }
        }
    } else if (node.isArray()) {
        for (Json::ArrayIndex i = 0; i < node.size(); ++i) {
            if (containsDollarKeyRecursive(node[i], depth + 1)) {
                return true;
            }
        }
    }
    return false;
}

void applyCorsHeaders(const drogon::HttpResponsePtr& resp, const std::string& allowedOrigin) {
    resp->addHeader("Access-Control-Allow-Origin", allowedOrigin);
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Accept");
    resp->addHeader("Access-Control-Expose-Headers", "X-Request-Id");
    resp->addHeader("Access-Control-Max-Age", "600");
    resp->addHeader("Vary", "Origin");
}

void applySecurityHeaders(const drogon::HttpResponsePtr& resp) {
    resp->addHeader("X-Content-Type-Options", "nosniff");
    resp->addHeader("X-Frame-Options", "DENY");
    resp->addHeader("Referrer-Policy", "no-referrer");
    resp->addHeader("X-Permitted-Cross-Domain-Policies", "none");
    resp->addHeader("Cache-Control", "no-store");
}

}  // namespace

void SecurityMiddleware::registerAll(drogon::HttpAppFramework& app,
                                     const hotel::config::AppConfig& config) {
    const auto allowedOrigins = parseAllowedOrigins(config.corsAllowedOrigin);

    hotel::core::Logger::info("Middleware de seguridad activo. CORS permitido para: " +
                              config.corsAllowedOrigin);

    // 1. Filtro previo al enrutamiento (CORS, preflight OPTIONS, Content-Type y anti-NoSQLi)
    app.registerPreRoutingAdvice(
        [allowedOrigins](const drogon::HttpRequestPtr& req,
                         drogon::AdviceCallback&& acb,
                         drogon::AdviceChainCallback&& accb) {
            const std::string origin = req->getHeader("origin");

            // Validacion estricta de origen cuando el navegador envia cabecera Origin
            if (!origin.empty() && !isOriginAllowed(allowedOrigins, origin)) {
                const std::string reqId = hotel::core::ErrorMapper::generateRequestId();
                hotel::core::Logger::warn("[" + reqId + "] Origen CORS rechazado: " + origin);

                const hotel::core::ForbiddenOperationException ex(
                    "Origen HTTP no autorizado por la politica CORS de la API.",
                    "CORS_ORIGIN_DENIED");
                auto resp = hotel::core::ErrorMapper::toHttpResponse(ex, reqId);
                applySecurityHeaders(resp);
                acb(resp);
                return;
            }

            // Responder preflight CORS (OPTIONS) directamente
            if (req->method() == drogon::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k204NoContent);
                if (!origin.empty()) {
                    applyCorsHeaders(resp, origin);
                }
                applySecurityHeaders(resp);
                acb(resp);
                return;
            }

            // Bloquear claves de query sospechosas (ej. ?correo[$ne]=x)
            if (containsSuspiciousQueryKey(req)) {
                const std::string reqId = hotel::core::ErrorMapper::generateRequestId();
                hotel::core::Logger::warn(
                    "[" + reqId + "] Parametro de consulta sospechoso bloqueado en " + req->path());

                const hotel::core::BadRequestException ex(
                    "Se detectaron parametros de consulta no permitidos.",
                    "NOSQL_INJECTION_BLOCKED");
                auto resp = hotel::core::ErrorMapper::toHttpResponse(ex, reqId);
                acb(resp);
                return;
            }

            // Validacion estricta de Content-Type y auditoria de operadores '$' en JSON
            const auto method = req->method();
            const bool isWriteMethod =
                (method == drogon::Post || method == drogon::Put || method == drogon::Patch);

            if (isWriteMethod) {
                const bool isEmptyCancel =
                    (method == drogon::Post && endsWith(req->path(), "/cancelar") &&
                     req->getBody().empty());

                if (!isEmptyCancel) {
                    const std::string contentType = req->getHeader("content-type");
                    if (contentType.empty() || !isJsonContentType(contentType)) {
                        const std::string reqId = hotel::core::ErrorMapper::generateRequestId();
                        const hotel::core::AppException ex(
                            415,
                            "UNSUPPORTED_MEDIA_TYPE",
                            "El encabezado Content-Type debe ser 'application/json'.");
                        auto resp = hotel::core::ErrorMapper::toHttpResponse(ex, reqId);
                        acb(resp);
                        return;
                    }

                    if (const auto jsonPtr = req->getJsonObject();
                        jsonPtr && containsDollarKeyRecursive(*jsonPtr)) {
                        const std::string reqId = hotel::core::ErrorMapper::generateRequestId();
                        hotel::core::Logger::warn(
                            "[" + reqId + "] Operador NoSQL ($) detectado en payload JSON en " +
                            req->path());

                        const hotel::core::BadRequestException ex(
                            "El cuerpo JSON contiene claves de operador no permitidas.",
                            "NOSQL_INJECTION_BLOCKED");
                        auto resp = hotel::core::ErrorMapper::toHttpResponse(ex, reqId);
                        acb(resp);
                        return;
                    }
                }
            }

            accb();
        });

    // 2. Filtro posterior al procesamiento (cabeceras de seguridad, CORS y acceso sanitizado)
    app.registerPostHandlingAdvice(
        [allowedOrigins](const drogon::HttpRequestPtr& req,
                         const drogon::HttpResponsePtr& resp) {
            if (!resp) {
                return;
            }

            applySecurityHeaders(resp);

            const std::string origin = req->getHeader("origin");
            if (!origin.empty() && isOriginAllowed(allowedOrigins, origin)) {
                applyCorsHeaders(resp, origin);
            }

            if (resp->getHeader("x-request-id").empty()) {
                resp->addHeader("X-Request-Id", hotel::core::ErrorMapper::generateRequestId());
            }

            hotel::core::Logger::info(std::string(req->methodString()) + " " + req->path() +
                                      " -> HTTP " +
                                      std::to_string(static_cast<int>(resp->statusCode())));
        });
}

}  // namespace hotel::middleware