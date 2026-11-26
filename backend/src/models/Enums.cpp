#include "hotel/models/Enums.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace hotel::models {

namespace {

std::string normalizeToken(std::string_view raw) {
    std::size_t start = 0;
    while (start < raw.size() && std::isspace(static_cast<unsigned char>(raw[start])) != 0) {
        ++start;
    }
    std::size_t end = raw.size();
    while (end > start && std::isspace(static_cast<unsigned char>(raw[end - 1])) != 0) {
        --end;
    }

    std::string out(raw.substr(start, end - start));
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

}  // namespace

std::string toString(EstadoHabitacion estado) {
    switch (estado) {
        case EstadoHabitacion::Disponible:      return "disponible";
        case EstadoHabitacion::Ocupada:         return "ocupada";
        case EstadoHabitacion::Mantenimiento:   return "mantenimiento";
        case EstadoHabitacion::FueraDeServicio: return "fuera_de_servicio";
    }
    return "disponible";
}

std::optional<EstadoHabitacion> parseEstadoHabitacion(std::string_view value) {
    const std::string token = normalizeToken(value);
    if (token == "disponible")        return EstadoHabitacion::Disponible;
    if (token == "ocupada")           return EstadoHabitacion::Ocupada;
    if (token == "mantenimiento")     return EstadoHabitacion::Mantenimiento;
    if (token == "fuera_de_servicio") return EstadoHabitacion::FueraDeServicio;
    return std::nullopt;
}

std::string toString(EstadoReservacion estado) {
    switch (estado) {
        case EstadoReservacion::Pendiente:  return "pendiente";
        case EstadoReservacion::Confirmada: return "confirmada";
        case EstadoReservacion::EnCurso:    return "en_curso";
        case EstadoReservacion::Completada: return "completada";
        case EstadoReservacion::Cancelada:  return "cancelada";
    }
    return "pendiente";
}

std::optional<EstadoReservacion> parseEstadoReservacion(std::string_view value) {
    const std::string token = normalizeToken(value);
    if (token == "pendiente")  return EstadoReservacion::Pendiente;
    if (token == "confirmada") return EstadoReservacion::Confirmada;
    if (token == "en_curso")   return EstadoReservacion::EnCurso;
    if (token == "completada") return EstadoReservacion::Completada;
    if (token == "cancelada")  return EstadoReservacion::Cancelada;
    return std::nullopt;
}

bool isBlockingReservationStatus(EstadoReservacion estado) noexcept {
    return estado == EstadoReservacion::Pendiente ||
           estado == EstadoReservacion::Confirmada ||
           estado == EstadoReservacion::EnCurso;
}

}  // namespace hotel::models