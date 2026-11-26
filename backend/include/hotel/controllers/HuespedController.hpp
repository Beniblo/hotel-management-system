#pragma once

#include <functional>
#include <string>

#include <drogon/HttpController.h>

#include "hotel/services/HuespedService.hpp"

namespace hotel::controllers {

class HuespedController : public drogon::HttpController<HuespedController, false> {
public:
    explicit HuespedController(hotel::services::HuespedService& huespedService);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HuespedController::list, "/api/v1/huespedes", drogon::Get);
    ADD_METHOD_TO(HuespedController::create, "/api/v1/huespedes", drogon::Post);
    ADD_METHOD_TO(HuespedController::getById, "/api/v1/huespedes/{id}", drogon::Get);
    ADD_METHOD_TO(HuespedController::update, "/api/v1/huespedes/{id}", drogon::Patch);
    ADD_METHOD_TO(HuespedController::remove, "/api/v1/huespedes/{id}", drogon::Delete);
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

    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string id);

private:
    hotel::services::HuespedService& huespedService_;
};

}  // namespace hotel::controllers