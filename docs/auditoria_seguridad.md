# Auditoría de Seguridad e Integración Integral (Fase 7)

## 1. Control de Accesos y Privilegios Mínimos en MongoDB Atlas
- **Separación de cuentas:**
  - Cuenta administrativa: uso exclusivo en Atlas UI y MongoDB Compass (nunca configurada en el backend C++).
  - `hotel_app_user`: rol `readWrite` acotado estrictamente a la base `HotelDB`.
  - `hotel_readonly_user`: rol `read` acotado estrictamente a `HotelDB` para verificación de permisos mínimos.
- **Restricción de red (IP Allowlist):** acceso limitado exclusivamente a la dirección IP pública del entorno de desarrollo (regla `0.0.0.0/0` deshabilitada).
- **Prueba empírica de permisos insuficientes:**
  - Con `hotel_readonly_user`, `GET /api/v1/huespedes` responde `HTTP 200 OK`.
  - Con `hotel_readonly_user`, `POST /api/v1/huespedes` es rechazado por MongoDB Atlas (código `13` / Unauthorized) y traducido por `RepositoryCommon` + `ErrorMapper` a `HTTP 403 Forbidden` (`errorCode: "DB_UNAUTHORIZED"`) sin caídas del servidor ni exposición de secretos.

## 2. Hardening de la API REST (Drogon) — Lista de Verificación OWASP API
| Control | Implementación en Backend C++ | Verificación |
| :--- | :--- | :--- |
| **CORS estricto** | `SecurityMiddleware` valida `Origin` contra `CORS_ALLOWED_ORIGIN` (`http://localhost:5173`) y responde `OPTIONS` con `204`. | `Origin` permitido → `204` con `ACAO`; origen externo → `403 CORS_ORIGIN_DENIED`. |
| **Content-Type obligatorio** | Rechaza escrituras (`POST`/`PATCH`/`PUT`) sin `application/json` (excepto `/cancelar` sin cuerpo). | `text/plain` en `POST` → `415 UNSUPPORTED_MEDIA_TYPE`. |
| **Anti-inyección NoSQL (API8)** | Filtros BSON tipados (`bsoncxx::builder::basic`), escape regex (`escapeRegex`) y bloqueo de claves `$` en `SecurityMiddleware` + `JsonHelpers`. | Payload con `{"$ne": ""}` → `400 NOSQL_INJECTION_BLOCKED`. |
| **Control BOPLA (API3)** | DTOs aceptan solo campos permitidos; `_id`, `fechaRegistro`, `tarifaAplicada` y `total` se calculan únicamente en servidor. | Campos inyectados en body son ignorados. |
| **Límites de consumo (API4)** | `setClientMaxBodySize(1 MB)`, paginación acotada (`1 <= limit <= 100`) y rango de reportes acotado a `366` días. | Rango > 366 días → `422 VALIDATION_ERROR`. |
| **Cabeceras HTTP seguras** | `X-Content-Type-Options: nosniff`, `X-Frame-Options: DENY`, `Referrer-Policy: no-referrer`, `Cache-Control: no-store`. | Verificadas en respuesta HTTP. |
| **Logging sanitizado y secretos** | `Logger::sanitize` redacta automáticamente cualquier `mongodb://` o `mongodb+srv://`; `.env` excluido vía `.gitignore`. | `git ls-files` y `git grep` confirman cero secretos en el repositorio. |