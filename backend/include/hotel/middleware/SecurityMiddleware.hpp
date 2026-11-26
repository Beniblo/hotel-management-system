#pragma once

#include <drogon/HttpAppFramework.h>

#include "hotel/config/AppConfig.hpp"

namespace hotel::middleware {

class SecurityMiddleware {
public:
    // Registra politicas globales de CORS estricto, Content-Type, anti-inyeccion NoSQL
    // y cabeceras de seguridad HTTP sobre la instancia de Drogon.
    static void registerAll(drogon::HttpAppFramework& app,
                            const hotel::config::AppConfig& config);
};

}  // namespace hotel::middleware