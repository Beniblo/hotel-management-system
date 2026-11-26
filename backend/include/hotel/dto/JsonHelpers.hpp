#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <json/json.h>

namespace hotel::dto {

// Valida que la raiz del body sea un objeto JSON valido (lanza 400 si no lo es).
void requireJsonObject(const Json::Value& json);

// Extractores estrictos de campos JSON (rechazan tipos incorrectos y objetos anidados con 422).
[[nodiscard]] std::string requireStringField(const Json::Value& obj, const std::string& field);
[[nodiscard]] std::optional<std::string> optionalStringField(const Json::Value& obj,
                                                             const std::string& field);

[[nodiscard]] std::int32_t requireInt32Field(const Json::Value& obj, const std::string& field);
[[nodiscard]] std::optional<std::int32_t> optionalInt32Field(const Json::Value& obj,
                                                             const std::string& field);

[[nodiscard]] double requireDoubleField(const Json::Value& obj, const std::string& field);
[[nodiscard]] std::optional<double> optionalDoubleField(const Json::Value& obj,
                                                        const std::string& field);

[[nodiscard]] std::optional<bool> optionalBoolField(const Json::Value& obj,
                                                    const std::string& field);

[[nodiscard]] std::optional<std::vector<std::string>> optionalStringArrayField(
    const Json::Value& obj,
    const std::string& field);

// Conversores estrictos de parametros de query string (lanza 400 si el formato es ilegal).
[[nodiscard]] std::optional<std::int32_t> parseOptionalInt32Query(std::string_view raw,
                                                                  std::string_view paramName);
[[nodiscard]] std::optional<double> parseOptionalDoubleQuery(std::string_view raw,
                                                             std::string_view paramName);
[[nodiscard]] std::optional<bool> parseOptionalBoolQuery(std::string_view raw,
                                                         std::string_view paramName);

// Construye un arreglo JSON de cadenas garantizando [] cuando esta vacio.
[[nodiscard]] Json::Value toJsonStringArray(const std::vector<std::string>& items);

// Construye metadatos estandar de paginacion.
[[nodiscard]] Json::Value buildPaginationMeta(std::int64_t total,
                                              std::int32_t page,
                                              std::int32_t limit);

}  // namespace hotel::dto