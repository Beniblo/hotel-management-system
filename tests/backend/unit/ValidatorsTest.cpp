#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hotel/core/Exceptions.hpp"
#include "hotel/models/Enums.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace {

using hotel::core::BadRequestException;
using hotel::core::ValidationException;
using namespace hotel::models;
using namespace hotel::validators;

TEST(ValidatorsTest, EmailValidationAndNormalization) {
    EXPECT_TRUE(isValidEmail("ana.lopez@ejemplo.com"));
    EXPECT_TRUE(isValidEmail("  Carlos_Mendoza+vip@hotel-demo.mx  "));
    EXPECT_EQ(normalizeEmail("  ANA.LOPEZ@Ejemplo.COM  "), "ana.lopez@ejemplo.com");

    EXPECT_FALSE(isValidEmail(""));
    EXPECT_FALSE(isValidEmail("sin-arroba.com"));
    EXPECT_FALSE(isValidEmail("doble@@dominio.com"));
    EXPECT_FALSE(isValidEmail(".inicio@dominio.com"));
    EXPECT_FALSE(isValidEmail("usuario@dominio..com"));
    EXPECT_FALSE(isValidEmail("usuario@dominio.a"));
}

TEST(ValidatorsTest, PhoneValidation) {
    EXPECT_TRUE(isValidPhone("4770000001"));
    EXPECT_TRUE(isValidPhone("+52 (477) 123-4567"));

    EXPECT_FALSE(isValidPhone("12345"));
    EXPECT_FALSE(isValidPhone("477-ABC-0001"));
    EXPECT_FALSE(isValidPhone("12345678901234567890"));
}

TEST(ValidatorsTest, ObjectIdValidation) {
    EXPECT_TRUE(isValidObjectId("65a000000000000000000001"));
    EXPECT_TRUE(isValidObjectId("ABCDEF0123456789abcdef01"));

    EXPECT_FALSE(isValidObjectId(""));
    EXPECT_FALSE(isValidObjectId("65a00000000000000000001"));   // 23 chars
    EXPECT_FALSE(isValidObjectId("65a0000000000000000000012")); // 25 chars
    EXPECT_FALSE(isValidObjectId("65a00000000000000000000g"));  // no hex

    EXPECT_NO_THROW(requireValidObjectId("65a000000000000000000001", "id"));
    EXPECT_THROW(requireValidObjectId("id-invalido", "huespedId"), BadRequestException);
}

TEST(ValidatorsTest, RoomNumberAndTypeValidation) {
    EXPECT_TRUE(isValidRoomNumber("101"));
    EXPECT_TRUE(isValidRoomNumber("204-A"));
    EXPECT_EQ(normalizeRoomNumber("  204-a "), "204-A");

    EXPECT_FALSE(isValidRoomNumber(""));
    EXPECT_FALSE(isValidRoomNumber("101 $ne"));

    EXPECT_TRUE(isValidRoomType("sencilla"));
    EXPECT_TRUE(isValidRoomType("SUITE"));
    EXPECT_TRUE(isValidRoomType("Familiar"));
    EXPECT_FALSE(isValidRoomType("presidencial"));
}

TEST(ValidatorsTest, IsoDateValidationAndUtcRoundTrip) {
    EXPECT_TRUE(isValidIsoDate("2026-12-05"));
    EXPECT_TRUE(isValidIsoDate("2024-02-29"));  // bisiesto valido

    EXPECT_FALSE(isValidIsoDate("2026-02-29"));  // no bisiesto
    EXPECT_FALSE(isValidIsoDate("2026-13-01"));
    EXPECT_FALSE(isValidIsoDate("2026-12-32"));
    EXPECT_FALSE(isValidIsoDate("05-12-2026"));

    const auto ms = isoDateToUtcMidnightMs("2026-12-05");
    EXPECT_EQ(utcMsToIsoDate(ms), "2026-12-05");

    EXPECT_THROW(static_cast<void>(isoDateToUtcMidnightMs("2026-02-30")), ValidationException);
}

TEST(ValidatorsTest, PaginationAndReportDateRange) {
    EXPECT_NO_THROW(validatePagination(1, 20));
    EXPECT_NO_THROW(validatePagination(5, 100));
    EXPECT_THROW(validatePagination(0, 20), BadRequestException);
    EXPECT_THROW(validatePagination(1, 0), BadRequestException);
    EXPECT_THROW(validatePagination(1, 101), BadRequestException);

    EXPECT_NO_THROW(validateReportDateRange("2026-12-01", "2026-12-01"));
    EXPECT_NO_THROW(validateReportDateRange("2026-01-01", "2026-12-31"));
    EXPECT_THROW(validateReportDateRange("2026-12-31", "2026-12-01"), ValidationException);
    EXPECT_THROW(validateReportDateRange("2026-01-01", "2028-01-01"), ValidationException);
}

TEST(ValidatorsTest, EntityDataValidation) {
    EXPECT_NO_THROW(
        validateHuespedData("Ana Lopez", "ana@ejemplo.com", "4771234567", {"piso alto"}));
    EXPECT_THROW(
        validateHuespedData("A", "correo-invalido", "123", {}), ValidationException);

    EXPECT_NO_THROW(validateHabitacionData("101", "doble", 2, 1450.0, {"wifi", "tv"}));
    EXPECT_THROW(validateHabitacionData("", "doble", 0, -100.0, {}), ValidationException);

    EXPECT_NO_THROW(validateReservacionData(
        "65a000000000000000000001", "65a000000000000000000002", "2026-12-05", "2026-12-08", 2));
    EXPECT_THROW(
        validateReservacionData(
            "65a000000000000000000001", "65a000000000000000000002", "2026-12-08", "2026-12-05", 2),
        ValidationException);
}

TEST(ValidatorsTest, DomainEnumsParsingAndBlockingPolicy) {
    EXPECT_EQ(parseEstadoHabitacion("DISPONIBLE"), EstadoHabitacion::Disponible);
    EXPECT_EQ(parseEstadoHabitacion("fuera_de_servicio"), EstadoHabitacion::FueraDeServicio);
    EXPECT_FALSE(parseEstadoHabitacion("desconocido").has_value());
    EXPECT_EQ(toString(EstadoHabitacion::Mantenimiento), "mantenimiento");

    EXPECT_EQ(parseEstadoReservacion("en_curso"), EstadoReservacion::EnCurso);
    EXPECT_FALSE(parseEstadoReservacion("expirada").has_value());

    EXPECT_TRUE(isBlockingReservationStatus(EstadoReservacion::Pendiente));
    EXPECT_TRUE(isBlockingReservationStatus(EstadoReservacion::Confirmada));
    EXPECT_TRUE(isBlockingReservationStatus(EstadoReservacion::EnCurso));
    EXPECT_FALSE(isBlockingReservationStatus(EstadoReservacion::Completada));
    EXPECT_FALSE(isBlockingReservationStatus(EstadoReservacion::Cancelada));
}

}  // namespace