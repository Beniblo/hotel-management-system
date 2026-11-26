#pragma once

#include <functional>

#include <drogon/HttpController.h>

#include "hotel/services/DisponibilidadService.hpp"

namespace hotel::controllers {

class DisponibilidadController : public drogon::HttpController<DisponibilidadController, false> {
public:
    explicit DisponibilidadController(hotel::services::DisponibilidadService& disponibilidadService);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DisponibilidadController::consultar, "/api/v1/disponibilidad", drogon::Get);
    METHOD_LIST_END

    void consultar(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    hotel::services::DisponibilidadService& disponibilidadService_;
};

}  // namespace hotel::controllers