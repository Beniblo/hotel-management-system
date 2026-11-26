#include "hotel/validators/InputValidators.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <unordered_set>

#include "hotel/core/Exceptions.hpp"

namespace hotel::validators {

namespace {

std::optional<std::chrono::sys_days> parseIsoDateToSysDays(std::string_view date) noexcept {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return std::nullopt;
    }

    const std::array<std::size_t, 8> digitIndices = {0, 1, 2, 3, 5, 6, 8, 9};
    for (const auto idx : digitIndices) {
        if (std::isdigit(static_cast<unsigned char>(date[idx])) == 0) {
            return std::nullopt;
        }
    }

    const int yearVal = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
                        (date[2] - '0') * 10 + (date[3] - '0');
    const unsigned monthVal = static_cast<unsigned>((date[5] - '0') * 10 + (date[6] - '0'));
    const unsigned dayVal = static_cast<unsigned>((date[8] - '0') * 10 + (date[9] - '0'));

    if (yearVal < 1970 || yearVal > 2100) {
        return std::nullopt;
    }

    const std::chrono::year_month_day ymd{
        std::chrono::year{yearVal},
        std::chrono::month{monthVal},
        std::chrono::day{dayVal}
    };

    if (!ymd.ok()) {
        return std::nullopt;
    }

    return std::chrono::sys_days{ymd};
}

}  // namespace

std::string trim(std::string_view value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return std::string(value.substr(start, end - start));
}

std::string toLower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

std::string normalizeEmail(std::string_view email) {
    return toLower(trim(email));
}

std::string normalizeRoomNumber(std::string_view numero) {
    std::string out = trim(numero);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return out;
}

std::vector<std::string> normalizeStringList(const std::vector<std::string>& items,
                                             std::size_t maxItems,
                                             std::size_t maxItemLength) {
    std::vector<std::string> normalized;
    std::unordered_set<std::string> seen;

    for (const auto& item : items) {
        const std::string cleaned = trim(item);
        if (cleaned.empty()) {
            continue;
        }
        if (cleaned.size() > maxItemLength) {
            throw hotel::core::ValidationException(
                "Uno de los elementos de lista excede la longitud permitida.",
                {{"lista", "Cada elemento debe tener maximo " + std::to_string(maxItemLength) + " caracteres."}});
        }
        const std::string key = toLower(cleaned);
        if (seen.insert(key).second) {
            normalized.push_back(cleaned);
        }
        if (normalized.size() > maxItems) {
            throw hotel::core::ValidationException(
                "La lista contiene demasiados elementos.",
                {{"lista", "Se permiten maximo " + std::to_string(maxItems) + " elementos."}});
        }
    }

    return normalized;
}

