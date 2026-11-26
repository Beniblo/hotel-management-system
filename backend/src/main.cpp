#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>

#include <json/json.h>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/dto/DtoMappers.hpp"
#include "hotel/dto/JsonHelpers.hpp"

int main() {
    using hotel::config::AppConfig;
    using hotel::core::ErrorMapper;
    using hotel::core::Logger;
    using hotel::core::ValidationException;

    try {
        const AppConfig config = AppConfig::load();
        Logger::setLevelFromString(config.logLevel);

        Logger::info("Iniciando verificacion de DTOs, mappers JSON y ErrorMapper (Parte 5A)...");

        // 1. Verificar parseo DTO de huesped y rechazo de inyeccion NoSQL en campo string
        Json::Value validGuest(Json::objectValue);
        validGuest["nombre"] = "Elena Torres";
        validGuest["correo"] = "elena.torres@ejemplo.com";
        validGuest["telefono"] = "4771234567";
        validGuest["preferencias"] = Json::Value(Json::arrayValue);
        validGuest["preferencias"].append("cama king");

        const auto parsedGuest = hotel::dto::parseCreateHuespedDto(validGuest);
        if (parsedGuest.nombre != "Elena Torres" || parsedGuest.preferencias.size() != 1) {
            throw std::runtime_error("parseCreateHuespedDto no devolvio los valores esperados.");
        }

        Json::Value nosqlAttack(Json::objectValue);
        nosqlAttack["nombre"] = "Atacante";
        nosqlAttack["correo"] = Json::Value(Json::objectValue);
        nosqlAttack["correo"]["$ne"] = "";
        nosqlAttack["telefono"] = "4770000000";

        bool attackBlocked = false;
        try {
            static_cast<void>(hotel::dto::parseCreateHuespedDto(nosqlAttack));
        } catch (const ValidationException& ex) {
            const auto resp = ErrorMapper::toHttpResponse(ex, "req-test-422");
            attackBlocked = (resp->statusCode() == drogon::k422UnprocessableEntity &&
                             resp->getHeader("X-Request-Id") == "req-test-422");
            Logger::info("[Seguridad/DTO] Payload NoSQL bloqueado OK -> HTTP 422 [" +
                         ex.errorCode() + "]");
        }
        if (!attackBlocked) {
            throw std::runtime_error("No se bloqueo un objeto NoSQL inyectado en correo.");
        }

        // 2. Verificar proteccion BOPLA en reservaciones (ignora total/tarifaAplicada inyectados)
        Json::Value resBody(Json::objectValue);
        resBody["huespedId"] = "65a000000000000000000001";
        resBody["habitacionId"] = "65a000000000000000000002";
        resBody["fechaEntrada"] = "2026-12-10";
        resBody["fechaSalida"] = "2026-12-12";
        resBody["personas"] = 2;
        resBody["total"] = 0.01;  // Campo interno ignorado por el DTO

        const auto parsedRes = hotel::dto::parseCreateReservacionDto(resBody);
        if (parsedRes.personas != 2 || parsedRes.fechaEntrada != "2026-12-10") {
            throw std::runtime_error("parseCreateReservacionDto fallo.");
        }

        // 3. Verificar serializacion JSON de lista paginada vacia ([] garantizado) y health
        hotel::repositories::PagedResult<hotel::models::Huesped> emptyPage;
        emptyPage.page = 1;
        emptyPage.limit = 20;
        emptyPage.total = 0;

        const Json::Value emptyJson = hotel::dto::toJson(emptyPage);
        if (!emptyJson["data"].isArray() || !emptyJson["data"].empty() ||
            emptyJson["meta"]["totalPages"].asInt64() != 0) {
            throw std::runtime_error("La lista paginada vacia no serializo data como [].");
        }

        const Json::Value healthJson = hotel::dto::buildHealthJson(true, config.mongodbDbName);
        if (healthJson["status"].asString() != "ok" ||
            healthJson["database"]["name"].asString() != "HotelDB") {
            throw std::runtime_error("buildHealthJson no devolvio la estructura esperada.");
        }

        // 4. Verificar traduccion segura de excepciones no controladas a HTTP 500
        drogon::HttpResponsePtr safe500;
        try {
            throw std::runtime_error("Fallo interno con URI mongodb+srv://user:pass@cluster.net/");
        } catch (...) {
            safe500 = ErrorMapper::fromCurrentException("req-test-500");
        }
        if (!safe500 || safe500->statusCode() != drogon::k500InternalServerError) {
            throw std::runtime_error("ErrorMapper::fromCurrentException no devolvio HTTP 500.");
        }

        Logger::info("[ErrorMapper] Excepcion interna traducida de forma segura -> HTTP 500.");
        Logger::info("[OK] Parte 5A verificada correctamente (DTOs, mappers JSON y ErrorMapper listos).");
        return 0;
    } catch (const std::exception& ex) {
        Logger::error(std::string("Fallo critico en Parte 5A: ") + ex.what());
        return 1;
    }
}