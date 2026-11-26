#include "hotel/repositories/RepositoryCommon.hpp"

#include <optional>
#include <string>

#include <mongocxx/exception/operation_exception.hpp>

#include "hotel/core/Exceptions.hpp"
#include "hotel/core/Logger.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::repositories {

namespace {

std::optional<int> extractServerErrorCode(const mongocxx::exception& ex) {
    if (const auto* opEx = dynamic_cast<const mongocxx::operation_exception*>(&ex)) {
        if (const auto& raw = opEx->raw_server_error(); raw.has_value()) {
            const auto view = raw->view();
            if (const auto codeElem = view["code"]; codeElem && codeElem.type() == bsoncxx::type::k_int32) {
                return codeElem.get_int32().value;
            }
            if (const auto writeErrors = view["writeErrors"];
                writeErrors && writeErrors.type() == bsoncxx::type::k_array) {
                for (const auto& errItem : writeErrors.get_array().value) {
                    if (errItem.type() == bsoncxx::type::k_document) {
                        const auto errDoc = errItem.get_document().value;
                        if (const auto codeElem = errDoc["code"];
                            codeElem && codeElem.type() == bsoncxx::type::k_int32) {
                            return codeElem.get_int32().value;
                        }
                    }
                }
            }
        }
    }
    return std::nullopt;
}

}  // namespace

bsoncxx::oid parseOid(std::string_view id, std::string_view fieldName) {
    hotel::validators::requireValidObjectId(id, fieldName);
    return bsoncxx::oid{id};
}

std::string escapeRegex(std::string_view input) {
    std::string escaped;
    escaped.reserve(input.size() * 2);
    for (const char ch : input) {
        switch (ch) {
            case '\\':
            case '.':
            case '^':
            case '$':
            case '|':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '*':
            case '+':
            case '?':
                escaped.push_back('\\');
                escaped.push_back(ch);
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

bsoncxx::types::b_date toBsonDate(std::chrono::system_clock::time_point tp) {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return bsoncxx::types::b_date{ms};
}

std::chrono::system_clock::time_point fromBsonDate(const bsoncxx::types::b_date& date) {
    return std::chrono::system_clock::time_point{
        std::chrono::duration_cast<std::chrono::system_clock::duration>(date.value)
    };
}

std::string readStringOrEmpty(const bsoncxx::document::view& doc, std::string_view key) {
    const auto elem = doc[key];
    if (elem && elem.type() == bsoncxx::type::k_string) {
        return std::string(elem.get_string().value);
    }
    return {};
}

bool readBoolOrDefault(const bsoncxx::document::view& doc,
                       std::string_view key,
                       bool defaultValue) {
    const auto elem = doc[key];
    if (elem && elem.type() == bsoncxx::type::k_bool) {
        return elem.get_bool().value;
    }
    return defaultValue;
}

std::int32_t readInt32OrDefault(const bsoncxx::document::view& doc,
                                std::string_view key,
                                std::int32_t defaultValue) {
    const auto elem = doc[key];
    if (!elem) {
        return defaultValue;
    }
    if (elem.type() == bsoncxx::type::k_int32) {
        return elem.get_int32().value;
    }
    if (elem.type() == bsoncxx::type::k_int64) {
        return static_cast<std::int32_t>(elem.get_int64().value);
    }
    if (elem.type() == bsoncxx::type::k_double) {
        return static_cast<std::int32_t>(elem.get_double().value);
    }
    return defaultValue;
}

double readDoubleOrDefault(const bsoncxx::document::view& doc,
                           std::string_view key,
                           double defaultValue) {
    const auto elem = doc[key];
    if (!elem) {
        return defaultValue;
    }
    if (elem.type() == bsoncxx::type::k_double) {
        return elem.get_double().value;
    }
    if (elem.type() == bsoncxx::type::k_int32) {
        return static_cast<double>(elem.get_int32().value);
    }
    if (elem.type() == bsoncxx::type::k_int64) {
        return static_cast<double>(elem.get_int64().value);
    }
    return defaultValue;
}

std::chrono::system_clock::time_point readDateOrNow(const bsoncxx::document::view& doc,
                                                    std::string_view key) {
    const auto elem = doc[key];
    if (elem && elem.type() == bsoncxx::type::k_date) {
        return fromBsonDate(elem.get_date());
    }
    return std::chrono::system_clock::now();
}

std::vector<std::string> readStringArray(const bsoncxx::document::view& doc,
                                         std::string_view key) {
    std::vector<std::string> result;
    const auto elem = doc[key];
    if (!elem || elem.type() != bsoncxx::type::k_array) {
        return result;
    }
    for (const auto& item : elem.get_array().value) {
        if (item.type() == bsoncxx::type::k_string) {
            result.emplace_back(item.get_string().value);
        }
    }
    return result;
}

[[noreturn]] void handleMongoException(const mongocxx::exception& ex,
                                       std::string_view operationContext,
                                       std::string_view duplicateMessage,
                                       std::string_view duplicateCode) {
    const std::string details = hotel::core::Logger::sanitize(ex.what());
    const auto serverCode = extractServerErrorCode(ex);
    const int errCode = serverCode.value_or(ex.code().value());

    hotel::core::Logger::error("Error MongoDB en " + std::string(operationContext) +
                               " (codigo=" + std::to_string(errCode) + "): " + details);

    // Codigo 11000: Violacion de indice unico (DuplicateKey)
    if (errCode == 11000 ||
        details.find("E11000") != std::string::npos ||
        details.find("duplicate key") != std::string::npos) {
        throw hotel::core::ConflictException(std::string(duplicateMessage),
                                             std::string(duplicateCode));
    }

    // Codigo 13: Unauthorized / permisos insuficientes (prueba hotel_readonly_user)
    if (errCode == 13 ||
        details.find("not authorized") != std::string::npos ||
        details.find("Unauthorized") != std::string::npos ||
        details.find("Authentication failed") != std::string::npos) {
        throw hotel::core::DatabaseAuthorizationException(
            "La cuenta de base de datos no tiene permisos suficientes para ejecutar esta operacion.");
    }

    throw hotel::core::DatabaseUnavailableException(
        "Error de comunicacion o disponibilidad al acceder a MongoDB Atlas.");
}

}  // namespace hotel::repositories