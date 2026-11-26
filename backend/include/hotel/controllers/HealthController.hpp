#pragma once

#include <functional>

#include <drogon/HttpController.h>

#include "hotel/database/MongoConnectionManager.hpp"

namespace hotel::controllers {

class HealthController : public drogon::HttpController<HealthController, false> {
public:
    explicit HealthController(hotel::database::MongoConnectionManager& dbManager);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::check, "/api/v1/health", drogon::Get);
    METHOD_LIST_END

    void check(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    hotel::database::MongoConnectionManager& dbManager_;
};

}  // namespace hotel::controllers