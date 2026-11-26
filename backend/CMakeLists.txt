cmake_minimum_required(VERSION 3.21)

project(hotel_backend
    VERSION 0.1.0
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
add_library(hotel_core STATIC
    src/config/AppConfig.cpp
    src/core/Exceptions.cpp
    src/core/Logger.cpp
    src/database/MongoConnectionManager.cpp
    src/models/Enums.cpp
    src/validators/InputValidators.cpp
)

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

# Ejecutable principal
add_executable(hotel_api
    src/main.cpp
)

target_link_libraries(hotel_api
    PRIVATE
        hotel_core
)

enable_testing()