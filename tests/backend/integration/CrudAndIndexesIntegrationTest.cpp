#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/AppContext.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/database/MongoConnectionManager.hpp"
#include "hotel/models/Enums.hpp"
#include "hotel/repositories/HuespedRepository.hpp"

namespace {

using hotel::config::AppConfig;
using hotel::core::AppContext;
using hotel::core::BadRequestException;
using hotel::core::ConflictException;
using hotel::core::DatabaseAuthorizationException;
using hotel::core::DatabaseUnavailableException;
using hotel::core::NotFoundException;
using hotel::database::MongoConnectionManager;
using hotel::models::EstadoHabitacion;
using hotel::repositories::HabitacionFilter;
using hotel::repositories::HuespedFilter;
using hotel::services::CreateHabitacionInput;
using hotel::services::CreateHuespedInput;
using hotel::services::UpdateHabitacionInput;
using hotel::services::UpdateHuespedInput;

std::string uniqueSuffix() {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    return std::to_string(ms);
}

std::optional<std::string> readEnvVar(const char* name) {
#ifdef _WIN32
    char* raw = nullptr;
    std::size_t len = 0;
    if (_dupenv_s(&raw, &len, name) != 0 || raw == nullptr) {
        return std::nullopt;
    }
    std::string value(raw);
    std::free(raw);
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
#else
    const char* raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0') {
        return std::nullopt;
    }
    return std::string(raw);
#endif
}

TEST(AtlasIntegrationTest, ConnectsAndPingsHotelDBSuccessfully) {
    const AppConfig config = AppConfig::load();
    AppContext context(config);

    EXPECT_TRUE(context.dbManager().ping());
    EXPECT_EQ(context.dbManager().databaseName(), "HotelDB");
}

TEST(AtlasIntegrationTest, HuespedCrudDuplicateEmailEmptyQueryAndDualDelete) {
    const AppConfig config = AppConfig::load();
    AppContext context(config);
    auto& service = context.huespedService();

    const std::string tag = uniqueSuffix();
    const std::string email = "it.huesped." + tag + "@ejemplo.com";
    std::string createdId;

    try {
        // 1. Alta (insert_one)
        CreateHuespedInput createIn;
        createIn.nombre = "Huesped Integracion " + tag;
        createIn.correo = "  " + email + "  ";
        createIn.telefono = "4771239900";
        createIn.preferencias = {"piso alto", "no fumar"};

        const auto created = service.crear(createIn);
        createdId = created.id;
        ASSERT_FALSE(createdId.empty());
        EXPECT_EQ(created.correo, email);
        EXPECT_TRUE(created.activo);

        // 2. Indice unico / duplicado (HTTP 409)
        EXPECT_THROW(static_cast<void>(service.crear(createIn)), ConflictException);

        // 3. Consulta individual y por filtro
        const auto fetched = service.obtenerPorId(createdId);
        EXPECT_EQ(fetched.nombre, createIn.nombre);

        HuespedFilter matchFilter;
        matchFilter.correo = email;
        const auto matchPage = service.listar(matchFilter);
        EXPECT_EQ(matchPage.total, 1);

        // 4. Resultado vacio (200 con [])
        HuespedFilter emptyFilter;
        emptyFilter.correo = "inexistente.total." + tag + "@ejemplo.com";
        const auto emptyPage = service.listar(emptyFilter);
        EXPECT_EQ(emptyPage.total, 0);
        EXPECT_TRUE(emptyPage.items.empty());

        // 5. ID mal formado (400) e ID inexistente (404)
        EXPECT_THROW(static_cast<void>(service.obtenerPorId("id-no-hex")), BadRequestException);
        EXPECT_THROW(static_cast<void>(service.obtenerPorId("65a000000000000000000099")),
                     NotFoundException);

        // 6. Actualizacion parcial (update_one)
        UpdateHuespedInput patchIn;
        patchIn.telefono = "4779998877";
        const auto updated = service.actualizar(createdId, patchIn);
        EXPECT_EQ(updated.telefono, "4779998877");

        // 7. Baja logica por defecto (activo = false)
        const auto softDelete = service.eliminar(createdId, false);
        EXPECT_FALSE(softDelete.eliminacionFisica);
        ASSERT_TRUE(softDelete.huespedDesactivado.has_value());
        EXPECT_FALSE(softDelete.huespedDesactivado->activo);

        // 8. Eliminacion fisica controlada (delete_one)
        const auto hardDelete = service.eliminar(createdId, true);
        EXPECT_TRUE(hardDelete.eliminacionFisica);
        createdId.clear();

        EXPECT_THROW(static_cast<void>(service.obtenerPorId(created.id)), NotFoundException);
    } catch (...) {
        if (!createdId.empty()) {
            try {
                static_cast<void>(service.eliminar(createdId, true));
            } catch (...) {
            }
        }
        throw;
    }
}

