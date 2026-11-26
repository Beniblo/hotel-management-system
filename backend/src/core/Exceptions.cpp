#include "hotel/core/Exceptions.hpp"

#include <utility>

namespace hotel::core {

AppException::AppException(int httpStatus,
                           std::string errorCode,
                           std::string publicMessage,
                           std::vector<FieldError> details)
    : std::runtime_error(publicMessage),
      httpStatus_(httpStatus),
      errorCode_(std::move(errorCode)),
      publicMessage_(std::move(publicMessage)),
      details_(std::move(details)) {}

int AppException::httpStatus() const noexcept {
    return httpStatus_;
}

const std::string& AppException::errorCode() const noexcept {
    return errorCode_;
}

const std::string& AppException::publicMessage() const noexcept {
    return publicMessage_;
}

const std::vector<FieldError>& AppException::details() const noexcept {
    return details_;
}

BadRequestException::BadRequestException(std::string publicMessage, std::string errorCode)
    : AppException(400, std::move(errorCode), std::move(publicMessage)) {}

ValidationException::ValidationException(std::string publicMessage,
                                         std::vector<FieldError> details,
                                         std::string errorCode)
    : AppException(422, std::move(errorCode), std::move(publicMessage), std::move(details)) {}

NotFoundException::NotFoundException(std::string publicMessage, std::string errorCode)
    : AppException(404, std::move(errorCode), std::move(publicMessage)) {}

ConflictException::ConflictException(std::string publicMessage, std::string errorCode)
    : AppException(409, std::move(errorCode), std::move(publicMessage)) {}

ForbiddenOperationException::ForbiddenOperationException(std::string publicMessage,
                                                         std::string errorCode)
    : AppException(403, std::move(errorCode), std::move(publicMessage)) {}

DatabaseAuthorizationException::DatabaseAuthorizationException(std::string publicMessage,
                                                               std::string errorCode)
    : AppException(403, std::move(errorCode), std::move(publicMessage)) {}

DatabaseUnavailableException::DatabaseUnavailableException(std::string publicMessage,
                                                           std::string errorCode)
    : AppException(503, std::move(errorCode), std::move(publicMessage)) {}

InternalServerException::InternalServerException(std::string publicMessage,
                                                 std::string errorCode)
    : AppException(500, std::move(errorCode), std::move(publicMessage)) {}

}  // namespace hotel::core