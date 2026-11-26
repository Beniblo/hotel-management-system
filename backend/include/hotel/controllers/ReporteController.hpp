#pragma once

#include <functional>

#include <drogon/HttpController.h>

#include "hotel/services/ReporteService.hpp"

namespace hotel::controllers {

class ReporteController : public drogon::HttpController<ReporteController, false> {
public:
    explicit ReporteController(hotel::services::ReporteService& reporteService);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReporteController::resumen, "/api/v1/reportes/resumen", drogon::Get);
    METHOD_LIST_END

    void resumen(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    hotel::services::ReporteService& reporteService_;
};

}  // namespace hotel::controllers