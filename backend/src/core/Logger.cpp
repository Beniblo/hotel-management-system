#include "hotel/core/Logger.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace hotel::core {

namespace {

std::atomic<int> gMinLevel{static_cast<int>(LogLevel::Info)};
std::mutex gLogMutex;

std::string currentUtcTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm utcTm{};
#ifdef _WIN32
    gmtime_s(&utcTm, &nowTime);
#else
    gmtime_r(&nowTime, &utcTm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

const char* levelToLabel(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

void redactScheme(std::string& text, std::string_view scheme) {
    std::size_t pos = 0;
    while ((pos = text.find(scheme, pos)) != std::string::npos) {
        std::size_t end = pos + scheme.size();
        while (end < text.size() &&
               std::isspace(static_cast<unsigned char>(text[end])) == 0 &&
               text[end] != '"' && text[end] != '\'' && text[end] != ')' && text[end] != ']') {
            ++end;
        }
        const std::string replacement = std::string(scheme) + "***:***@<redacted>";
        text.replace(pos, end - pos, replacement);
        pos += replacement.size();
    }
}

}  // namespace

void Logger::setLevel(LogLevel level) {
    gMinLevel.store(static_cast<int>(level), std::memory_order_relaxed);
}

void Logger::setLevelFromString(std::string_view levelName) {
    std::string upper(levelName);
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    if (upper == "TRACE") {
        setLevel(LogLevel::Trace);
    } else if (upper == "DEBUG") {
        setLevel(LogLevel::Debug);
    } else if (upper == "WARN") {
        setLevel(LogLevel::Warn);
    } else if (upper == "ERROR") {
        setLevel(LogLevel::Error);
    } else {
        setLevel(LogLevel::Info);
    }
}

LogLevel Logger::level() {
    return static_cast<LogLevel>(gMinLevel.load(std::memory_order_relaxed));
}

void Logger::trace(std::string_view message) { log(LogLevel::Trace, message); }
void Logger::debug(std::string_view message) { log(LogLevel::Debug, message); }
void Logger::info(std::string_view message)  { log(LogLevel::Info, message); }
void Logger::warn(std::string_view message)  { log(LogLevel::Warn, message); }
void Logger::error(std::string_view message) { log(LogLevel::Error, message); }

std::string Logger::sanitize(std::string_view message) {
    std::string safe(message);
    redactScheme(safe, "mongodb+srv://");
    redactScheme(safe, "mongodb://");
    return safe;
}

void Logger::log(LogLevel messageLevel, std::string_view message) {
    if (static_cast<int>(messageLevel) < gMinLevel.load(std::memory_order_relaxed)) {
        return;
    }

    const std::string line = "[" + currentUtcTimestamp() + "] [" +
                             levelToLabel(messageLevel) + "] " + sanitize(message);

    std::lock_guard<std::mutex> lock(gLogMutex);
    if (messageLevel == LogLevel::Error || messageLevel == LogLevel::Warn) {
        std::cerr << line << '\n';
    } else {
        std::cout << line << '\n';
    }
}

}  // namespace hotel::core