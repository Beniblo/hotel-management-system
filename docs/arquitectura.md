# Arquitectura del Sistema de Gestión Hotelera

## 1. Visión general

El sistema implementa una arquitectura web cliente-servidor de capas desacopladas en entorno **Windows (x64)**. El navegador web consume exclusivamente la API REST escrita en **C++20 con Drogon**, la cual concentra la validación, las reglas de negocio y la comunicación directa con **MongoDB Atlas** mediante el controlador oficial **`mongocxx` / `bsoncxx`**.

**Regla arquitectónica estricta:**
- El frontend **nunca** se conecta directamente a MongoDB Atlas ni almacena credenciales de base de datos.
- **MongoDB Compass** se utiliza exclusivamente como herramienta gráfica externa de administración, exploración y verificación visual de `HotelDB`; no forma parte del flujo de ejecución de la aplicación.

---

## 2. Diagrama de arquitectura general

```text
+-------------------------------------------------------------------+
|                     CLIENTE WEB (Navegador)                       |
|      SPA: HTML5 + Vue 3 + TypeScript + Vite + Tailwind CSS        |
|   [Views] ---> [Composables / Pinia] ---> [Services / apiClient]  |
+---------------------------------+---------------------------------+
                                  | HTTP / JSON (REST API + CORS)
                                  v
+-------------------------------------------------------------------+
|              BACKEND API REST (C++20 / Drogon - Win64)            |
|  +-------------------------------------------------------------+  |
|  | Capa 1: Middleware / Filtros HTTP                           |  |
|  |         (CORS estricto, Content-Type, Limites, Request-ID)  |  |
|  +------------------------------+------------------------------+  |
|                                 v                                 |
|  +-------------------------------------------------------------+  |
|  | Capa 2: Controladores REST + DTOs + ErrorMapper             |  |
|  |         (Deserializacion JSON, respuestas 200-503)          |  |
|  +------------------------------+------------------------------+  |
|                                 v                                 |
|  +-------------------------------------------------------------+  |
|  | Capa 3: Servicios de Negocio + Validadores + RoomLockManager|  |
|  |         (Traslapes, calculo de totales, baja logica/fisica) |  |
|  +------------------------------+------------------------------+  |
|                                 v                                 |
|  +-------------------------------------------------------------+  |
|  | Capa 4: Repositorios (Dominio <-> BSON via bsoncxx/mongocxx)|  |
|  |         (insert_one, find, update_one, delete_one)          |  |
|  +------------------------------+------------------------------+  |
|                                 v                                 |
|  +-------------------------------------------------------------+  |
|  | Capa 5: MongoConnectionManager                              |  |
|  |         (mongocxx::instance unico + mongocxx::pool)         |  |
|  +------------------------------+------------------------------+  |
+---------------------------------+---------------------------------+
                                  | TLS / MongoDB Wire Protocol
                                  | Usuario dedicado: readWrite en HotelDB
                                  v
+---------------------------------+-----------------+     +-------------------------------+
|                MONGODB ATLAS (Nube)               |<----|        MONGODB COMPASS        |
|        Cluster con IP Allowlist restringida       | TLS |  Administracion y verificacion|
|  Base de datos: HotelDB                           |     |  Cuenta Admin independiente.  |
|   |-- huespedes      (idx: correo unique)         |     |  NO es intermediario de la    |
|   |-- habitaciones   (idx: numero unique)         |     |  aplicacion.                  |
|   `-- reservaciones  (idx: habitacionId+fechas)   |     +-------------------------------+
+---------------------------------------------------+