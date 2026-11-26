#pragma once

#include <string>
#include <string_view>

namespace hotel::core {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static void setLevelFromString(std::string_view levelName);
    [[nodiscard]] static LogLevel level();

    static void trace(std::string_view message);
    static void debug(std::string_view message);
    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);

    // Oculta automaticamente URIs de MongoDB y credenciales antes de emitir un mensaje.
    [[nodiscard]] static std::string sanitize(std::string_view message);

private:
    static void log(LogLevel messageLevel, std::string_view message);
};

}  // namespace hotel::core