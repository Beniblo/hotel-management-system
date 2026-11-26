#pragma once

#include <functional>
#include <string>

#include <drogon/HttpController.h>

#include "hotel/services/ReservacionService.hpp"

namespace hotel::controllers {

class ReservacionController : public drogon::HttpController<ReservacionController, false> {
public:
    explicit ReservacionController(hotel::services::ReservacionService& reservacionService);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReservacionController::list, "/api/v1/reservaciones", drogon::Get);
    ADD_METHOD_TO(ReservacionController::create, "/api/v1/reservaciones", drogon::Post);
    ADD_METHOD_TO(ReservacionController::getById, "/api/v1/reservaciones/{id}", drogon::Get);
    ADD_METHOD_TO(ReservacionController::update, "/api/v1/reservaciones/{id}", drogon::Patch);
    ADD_METHOD_TO(ReservacionController::cancel,
                  "/api/v1/reservaciones/{id}/cancelar",
                  drogon::Post);
    ADD_METHOD_TO(ReservacionController::remove, "/api/v1/reservaciones/{id}", drogon::Delete);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void create(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getById(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 std::string id);

    void update(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string id);

    void cancel(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string id);

    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string id);

private:
    hotel::services::ReservacionService& reservacionService_;
};

}  // namespace hotel::controllers