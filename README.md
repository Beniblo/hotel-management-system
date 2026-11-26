# Sistema de Gestión Hotelera (C++ / Drogon / MongoDB Atlas / Vue 3)

Aplicación web desacoplada para la administración operativa de un hotel (huéspedes, habitaciones, reservaciones, disponibilidad y reportes), desarrollada en entorno **Windows (x64)**.

## Arquitectura general

```text
Frontend (Vue 3 + TypeScript + Vite + Tailwind)
        │  HTTP / JSON (REST API)
        ▼
Backend (C++20 + Drogon + Servicios/Repositorios + mongocxx/bsoncxx)
        │  TLS / MongoDB Wire Protocol (privilegios mínimos)
        ▼
MongoDB Atlas (Base de datos: HotelDB)
        ▲
        │  Administración y verificación visual (NO es intermediario)
MongoDB Compass