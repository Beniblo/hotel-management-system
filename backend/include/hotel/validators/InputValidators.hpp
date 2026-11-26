#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hotel::validators {

// Limpieza y normalizacion de texto
[[nodiscard]] std::string trim(std::string_view value);
[[nodiscard]] std::string toLower(std::string_view value);
[[nodiscard]] std::string normalizeEmail(std::string_view email);
[[nodiscard]] std::string normalizeRoomNumber(std::string_view numero);
[[nodiscard]] std::vector<std::string> normalizeStringList(
    const std::vector<std::string>& items,
    std::size_t maxItems = 25,
    std::size_t maxItemLength = 60);

// Validadores primitivos
[[nodiscard]] bool isValidObjectId(std::string_view id) noexcept;
void requireValidObjectId(std::string_view id, std::string_view fieldName = "id");

[[nodiscard]] bool isValidEmail(std::string_view email) noexcept;
[[nodiscard]] bool isValidPhone(std::string_view phone) noexcept;
[[nodiscard]] bool isValidRoomNumber(std::string_view numero) noexcept;
[[nodiscard]] bool isValidRoomType(std::string_view tipo);
[[nodiscard]] bool isValidIsoDate(std::string_view date) noexcept;

// Utilidades de fechas UTC (YYYY-MM-DD) sin desfase por zona horaria local
[[nodiscard]] std::int64_t isoDateToUtcMidnightMs(std::string_view date);
[[nodiscard]] std::string utcMsToIsoDate(std::int64_t epochMs);
[[nodiscard]] std::string formatUtcTimestamp(std::chrono::system_clock::time_point tp);

// Reglas de negocio de fechas, traslapes y montos
[[nodiscard]] std::int32_t calculateNights(std::string_view fechaEntrada,
                                           std::string_view fechaSalida);

// Intervalos semiabiertos [E1, S1) y [E2, S2): se traslapan si y solo si E1 < S2 && S1 > E2
[[nodiscard]] bool datesOverlap(std::string_view entrada1,
                                std::string_view salida1,
                                std::string_view entrada2,
                                std::string_view salida2);

[[nodiscard]] double roundCurrency(double amount) noexcept;
[[nodiscard]] double calculateReservationTotal(std::int32_t noches, double tarifaPorNoche);

// Validaciones de paginacion, rangos y entidades completas (lanzan ValidationException / BadRequestException)
void validatePagination(std::int32_t page, std::int32_t limit);
void validateReportDateRange(std::string_view fechaInicio,
                             std::string_view fechaFin,
                             std::int32_t maxDays = 366);

void validateHuespedData(std::string_view nombre,
                         std::string_view correo,
                         std::string_view telefono,
                         const std::vector<std::string>& preferencias);

void validateHabitacionData(std::string_view numero,
                            std::string_view tipo,
                            std::int32_t capacidad,
                            double tarifa,
                            const std::vector<std::string>& servicios);

void validateReservacionData(std::string_view huespedId,
                             std::string_view habitacionId,
                             std::string_view fechaEntrada,
                             std::string_view fechaSalida,
                             std::int32_t personas);

}  // namespace hotel::validators