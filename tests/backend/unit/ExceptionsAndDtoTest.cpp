#include <gtest/gtest.h>

#include <json/json.h>
#include <stdexcept>
#include <string>

#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/dto/DtoMappers.hpp"
#include "hotel/dto/JsonHelpers.hpp"
#include "hotel/repositories/RepositoryCommon.hpp"

namespace {

using namespace hotel::core;
using namespace hotel::dto;

TEST(ExceptionsTest, HttpStatusCodesMatchContract) {
    EXPECT_EQ(BadRequestException("bad").httpStatus(), 400);
    EXPECT_EQ(ForbiddenOperationException("denied").httpStatus(), 403);
    EXPECT_EQ(DatabaseAuthorizationException("unauthorized").httpStatus(), 403);
    EXPECT_EQ(NotFoundException("missing").httpStatus(), 404);
    EXPECT_EQ(ConflictException("dup").httpStatus(), 409);
    EXPECT_EQ(ValidationException("invalid", {{"correo", "invalido"}}).httpStatus(), 422);
    EXPECT_EQ(InternalServerException("boom").httpStatus(), 500);
    EXPECT_EQ(DatabaseUnavailableException("down").httpStatus(), 503);
}

TEST(ErrorMapperTest, BuildsUniformEnvelopeAndHidesInternalErrors) {
    const ValidationException valEx(
        "Datos invalidos.",
        {{"fechaSalida", "Debe ser posterior a fechaEntrada."}},
        "VALIDATION_ERROR");

    const Json::Value errJson = ErrorMapper::buildErrorJson(valEx, "req-unit-01");
    EXPECT_EQ(errJson["status"].asInt(), 422);
    EXPECT_EQ(errJson["errorCode"].asString(), "VALIDATION_ERROR");
    EXPECT_EQ(errJson["requestId"].asString(), "req-unit-01");
    ASSERT_TRUE(errJson["details"].isArray());
    ASSERT_EQ(errJson["details"].size(), 1U);
    EXPECT_EQ(errJson["details"][0]["field"].asString(), "fechaSalida");

    drogon::HttpResponsePtr safe500;
    try {
        throw std::runtime_error("Fallo secreto mongodb+srv://admin:superpass@cluster.net/");
    } catch (...) {
        safe500 = ErrorMapper::fromCurrentException("req-unit-500");
    }

    ASSERT_NE(safe500, nullptr);
    EXPECT_EQ(safe500->statusCode(), drogon::k500InternalServerError);
    EXPECT_EQ(safe500->getHeader("X-Request-Id"), "req-unit-500");
    const std::string bodyStr(safe500->getBody());
    EXPECT_EQ(bodyStr.find("superpass"), std::string::npos);
    EXPECT_EQ(bodyStr.find("mongodb+srv"), std::string::npos);
}

TEST(SecurityHelpersTest, SanitizesSecretsAndEscapesRegex) {
    const std::string rawLog =
        "Fallo al conectar a mongodb+srv://hotel_app:MiClave123@cluster0.xyz.mongodb.net/?w=majority";
    const std::string sanitized = Logger::sanitize(rawLog);
    EXPECT_EQ(sanitized.find("MiClave123"), std::string::npos);
    EXPECT_EQ(sanitized.find("cluster0.xyz"), std::string::npos);
    EXPECT_NE(sanitized.find("<redacted>"), std::string::npos);

    const std::string escaped = hotel::repositories::escapeRegex("ana.*(test)+$");
    EXPECT_EQ(escaped, "ana\\.\\*\\(test\\)\\+\\$");
}

TEST(DtoMappersTest, RejectsInvalidTypesAndProtectsServerFields) {
    // Intento de inyeccion de objeto donde se espera string
    Json::Value badGuest(Json::objectValue);
    badGuest["nombre"] = "Prueba";
    badGuest["correo"] = Json::Value(Json::objectValue);
    badGuest["correo"]["$ne"] = "";
    badGuest["telefono"] = "4770000001";
    EXPECT_THROW(static_cast<void>(parseCreateHuespedDto(badGuest)), ValidationException);

    // Query params estrictos
    EXPECT_EQ(parseOptionalInt32Query("15", "limit"), 15);
    EXPECT_THROW(static_cast<void>(parseOptionalInt32Query("15abc", "limit")), BadRequestException);
    EXPECT_EQ(parseOptionalBoolQuery("true", "activo"), true);
    EXPECT_EQ(parseOptionalBoolQuery("false", "activo"), false);
    EXPECT_THROW(static_cast<void>(parseOptionalBoolQuery("si", "activo")), BadRequestException);

    // Lista paginada vacia serializa data como []
    hotel::repositories::PagedResult<hotel::models::Habitacion> emptyRooms;
    emptyRooms.total = 0;
    emptyRooms.page = 1;
    emptyRooms.limit = 10;
    const Json::Value jsonPage = toJson(emptyRooms);
    ASSERT_TRUE(jsonPage["data"].isArray());
    EXPECT_TRUE(jsonPage["data"].empty());
    EXPECT_EQ(jsonPage["meta"]["totalPages"].asInt64(), 0);
}

}  // namespace