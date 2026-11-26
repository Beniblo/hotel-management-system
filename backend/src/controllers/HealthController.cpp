#include "hotel/controllers/HealthController.hpp"

#include <string>

#include "hotel/core/ErrorMapper.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/dto/DtoMappers.hpp"

namespace hotel::controllers {

HealthController::HealthController(hotel::database::MongoConnectionManager& dbManager)
    : dbManager_(dbManager) {}

void HealthController::check(
    const drogon::HttpRequestPtr& /*req*/,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const std::string requestId = hotel::core::ErrorMapper::generateRequestId();

    try {
        const bool connected = dbManager_.ping();
        const Json::Value body =
            hotel::dto::buildHealthJson(connected, dbManager_.databaseName());

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(connected ? drogon::k200OK : drogon::k503ServiceUnavailable);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (const hotel::core::AppException&) {
        const Json::Value body = hotel::dto::buildHealthJson(false, dbManager_.databaseName());
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(drogon::k503ServiceUnavailable);
        response->addHeader("X-Request-Id", requestId);
        callback(response);
    } catch (...) {
        callback(hotel::core::ErrorMapper::fromCurrentException(requestId));
    }
}

}  // namespace hotel::controllers