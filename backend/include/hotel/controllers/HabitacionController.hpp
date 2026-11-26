#pragma once

#include <functional>
#include <string>

#include <drogon/HttpController.h>

#include "hotel/services/HabitacionService.hpp"

namespace hotel::controllers {

class HabitacionController : public drogon::HttpController<HabitacionController, false> {
public:
    explicit HabitacionController(hotel::services::HabitacionService& habitacionService);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HabitacionController::list, "/api/v1/habitaciones", drogon::Get);
    ADD_METHOD_TO(HabitacionController::create, "/api/v1/habitaciones", drogon::Post);
    ADD_METHOD_TO(HabitacionController::getById, "/api/v1/habitaciones/{id}", drogon::Get);
    ADD_METHOD_TO(HabitacionController::update, "/api/v1/habitaciones/{id}", drogon::Patch);
    ADD_METHOD_TO(HabitacionController::updateEstado,
                  "/api/v1/habitaciones/{id}/estado",
                  drogon::Patch);
    ADD_METHOD_TO(HabitacionController::remove, "/api/v1/habitaciones/{id}", drogon::Delete);
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

    void updateEstado(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                      std::string id);

    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string id);

private:
    hotel::services::HabitacionService& habitacionService_;
};

}  // namespace hotel::controllers