TEST(AtlasIntegrationTest, HabitacionCrudDuplicateNumberAndStateTransition) {
    const AppConfig config = AppConfig::load();
    AppContext context(config);
    auto& service = context.habitacionService();

    const std::string tag = uniqueSuffix();
    const std::string roomNum = "IT-" + tag.substr(tag.size() > 6 ? tag.size() - 6 : 0);
    std::string createdId;

    try {
        // 1. Alta
        CreateHabitacionInput createIn;
        createIn.numero = roomNum;
        createIn.tipo = "doble";
        createIn.capacidad = 2;
        createIn.tarifa = 1750.0;
        createIn.servicios = {"wifi", "tv"};

        const auto created = service.crear(createIn);
        createdId = created.id;
        ASSERT_FALSE(createdId.empty());
        EXPECT_EQ(created.estado, EstadoHabitacion::Disponible);

        // 2. Rechazo de numero duplicado (HTTP 409)
        EXPECT_THROW(static_cast<void>(service.crear(createIn)), ConflictException);

        // 3. Cambio rapido de estado
        const auto inMaintenance =
            service.cambiarEstado(createdId, EstadoHabitacion::Mantenimiento);
        EXPECT_EQ(inMaintenance.estado, EstadoHabitacion::Mantenimiento);

        // 4. Actualizacion parcial
        UpdateHabitacionInput patchIn;
        patchIn.tarifa = 1890.0;
        patchIn.estado = EstadoHabitacion::Disponible;
        const auto updated = service.actualizar(createdId, patchIn);
        EXPECT_DOUBLE_EQ(updated.tarifa, 1890.0);
        EXPECT_EQ(updated.estado, EstadoHabitacion::Disponible);

        // 5. Consulta por filtro
        HabitacionFilter filter;
        filter.tipo = "doble";
        filter.capacidadMin = 2;
        const auto listed = service.listar(filter);
        EXPECT_GE(listed.total, 1);

        // 6. Eliminacion fisica controlada
        service.eliminar(createdId);
        createdId.clear();

        EXPECT_THROW(static_cast<void>(service.obtenerPorId(created.id)), NotFoundException);
    } catch (...) {
        if (!createdId.empty()) {
            try {
                service.eliminar(createdId);
            } catch (...) {
            }
        }
        throw;
    }
}

TEST(AtlasIntegrationTest, UnreachableUriThrowsDatabaseUnavailable503) {
    AppConfig badCfg = AppConfig::load();
    badCfg.mongodbUri =
        "mongodb://127.0.0.1:27099/?serverSelectionTimeoutMS=1000&connectTimeoutMS=1000";

    MongoConnectionManager badManager(badCfg);
    EXPECT_THROW(static_cast<void>(badManager.ping()), DatabaseUnavailableException);
}

TEST(AtlasIntegrationTest, ReadOnlyUserCannotWriteThrowsForbidden403) {
    const auto readonlyUri = readEnvVar("MONGODB_READONLY_URI");
    if (!readonlyUri.has_value()) {
        GTEST_SKIP()
            << "Define $env:MONGODB_READONLY_URI con las credenciales de hotel_readonly_user "
               "para ejecutar esta prueba automatizada de permisos insuficientes (HTTP 403).";
    }

    AppConfig roCfg = AppConfig::load();
    roCfg.mongodbUri = *readonlyUri;

    MongoConnectionManager roManager(roCfg);
    hotel::repositories::HuespedRepository roRepo(roManager);

    // Lectura permitida con rol read
    EXPECT_NO_THROW(static_cast<void>(roRepo.findAll(HuespedFilter{})));

    // Escritura bloqueada por Atlas (codigo 13 -> HTTP 403)
    hotel::models::Huesped forbidden;
    forbidden.nombre = "Intento Solo Lectura";
    forbidden.correo = "readonly.test." + uniqueSuffix() + "@ejemplo.com";
    forbidden.telefono = "4770000099";
    forbidden.activo = true;
    forbidden.fechaRegistro = std::chrono::system_clock::now();
    forbidden.actualizadoEn = forbidden.fechaRegistro;

    EXPECT_THROW(static_cast<void>(roRepo.create(forbidden)), DatabaseAuthorizationException);
}

}  // namespace