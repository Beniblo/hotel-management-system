cmake_minimum_required(VERSION 3.21)

project(hotel_backend
    VERSION 1.0.0
    LANGUAGES CXX
)

# Estandar C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opciones de compilacion para MSVC en Windows
if(MSVC)
    add_compile_options(/W4 /utf-8 /EHsc /Zc:__cplusplus)
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00
    )
endif()

# Dependencias gestionadas por vcpkg
find_package(Drogon CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Libreria interna del backend (reutilizable en hotel_api y pruebas GoogleTest)
set(HOTEL_CORE_SOURCES
    # Configuracion y nucleo
    src/config/AppConfig.cpp
    src/core/Exceptions.cpp
    src/core/Logger.cpp
    src/core/ErrorMapper.cpp
    src/core/AppContext.cpp

    # Base de datos (mongocxx::instance + mongocxx::pool)
    src/database/MongoConnectionManager.cpp

    # Dominio y validadores
    src/models/Enums.cpp
    src/validators/InputValidators.cpp

    # Repositorios BSON
    src/repositories/RepositoryCommon.cpp
    src/repositories/HuespedRepository.cpp
    src/repositories/HabitacionRepository.cpp
    src/repositories/ReservacionRepository.cpp

    # Servicios de negocio
    src/services/HuespedService.cpp
    src/services/HabitacionService.cpp
    src/services/RoomLockManager.cpp
    src/services/ReservacionService.cpp
    src/services/DisponibilidadService.cpp
    src/services/ReporteService.cpp

    # DTOs y mappers JSON
    src/dto/JsonHelpers.cpp
    src/dto/DtoMappers.cpp

    # Middleware y seguridad HTTP
    src/middleware/SecurityMiddleware.cpp

    # Controladores REST Drogon
    src/controllers/HealthController.cpp
    src/controllers/HuespedController.cpp
    src/controllers/HabitacionController.cpp
    src/controllers/ReservacionController.cpp
    src/controllers/DisponibilidadController.cpp
    src/controllers/ReporteController.cpp
)

add_library(hotel_core STATIC ${HOTEL_CORE_SOURCES})

target_include_directories(hotel_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hotel_core
    PUBLIC
        Drogon::Drogon
        mongo::bsoncxx_shared
        mongo::mongocxx_shared
)

# Ejecutable principal de la API REST
add_executable(hotel_api
    src/main.cpp
)

target_link_libraries(hotel_api
    PRIVATE
        hotel_core
)

# Pruebas automatizadas con GoogleTest + CTest
enable_testing()
include(GoogleTest)

set(TESTS_UNIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tests/backend/unit")

add_executable(hotel_unit_tests
    ${TESTS_UNIT_DIR}/ValidatorsTest.cpp
    ${TESTS_UNIT_DIR}/OverlapsAndTotalsTest.cpp
    ${TESTS_UNIT_DIR}/ExceptionsAndDtoTest.cpp
)

target_link_libraries(hotel_unit_tests
    PRIVATE
        hotel_core
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(hotel_unit_tests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES LABELS "unit"
)