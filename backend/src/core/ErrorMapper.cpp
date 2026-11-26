#include "hotel/core/ErrorMapper.hpp"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "hotel/core/Logger.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::core {

namespace {

std::atomic<std::uint64_t> gRequestCounter{1};

drogon::HttpStatusCode toDrogonStatus(int status) {
    switch (status) {
        case 400: return drogon::k400BadRequest;
        case 401: return drogon::k401Unauthorized;
        case 403: return drogon::k403Forbidden;
        case 404: return drogon::k404NotFound;
        case 409: return drogon::k409Conflict;
        case 413: return drogon::k413RequestEntityTooLarge;
        case 415: return drogon::k415UnsupportedMediaType;
        case 422: return drogon::k422UnprocessableEntity;
        case 503: return drogon::k503ServiceUnavailable;
        default:  return drogon::k500InternalServerError;
    }
}

}  // namespace

std::string ErrorMapper::generateRequestId() {
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    const auto seq = gRequestCounter.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream oss;
    oss << "req-" << std::hex << nowMs << '-' << std::setw(4) << std::setfill('0') << seq;
    return oss.str();
}

Json::Value ErrorMapper::buildErrorJson(const AppException& ex, const std::string& requestId) {
    Json::Value body(Json::objectValue);
    body["timestamp"] = hotel::validators::formatUtcTimestamp(std::chrono::system_clock::now());
    body["status"] = ex.httpStatus();
    body["errorCode"] = ex.errorCode();
    body["message"] = ex.publicMessage();
    body["requestId"] = requestId.empty() ? generateRequestId() : requestId;

    if (!ex.details().empty()) {
        Json::Value detailsArr(Json::arrayValue);
        for (const auto& item : ex.details()) {
            Json::Value detailObj(Json::objectValue);
            detailObj["field"] = item.field;
            detailObj["message"] = item.message;
            detailsArr.append(std::move(detailObj));
        }
        body["details"] = std::move(detailsArr);
    }

    return body;
}

drogon::HttpResponsePtr ErrorMapper::toHttpResponse(const AppException& ex,
                                                    const std::string& requestId) {
    const std::string effectiveReqId = requestId.empty() ? generateRequestId() : requestId;
    const Json::Value body = buildErrorJson(ex, effectiveReqId);

    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(toDrogonStatus(ex.httpStatus()));
    response->addHeader("X-Request-Id", effectiveReqId);
    return response;
}

drogon::HttpResponsePtr ErrorMapper::fromCurrentException(const std::string& requestId) {
    const std::string effectiveReqId = requestId.empty() ? generateRequestId() : requestId;

    try {
        throw;
    } catch (const AppException& ex) {
        if (ex.httpStatus() >= 500) {
            Logger::error("[" + effectiveReqId + "] Error HTTP " +
                          std::to_string(ex.httpStatus()) + " [" + ex.errorCode() + "]: " +
                          ex.publicMessage());
        } else {
            Logger::warn("[" + effectiveReqId + "] Solicitud rechazada HTTP " +
                         std::to_string(ex.httpStatus()) + " [" + ex.errorCode() + "]: " +
                         ex.publicMessage());
        }
        return toHttpResponse(ex, effectiveReqId);
    } catch (const std::exception& ex) {
        Logger::error("[" + effectiveReqId + "] Excepcion no controlada: " + ex.what());
        const InternalServerException safeEx;
        return toHttpResponse(safeEx, effectiveReqId);
    } catch (...) {
        Logger::error("[" + effectiveReqId + "] Excepcion desconocida no controlada.");
        const InternalServerException safeEx;
        return toHttpResponse(safeEx, effectiveReqId);
    }
}

}  // namespace hotel::core