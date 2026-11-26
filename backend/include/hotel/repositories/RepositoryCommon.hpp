#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <bsoncxx/document/element.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/exception/exception.hpp>

namespace hotel::repositories {

template <typename T>
struct PagedResult {
    std::vector<T> items;
    std::int64_t total{0};
    std::int32_t page{1};
    std::int32_t limit{20};
};

// Convierte un string hexadecimal validado a bsoncxx::oid (lanza BadRequestException si falla).
[[nodiscard]] bsoncxx::oid parseOid(std::string_view id, std::string_view fieldName = "id");

// Escapa metacaracteres regex para busquedas parciales seguras contra inyeccion NoSQL / ReDoS.
[[nodiscard]] std::string escapeRegex(std::string_view input);

// Conversion segura entre std::chrono::system_clock::time_point y bsoncxx::types::b_date.
[[nodiscard]] bsoncxx::types::b_date toBsonDate(std::chrono::system_clock::time_point tp);
[[nodiscard]] std::chrono::system_clock::time_point fromBsonDate(const bsoncxx::types::b_date& date);

// Lectores seguros de campos BSON.
[[nodiscard]] std::string readStringOrEmpty(const bsoncxx::document::view& doc, std::string_view key);
[[nodiscard]] bool readBoolOrDefault(const bsoncxx::document::view& doc,
                                     std::string_view key,
                                     bool defaultValue);
[[nodiscard]] std::int32_t readInt32OrDefault(const bsoncxx::document::view& doc,
                                              std::string_view key,
                                              std::int32_t defaultValue);
[[nodiscard]] double readDoubleOrDefault(const bsoncxx::document::view& doc,
                                         std::string_view key,
                                         double defaultValue);
[[nodiscard]] std::chrono::system_clock::time_point readDateOrNow(
    const bsoncxx::document::view& doc,
    std::string_view key);
[[nodiscard]] std::vector<std::string> readStringArray(const bsoncxx::document::view& doc,
                                                       std::string_view key);

// Traduce mongocxx::exception a excepciones de dominio (409 en 11000, 403 en 13, 503 en red).
[[noreturn]] void handleMongoException(const mongocxx::exception& ex,
                                       std::string_view operationContext,
                                       std::string_view duplicateMessage = "Registro duplicado.",
                                       std::string_view duplicateCode = "DUPLICATE_KEY");

}  // namespace hotel::repositories