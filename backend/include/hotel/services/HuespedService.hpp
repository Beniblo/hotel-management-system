#pragma once

#include <optional>
#include <string>
#include <vector>

#include "hotel/models/Huesped.hpp"
#include "hotel/repositories/HuespedRepository.hpp"
#include "hotel/repositories/ReservacionRepository.hpp"

namespace hotel::services {

struct CreateHuespedInput {
    std::string nombre;
    std::string correo;
    std::string telefono;
    std::vector<std::string> preferencias;
};

struct UpdateHuespedInput {
    std::optional<std::string> nombre;
    std::optional<std::string> correo;
    std::optional<std::string> telefono;
    std::optional<std::vector<std::string>> preferencias;
    std::optional<bool> activo;

    [[nodiscard]] bool hasAnyField() const noexcept {
        return nombre.has_value() || correo.has_value() || telefono.has_value() ||
               preferencias.has_value() || activo.has_value();
    }
};

struct DeleteHuespedResult {
    bool eliminacionFisica{false};
    std::optional<hotel::models::Huesped> huespedDesactivado;
};

class HuespedService {
public:
    HuespedService(hotel::repositories::HuespedRepository& huespedRepository,
                   hotel::repositories::ReservacionRepository& reservacionRepository);

    [[nodiscard]] hotel::models::Huesped crear(const CreateHuespedInput& input);
    [[nodiscard]] hotel::repositories::PagedResult<hotel::models::Huesped> listar(
        const hotel::repositories::HuespedFilter& filter);
    [[nodiscard]] hotel::models::Huesped obtenerPorId(const std::string& id);
    [[nodiscard]] hotel::models::Huesped actualizar(const std::string& id,
                                                    const UpdateHuespedInput& input);

    // Baja logica por defecto (fisico = false) o eliminacion fisica controlada (fisico = true).
    [[nodiscard]] DeleteHuespedResult eliminar(const std::string& id, bool fisico = false);

private:
    hotel::repositories::HuespedRepository& huespedRepo_;
    hotel::repositories::ReservacionRepository& reservacionRepo_;
};

}  // namespace hotel::services