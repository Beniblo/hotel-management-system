#pragma once

#include <cstdint>
#include <string>

namespace hotel::config {

struct AppConfig {
    std::string mongodbUri;
    std::string mongodbDbName{"HotelDB"};
    std::uint32_t mongodbMaxPoolSize{16};
    std::uint32_t mongodbConnectTimeoutMs{5000};
    std::uint32_t mongodbServerSelectionTimeoutMs{5000};

    std::string serverHost{"127.0.0.1"};
    std::uint16_t serverPort{8080};

    std::string corsAllowedOrigin{"http://localhost:5173"};
    std::string logLevel{"INFO"};

    // Carga la configuracion dando prioridad a variables de entorno del SO
    // y usando el archivo .env local como respaldo. Falla rapido si falta un valor critico.
    static AppConfig load(const std::string& envFilePath = "");

    // Devuelve una version anonimizada de la URI para logs seguros (sin credenciales ni host).
    static std::string maskUri(const std::string& uri);
};

}  // namespace hotel::config