bool isValidObjectId(std::string_view id) noexcept {
    if (id.size() != 24) {
        return false;
    }
    return std::all_of(id.begin(), id.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

void requireValidObjectId(std::string_view id, std::string_view fieldName) {
    if (!isValidObjectId(id)) {
        throw hotel::core::BadRequestException(
            "El identificador '" + std::string(fieldName) +
                "' no tiene un formato ObjectId valido (24 caracteres hexadecimales).",
            "INVALID_OBJECT_ID");
    }
}

bool isValidEmail(std::string_view email) noexcept {
    const std::string cleaned = trim(email);
    if (cleaned.size() < 5 || cleaned.size() > 254) {
        return false;
    }

    const auto atPos = cleaned.find('@');
    if (atPos == std::string::npos || atPos == 0 || atPos == cleaned.size() - 1) {
        return false;
    }
    if (cleaned.find('@', atPos + 1) != std::string::npos) {
        return false;
    }

    const std::string_view local(cleaned.data(), atPos);
    const std::string_view domain(cleaned.data() + atPos + 1, cleaned.size() - atPos - 1);

    if (local.size() > 64 || domain.size() < 3 || domain.size() > 253) {
        return false;
    }
    if (local.front() == '.' || local.back() == '.' ||
        domain.front() == '.' || domain.back() == '.' ||
        domain.front() == '-' || domain.back() == '-') {
        return false;
    }
    if (cleaned.find("..") != std::string::npos) {
        return false;
    }

    const auto dotPos = domain.rfind('.');
    if (dotPos == std::string_view::npos || dotPos == 0 || domain.size() - dotPos - 1 < 2) {
        return false;
    }

    for (const char c : local) {
        const auto ch = static_cast<unsigned char>(c);
        const bool ok = std::isalnum(ch) != 0 || c == '.' || c == '_' || c == '%' || c == '+' || c == '-';
        if (!ok) {
            return false;
        }
    }
    for (const char c : domain) {
        const auto ch = static_cast<unsigned char>(c);
        const bool ok = std::isalnum(ch) != 0 || c == '.' || c == '-';
        if (!ok) {
            return false;
        }
    }

    return true;
}

bool isValidPhone(std::string_view phone) noexcept {
    const std::string cleaned = trim(phone);
    if (cleaned.size() < 7 || cleaned.size() > 20) {
        return false;
    }

    std::size_t digitCount = 0;
    for (const char c : cleaned) {
        const auto ch = static_cast<unsigned char>(c);
        if (std::isdigit(ch) != 0) {
            ++digitCount;
        } else if (c != '+' && c != '-' && c != ' ' && c != '(' && c != ')') {
            return false;
        }
    }

    return digitCount >= 7 && digitCount <= 15;
}

bool isValidRoomNumber(std::string_view numero) noexcept {
    const std::string cleaned = trim(numero);
    if (cleaned.empty() || cleaned.size() > 20) {
        return false;
    }
    return std::all_of(cleaned.begin(), cleaned.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '-';
    });
}

bool isValidRoomType(std::string_view tipo) {
    const std::string normalized = toLower(trim(tipo));
    return normalized == "sencilla" ||
           normalized == "doble" ||
           normalized == "triple" ||
           normalized == "suite" ||
           normalized == "familiar";
}

bool isValidIsoDate(std::string_view date) noexcept {
    return parseIsoDateToSysDays(date).has_value();
}

std::int64_t isoDateToUtcMidnightMs(std::string_view date) {
    const auto daysOpt = parseIsoDateToSysDays(date);
    if (!daysOpt.has_value()) {
        throw hotel::core::ValidationException(
            "Formato de fecha invalido.",
            {{"fecha", "La fecha '" + std::string(date) + "' debe usar formato valido YYYY-MM-DD."}});
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               daysOpt->time_since_epoch())
        .count();
}

std::string utcMsToIsoDate(std::int64_t epochMs) {
    const std::chrono::sys_time<std::chrono::milliseconds> tp{std::chrono::milliseconds{epochMs}};
    const auto sd = std::chrono::floor<std::chrono::days>(tp);
    const std::chrono::year_month_day ymd{sd};

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << static_cast<int>(ymd.year()) << '-'
        << std::setw(2) << static_cast<unsigned>(ymd.month()) << '-'
        << std::setw(2) << static_cast<unsigned>(ymd.day());
    return oss.str();
}

std::string formatUtcTimestamp(std::chrono::system_clock::time_point tp) {
    const std::time_t rawTime = std::chrono::system_clock::to_time_t(tp);
    std::tm utcTm{};
#ifdef _WIN32
    gmtime_s(&utcTm, &rawTime);
#else
    gmtime_r(&rawTime, &utcTm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::int32_t calculateNights(std::string_view fechaEntrada, std::string_view fechaSalida) {
    const auto entrada = parseIsoDateToSysDays(fechaEntrada);
    const auto salida = parseIsoDateToSysDays(fechaSalida);

    std::vector<hotel::core::FieldError> errors;
    if (!entrada.has_value()) {
        errors.push_back({"fechaEntrada", "Debe tener formato valido YYYY-MM-DD."});
    }
    if (!salida.has_value()) {
        errors.push_back({"fechaSalida", "Debe tener formato valido YYYY-MM-DD."});
    }
    if (!errors.empty()) {
        throw hotel::core::ValidationException("Fechas de estancia invalidas.", std::move(errors));
    }

    const auto diffDays = (*salida - *entrada).count();
    if (diffDays <= 0) {
        throw hotel::core::ValidationException(
            "Rango de fechas invalido.",
            {{"fechaSalida", "fechaSalida debe ser estrictamente posterior a fechaEntrada."}});
    }

    return static_cast<std::int32_t>(diffDays);
}

bool datesOverlap(std::string_view entrada1,
                  std::string_view salida1,
                  std::string_view entrada2,
                  std::string_view salida2) {
    static_cast<void>(calculateNights(entrada1, salida1));
    static_cast<void>(calculateNights(entrada2, salida2));

    const auto e1 = *parseIsoDateToSysDays(entrada1);
    const auto s1 = *parseIsoDateToSysDays(salida1);
    const auto e2 = *parseIsoDateToSysDays(entrada2);
    const auto s2 = *parseIsoDateToSysDays(salida2);

    return e1 < s2 && s1 > e2;
}

double roundCurrency(double amount) noexcept {
    if (!std::isfinite(amount)) {
        return 0.0;
    }
    const double rounded = std::round(amount * 100.0) / 100.0;
    return (rounded == -0.0) ? 0.0 : rounded;
}

double calculateReservationTotal(std::int32_t noches, double tarifaPorNoche) {
    if (noches <= 0) {
        throw hotel::core::ValidationException(
            "El numero de noches debe ser mayor a cero.",
            {{"noches", "Debe ser un entero positivo."}});
    }
    if (!std::isfinite(tarifaPorNoche) || tarifaPorNoche <= 0.0) {
        throw hotel::core::ValidationException(
            "La tarifa por noche debe ser mayor a cero.",
            {{"tarifa", "Debe ser un monto positivo."}});
    }
    return roundCurrency(static_cast<double>(noches) * tarifaPorNoche);
}

void validatePagination(std::int32_t page, std::int32_t limit) {
    std::vector<hotel::core::FieldError> errors;
    if (page < 1) {
        errors.push_back({"page", "El parametro page debe ser mayor o igual a 1."});
    }
    if (limit < 1 || limit > 100) {
        errors.push_back({"limit", "El parametro limit debe estar entre 1 y 100."});
    }
    if (!errors.empty()) {
        throw hotel::core::BadRequestException(
            "Parametros de paginacion invalidos (page >= 1, 1 <= limit <= 100).",
            "INVALID_PAGINATION");
    }
}

void validateReportDateRange(std::string_view fechaInicio,
                             std::string_view fechaFin,
                             std::int32_t maxDays) {
    const auto inicio = parseIsoDateToSysDays(fechaInicio);
    const auto fin = parseIsoDateToSysDays(fechaFin);

    std::vector<hotel::core::FieldError> errors;
    if (!inicio.has_value()) {
        errors.push_back({"fechaInicio", "Debe tener formato valido YYYY-MM-DD."});
    }
    if (!fin.has_value()) {
        errors.push_back({"fechaFin", "Debe tener formato valido YYYY-MM-DD."});
    }
    if (!errors.empty()) {
        throw hotel::core::ValidationException("Fechas de reporte invalidas.", std::move(errors));
    }

    const auto diffDays = (*fin - *inicio).count();
    if (diffDays < 0) {
        throw hotel::core::ValidationException(
            "Rango de fechas de reporte invalido.",
            {{"fechaFin", "fechaFin debe ser mayor o igual a fechaInicio."}});
    }
    if (diffDays > maxDays) {
        throw hotel::core::ValidationException(
            "El periodo solicitado excede el maximo permitido.",
            {{"fechaFin", "El rango maximo permitido es de " + std::to_string(maxDays) + " dias."}});
    }
}

void validateHuespedData(std::string_view nombre,
                         std::string_view correo,
                         std::string_view telefono,
                         const std::vector<std::string>& preferencias) {
    std::vector<hotel::core::FieldError> errors;

    const std::string cleanName = trim(nombre);
    if (cleanName.size() < 2 || cleanName.size() > 120) {
        errors.push_back({"nombre", "El nombre es obligatorio y debe tener entre 2 y 120 caracteres."});
    }
    if (!isValidEmail(correo)) {
        errors.push_back({"correo", "El correo electronico no tiene un formato valido."});
    }
    if (!isValidPhone(telefono)) {
        errors.push_back({"telefono", "El telefono debe contener entre 7 y 15 digitos validos."});
    }
    if (preferencias.size() > 25) {
        errors.push_back({"preferencias", "Se permiten maximo 25 preferencias por huesped."});
    }

    if (!errors.empty()) {
        throw hotel::core::ValidationException("Datos de huesped invalidos.", std::move(errors));
    }

    static_cast<void>(normalizeStringList(preferencias));
}

void validateHabitacionData(std::string_view numero,
                            std::string_view tipo,
                            std::int32_t capacidad,
                            double tarifa,
                            const std::vector<std::string>& servicios) {
    std::vector<hotel::core::FieldError> errors;

    if (!isValidRoomNumber(numero)) {
        errors.push_back({"numero", "El numero de habitacion es obligatorio (solo letras, digitos o guion, max 20 caracteres)."});
    }
    if (!isValidRoomType(tipo)) {
        errors.push_back({"tipo", "El tipo debe ser uno de: sencilla, doble, triple, suite o familiar."});
    }
    if (capacidad < 1 || capacidad > 20) {
        errors.push_back({"capacidad", "La capacidad debe ser un entero entre 1 y 20 personas."});
    }
    if (!std::isfinite(tarifa) || tarifa <= 0.0 || tarifa > 1000000.0) {
        errors.push_back({"tarifa", "La tarifa por noche debe ser un numero positivo valido."});
    }
    if (servicios.size() > 25) {
        errors.push_back({"servicios", "Se permiten maximo 25 servicios por habitacion."});
    }

    if (!errors.empty()) {
        throw hotel::core::ValidationException("Datos de habitacion invalidos.", std::move(errors));
    }

    static_cast<void>(normalizeStringList(servicios));
}

void validateReservacionData(std::string_view huespedId,
                             std::string_view habitacionId,
                             std::string_view fechaEntrada,
                             std::string_view fechaSalida,
                             std::int32_t personas) {
    std::vector<hotel::core::FieldError> errors;

    if (!isValidObjectId(huespedId)) {
        errors.push_back({"huespedId", "Debe ser un ObjectId hexadecimal valido de 24 caracteres."});
    }
    if (!isValidObjectId(habitacionId)) {
        errors.push_back({"habitacionId", "Debe ser un ObjectId hexadecimal valido de 24 caracteres."});
    }
    if (!isValidIsoDate(fechaEntrada)) {
        errors.push_back({"fechaEntrada", "Debe tener formato valido YYYY-MM-DD."});
    }
    if (!isValidIsoDate(fechaSalida)) {
        errors.push_back({"fechaSalida", "Debe tener formato valido YYYY-MM-DD."});
    }
    if (personas < 1 || personas > 20) {
        errors.push_back({"personas", "El numero de personas debe estar entre 1 y 20."});
    }

    if (!errors.empty()) {
        throw hotel::core::ValidationException("Datos de reservacion invalidos.", std::move(errors));
    }

    static_cast<void>(calculateNights(fechaEntrada, fechaSalida));
}

}  // namespace hotel::validators