#include "hotel/dto/JsonHelpers.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "hotel/core/Exceptions.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::dto {

namespace {

bool isStrictInt32Json(const Json::Value& val, std::int32_t& out) {
    if (val.isBool()) {
        return false;
    }
    if (val.isInt()) {
        out = static_cast<std::int32_t>(val.asInt());
        return true;
    }
    if (val.isInt64()) {
        const auto v = val.asInt64();
        if (v >= std::numeric_limits<std::int32_t>::min() &&
            v <= std::numeric_limits<std::int32_t>::max()) {
            out = static_cast<std::int32_t>(v);
            return true;
        }
        return false;
    }
    if (val.isUInt64()) {
        const auto v = val.asUInt64();
        if (v <= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
            out = static_cast<std::int32_t>(v);
            return true;
        }
        return false;
    }
    return false;
}

}  // namespace

void requireJsonObject(const Json::Value& json) {
    if (!json.isObject()) {
        throw hotel::core::BadRequestException(
            "El cuerpo de la solicitud debe ser un objeto JSON valido.", "INVALID_JSON_BODY");
    }
}

std::string requireStringField(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        throw hotel::core::ValidationException(
            "Falta un campo obligatorio en la solicitud.",
            {{field, "El campo '" + field + "' es obligatorio."}});
    }
    if (!obj[field].isString()) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser una cadena de texto."}});
    }
    return obj[field].asString();
}

std::optional<std::string> optionalStringField(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        return std::nullopt;
    }
    if (!obj[field].isString()) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser una cadena de texto."}});
    }
    return obj[field].asString();
}

std::int32_t requireInt32Field(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        throw hotel::core::ValidationException(
            "Falta un campo obligatorio en la solicitud.",
            {{field, "El campo '" + field + "' es obligatorio."}});
    }
    std::int32_t parsed = 0;
    if (!isStrictInt32Json(obj[field], parsed)) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un numero entero valido."}});
    }
    return parsed;
}

std::optional<std::int32_t> optionalInt32Field(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        return std::nullopt;
    }
    std::int32_t parsed = 0;
    if (!isStrictInt32Json(obj[field], parsed)) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un numero entero valido."}});
    }
    return parsed;
}

double requireDoubleField(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        throw hotel::core::ValidationException(
            "Falta un campo obligatorio en la solicitud.",
            {{field, "El campo '" + field + "' es obligatorio."}});
    }
    if (obj[field].isBool() || !obj[field].isNumeric()) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un valor numerico valido."}});
    }
    const double val = obj[field].asDouble();
    if (!std::isfinite(val)) {
        throw hotel::core::ValidationException(
            "Valor numerico invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un numero finito."}});
    }
    return val;
}

std::optional<double> optionalDoubleField(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        return std::nullopt;
    }
    if (obj[field].isBool() || !obj[field].isNumeric()) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un valor numerico valido."}});
    }
    const double val = obj[field].asDouble();
    if (!std::isfinite(val)) {
        throw hotel::core::ValidationException(
            "Valor numerico invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un numero finito."}});
    }
    return val;
}

std::optional<bool> optionalBoolField(const Json::Value& obj, const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        return std::nullopt;
    }
    if (!obj[field].isBool()) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser booleano (true o false)."}});
    }
    return obj[field].asBool();
}

std::optional<std::vector<std::string>> optionalStringArrayField(const Json::Value& obj,
                                                                 const std::string& field) {
    if (!obj.isMember(field) || obj[field].isNull()) {
        return std::nullopt;
    }
    if (!obj[field].isArray()) {
        throw hotel::core::ValidationException(
            "Tipo de dato invalido en la solicitud.",
            {{field, "El campo '" + field + "' debe ser un arreglo de cadenas de texto."}});
    }

    std::vector<std::string> items;
    items.reserve(obj[field].size());
    for (Json::ArrayIndex i = 0; i < obj[field].size(); ++i) {
        if (!obj[field][i].isString()) {
            throw hotel::core::ValidationException(
                "Tipo de dato invalido en arreglo.",
                {{field, "Todos los elementos de '" + field + "' deben ser cadenas de texto."}});
        }
        items.push_back(obj[field][i].asString());
    }
    return items;
}

std::optional<std::int32_t> parseOptionalInt32Query(std::string_view raw,
                                                    std::string_view paramName) {
    const std::string cleaned = hotel::validators::trim(raw);
    if (cleaned.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t idx = 0;
        const long long parsed = std::stoll(cleaned, &idx);
        if (idx != cleaned.size() ||
            parsed < std::numeric_limits<std::int32_t>::min() ||
            parsed > std::numeric_limits<std::int32_t>::max()) {
            throw std::runtime_error("out_of_range");
        }
        return static_cast<std::int32_t>(parsed);
    } catch (const std::exception&) {
        throw hotel::core::BadRequestException(
            "El parametro de consulta '" + std::string(paramName) + "' debe ser un entero valido.",
            "INVALID_QUERY_PARAM");
    }
}

std::optional<double> parseOptionalDoubleQuery(std::string_view raw,
                                               std::string_view paramName) {
    const std::string cleaned = hotel::validators::trim(raw);
    if (cleaned.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t idx = 0;
        const double parsed = std::stod(cleaned, &idx);
        if (idx != cleaned.size() || !std::isfinite(parsed)) {
            throw std::runtime_error("invalid_double");
        }
        return parsed;
    } catch (const std::exception&) {
        throw hotel::core::BadRequestException(
            "El parametro de consulta '" + std::string(paramName) + "' debe ser un numero valido.",
            "INVALID_QUERY_PARAM");
    }
}

std::optional<bool> parseOptionalBoolQuery(std::string_view raw, std::string_view paramName) {
    const std::string cleaned = hotel::validators::toLower(hotel::validators::trim(raw));
    if (cleaned.empty()) {
        return std::nullopt;
    }
    if (cleaned == "true" || cleaned == "1") {
        return true;
    }
    if (cleaned == "false" || cleaned == "0") {
        return false;
    }
    throw hotel::core::BadRequestException(
        "El parametro de consulta '" + std::string(paramName) +
            "' debe ser booleano (true o false).",
        "INVALID_QUERY_PARAM");
}

Json::Value toJsonStringArray(const std::vector<std::string>& items) {
    Json::Value arr(Json::arrayValue);
    for (const auto& item : items) {
        arr.append(item);
    }
    return arr;
}

Json::Value buildPaginationMeta(std::int64_t total, std::int32_t page, std::int32_t limit) {
    Json::Value meta(Json::objectValue);
    meta["total"] = static_cast<Json::Int64>(total);
    meta["page"] = page;
    meta["limit"] = limit;
    const std::int64_t totalPages =
        (limit > 0 && total > 0) ? ((total + limit - 1) / limit) : 0;
    meta["totalPages"] = static_cast<Json::Int64>(totalPages);
    return meta;
}

}  // namespace hotel::dto