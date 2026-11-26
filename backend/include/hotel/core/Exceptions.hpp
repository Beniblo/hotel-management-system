#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace hotel::core {

struct FieldError {
    std::string field;
    std::string message;
};

class AppException : public std::runtime_error {
public:
    AppException(int httpStatus,
                 std::string errorCode,
                 std::string publicMessage,
                 std::vector<FieldError> details = {});

    ~AppException() override = default;

    [[nodiscard]] int httpStatus() const noexcept;
    [[nodiscard]] const std::string& errorCode() const noexcept;
    [[nodiscard]] const std::string& publicMessage() const noexcept;
    [[nodiscard]] const std::vector<FieldError>& details() const noexcept;

private:
    int httpStatus_;
    std::string errorCode_;
    std::string publicMessage_;
    std::vector<FieldError> details_;
};

class BadRequestException final : public AppException {
public:
    explicit BadRequestException(std::string publicMessage,
                                 std::string errorCode = "BAD_REQUEST");
};

class ValidationException final : public AppException {
public:
    explicit ValidationException(std::string publicMessage,
                                 std::vector<FieldError> details = {},
                                 std::string errorCode = "VALIDATION_ERROR");
};

class NotFoundException final : public AppException {
public:
    explicit NotFoundException(std::string publicMessage,
                               std::string errorCode = "RESOURCE_NOT_FOUND");
};

class ConflictException final : public AppException {
public:
    explicit ConflictException(std::string publicMessage,
                               std::string errorCode = "CONFLICT");
};

class ForbiddenOperationException final : public AppException {
public:
    explicit ForbiddenOperationException(std::string publicMessage,
                                         std::string errorCode = "FORBIDDEN");
};

class DatabaseAuthorizationException final : public AppException {
public:
    explicit DatabaseAuthorizationException(
        std::string publicMessage = "Permisos insuficientes para ejecutar la operacion en base de datos.",
        std::string errorCode = "DB_UNAUTHORIZED");
};

class DatabaseUnavailableException final : public AppException {
public:
    explicit DatabaseUnavailableException(
        std::string publicMessage = "El servicio de base de datos no esta disponible temporalmente.",
        std::string errorCode = "DB_UNAVAILABLE");
};

class InternalServerException final : public AppException {
public:
    explicit InternalServerException(
        std::string publicMessage = "Ocurrio un error interno inesperado.",
        std::string errorCode = "INTERNAL_SERVER_ERROR");
};

}  // namespace hotel::core