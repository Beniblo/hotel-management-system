#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace hotel::models {

struct Huesped {
    std::string id;
    std::string nombre;
    std::string correo;
    std::string telefono;
    std::chrono::system_clock::time_point fechaRegistro{};
    std::vector<std::string> preferencias;
    bool activo{true};
    std::chrono::system_clock::time_point actualizadoEn{};
};

}  // namespace hotel::models