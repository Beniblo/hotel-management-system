#include "hotel/config/AppConfig.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace hotel::config {

namespace {

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) {
        return !isSpace(ch);
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) {
        return !isSpace(ch);
    }).base(), value.end());
    return value;
}

std::string stripQuotes(std::string value) {
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::optional<std::string> getOsEnv(const std::string& key) {
#ifdef _WIN32
    char* buffer = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&buffer, &size, key.c_str()) != 0 || buffer == nullptr) {
        return std::nullopt;
    }
    std::string result(buffer);
    std::free(buffer);
    if (result.empty()) {
        return std::nullopt;
    }
    return result;
#else
    const char* value = std::getenv(key.c_str());
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

std::optional<std::filesystem::path> resolveEnvPath(const std::string& explicitPath) {
    namespace fs = std::filesystem;
    if (!explicitPath.empty()) {
        fs::path path(explicitPath);
        if (fs::exists(path)) {
            return path;
        }
        return std::nullopt;
    }

    const std::vector<fs::path> candidates = {
        fs::path(".env"),
        fs::path("backend/.env"),
        fs::path("../.env"),
        fs::path("../../.env")
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::unordered_map<std::string, std::string> parseEnvFile(const std::filesystem::path& path) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        if (firstLine && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);  // Quitar UTF-8 BOM si Windows Notepad lo agrego
        }
        firstLine = false;

        std::string cleaned = trim(line);
        if (cleaned.empty() || cleaned.front() == '#') {
            continue;
        }

        const auto eqPos = cleaned.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = trim(cleaned.substr(0, eqPos));
        std::string val = stripQuotes(trim(cleaned.substr(eqPos + 1)));
        if (!key.empty()) {
            values[key] = val;
        }
    }
    return values;
}

std::uint32_t parseUint32(const std::string& key,
                          const std::string& raw,
                          std::uint32_t minValue,
                          std::uint32_t maxValue) {
    try {
        std::size_t idx = 0;
        const unsigned long parsed = std::stoul(raw, &idx);
        if (idx != raw.size() || parsed < minValue || parsed > maxValue) {
            throw std::runtime_error("Valor fuera de rango para " + key);
        }
        return static_cast<std::uint32_t>(parsed);
    } catch (const std::exception&) {
        throw std::runtime_error("Variable de configuracion invalida: " + key +
                                 " (debe ser entero entre " + std::to_string(minValue) +
                                 " y " + std::to_string(maxValue) + ")");
    }
}

}  // namespace

AppConfig AppConfig::load(const std::string& envFilePath) {
    std::unordered_map<std::string, std::string> fileValues;
    if (const auto resolved = resolveEnvPath(envFilePath); resolved.has_value()) {
        fileValues = parseEnvFile(*resolved);
    }

    const auto getValue = [&](const std::string& key,
                              const std::string& defaultValue = "") -> std::string {
        if (const auto osVal = getOsEnv(key); osVal.has_value()) {
            return trim(*osVal);
        }
        if (const auto it = fileValues.find(key); it != fileValues.end() && !it->second.empty()) {
            return it->second;
        }
        return defaultValue;
    };

    AppConfig cfg;
    cfg.mongodbUri = getValue("MONGODB_URI");
    cfg.mongodbDbName = getValue("MONGODB_DB_NAME", "HotelDB");
    cfg.serverHost = getValue("SERVER_HOST", "127.0.0.1");
    cfg.corsAllowedOrigin = getValue("CORS_ALLOWED_ORIGIN", "http://localhost:5173");
    cfg.logLevel = toUpper(getValue("LOG_LEVEL", "INFO"));

    // Validacion fail-fast de parametros criticos
    if (cfg.mongodbUri.empty()) {
        throw std::runtime_error(
            "Configuracion incompleta: falta MONGODB_URI. Define la variable de entorno o backend/.env.");
    }
    if (cfg.mongodbUri.rfind("mongodb://", 0) != 0 &&
        cfg.mongodbUri.rfind("mongodb+srv://", 0) != 0) {
        throw std::runtime_error(
            "Configuracion invalida: MONGODB_URI debe iniciar con 'mongodb://' o 'mongodb+srv://'.");
    }
    if (cfg.mongodbUri.find('<') != std::string::npos ||
        cfg.mongodbUri.find('>') != std::string::npos) {
        throw std::runtime_error(
            "Configuracion invalida: MONGODB_URI contiene marcadores de plantilla (<...>).");
    }
    if (cfg.mongodbDbName.empty()) {
        throw std::runtime_error("Configuracion incompleta: falta MONGODB_DB_NAME.");
    }

    cfg.mongodbMaxPoolSize = parseUint32(
        "MONGODB_MAX_POOL_SIZE", getValue("MONGODB_MAX_POOL_SIZE", "16"), 1, 100);
    cfg.mongodbConnectTimeoutMs = parseUint32(
        "MONGODB_CONNECT_TIMEOUT_MS", getValue("MONGODB_CONNECT_TIMEOUT_MS", "5000"), 500, 60000);
    cfg.mongodbServerSelectionTimeoutMs = parseUint32(
        "MONGODB_SERVER_SELECTION_TIMEOUT_MS",
        getValue("MONGODB_SERVER_SELECTION_TIMEOUT_MS", "5000"),
        500,
        60000);

    const auto portValue = parseUint32("SERVER_PORT", getValue("SERVER_PORT", "8080"), 1, 65535);
    cfg.serverPort = static_cast<std::uint16_t>(portValue);

    const std::vector<std::string> validLevels = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};
    if (std::find(validLevels.begin(), validLevels.end(), cfg.logLevel) == validLevels.end()) {
        throw std::runtime_error(
            "Configuracion invalida: LOG_LEVEL debe ser TRACE, DEBUG, INFO, WARN o ERROR.");
    }

    return cfg;
}

std::string AppConfig::maskUri(const std::string& uri) {
    if (uri.rfind("mongodb+srv://", 0) == 0) {
        return "mongodb+srv://***:***@<redacted-cluster>";
    }
    if (uri.rfind("mongodb://", 0) == 0) {
        return "mongodb://***:***@<redacted-host>";
    }
    return "<redacted-uri>";
}

}  // namespace hotel::config