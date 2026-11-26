cmake_minimum_required(VERSION 3.21)

project(hotel_backend
    VERSION 0.1.0
    LANGUAGES CXX
)

# Estandar C++20 (con respaldo previsto a C++17 si fuera necesario)
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

# Dependencias gestionadas automaticamente por vcpkg
find_package(Drogon CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Ejecutable principal (smoke build inicial en Fase 2A)
add_executable(hotel_api
    src/main.cpp
)

target_include_directories(hotel_api
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hotel_api
    PRIVATE
        Drogon::Drogon
        mongo::bsoncxx_shared
        mongo::mongocxx_shared
)

enable_testing()