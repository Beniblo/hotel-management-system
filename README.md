# Sistema de Gestión Hotelera

**C++20 · Drogon Framework · MongoDB Atlas · Vue 3 · TypeScript**

Sistema web desacoplado para la administración operativa de un hotel, desarrollado en **Windows x64**. Permite gestionar huéspedes, habitaciones, reservaciones, disponibilidad y reportes administrativos mediante una API REST desarrollada en **C++20 con Drogon Framework**, conectada directamente a **MongoDB Atlas (`HotelDB`)** mediante el controlador oficial de MongoDB para C++ (`mongocxx` / `bsoncxx`).

El sistema incorpora un panel administrativo desarrollado con **Vue 3, TypeScript, Vite y Tailwind CSS**, que consume los servicios del backend mediante HTTP y JSON.

---

## Tabla de contenidos

- [Arquitectura general](#arquitectura-general)
- [Stack tecnológico](#stack-tecnológico)
- [Funcionalidades principales](#funcionalidades-principales)
- [Estructura del repositorio](#estructura-del-repositorio)
- [Requisitos previos](#requisitos-previos)
- [Inicio rápido](#inicio-rápido)
- [Ejecución de pruebas](#ejecución-de-pruebas)
- [Seguridad y privilegios mínimos](#seguridad-y-privilegios-mínimos)
- [Documentación](#documentación)

---

## Arquitectura general

El sistema utiliza una arquitectura web desacoplada, separando las responsabilidades del frontend, backend y almacenamiento de datos.

```text
                 NAVEGADOR WEB
                       |
                       |
          Vue 3 + TypeScript + Pinia
                Tailwind CSS
                  Vite :5173
                       |
                       | HTTP / JSON
                       | REST API v1
                       | CORS estricto
                       |
                       v
              BACKEND C++20 / DROGON
                    :8080
                       |
       +---------------+----------------+
       |               |                |
       v               v                v
 SecurityMiddleware Controllers      Services
       |             + DTOs          + Validators
       |          + ErrorMapper    + RoomLockManager
       |               |                |
       +---------------+----------------+
                       |
                       v
                   Repositories
                    BSON tipado
                       |
                       v
              MongoConnectionManager
                 mongocxx::instance
                  mongocxx::pool
                       |
                       | TLS
                       | MongoDB Wire Protocol
                       |
                       v
                 MONGODB ATLAS
                     HotelDB
                       |
          +------------+------------+
          |            |            |
          v            v            v
      huespedes   habitaciones  reservaciones

                 MONGODB COMPASS
             Administración y verificación
```

### Descripción de las capas

| Componente | Responsabilidad |
|---|---|
| SecurityMiddleware | Controles de CORS, Content-Type, cabeceras HTTP y protección de solicitudes. |
| Controllers | Recepción de solicitudes HTTP y generación de respuestas. |
| DTOs | Estructuración y transferencia de información entre componentes. |
| ErrorMapper | Estandarización del manejo de errores y respuestas de la API. |
| Services | Implementación de reglas de negocio y coordinación de operaciones. |
| Validators | Validación de datos y restricciones de entrada. |
| RoomLockManager | Coordinación de operaciones concurrentes relacionadas con reservaciones por habitación. |
| Repositories | Acceso a MongoDB mediante operaciones BSON tipadas. |
| MongoConnectionManager | Administración de `mongocxx::instance` y del pool de conexiones. |

**MongoDB Compass** se utiliza exclusivamente para administración y verificación visual de la base de datos. No participa en el flujo operativo de comunicación entre el frontend y el backend.

---

## Stack tecnológico

| Capa | Tecnologías |
|---|---|
| Lenguaje Backend | C++20 |
| Framework Backend | Drogon Framework |
| Compilador | MSVC x64 |
| IDE / Toolchain | Visual Studio 2022 |
| Sistema de compilación | CMake + Ninja |
| Gestión de dependencias | vcpkg |
| Driver de MongoDB | mongo-cxx-driver (`mongocxx` / `bsoncxx`) |
| Serialización JSON | jsoncpp |
| Base de datos | MongoDB Atlas |
| Administración de BD | MongoDB Compass |
| Frontend | HTML5, Vue 3 y TypeScript |
| Herramientas Frontend | Vite, Vue Router y Pinia |
| Estilos | Tailwind CSS |
| Comunicación HTTP | Axios |
| Pruebas Backend | GoogleTest + CTest |
| Pruebas API | PowerShell |
| Documentación API | OpenAPI 3.1 |

---

## Funcionalidades principales

### 1. Gestión de huéspedes

Endpoint base: `/api/v1/huespedes`

Permite administrar la información de los huéspedes registrados en el sistema.

- Registro de huéspedes.
- Consulta individual y listado paginado.
- Filtros por nombre, correo y estado.
- Actualización parcial de información.
- Validación de correos electrónicos únicos.
- Baja lógica por defecto mediante `activo: false`.
- Eliminación física controlada mediante `?fisico=true`.

La eliminación lógica permite conservar información histórica relacionada con las reservaciones.

### 2. Gestión de habitaciones

Endpoint base: `/api/v1/habitaciones`

Permite administrar el catálogo de habitaciones y sus características operativas.

- Registro y consulta de habitaciones.
- Actualización y eliminación controlada.
- Filtros por tipo, estado, capacidad y tarifa.
- Validación de números de habitación únicos.
- Administración del estado operativo.
- Actualización específica del estado mediante `PATCH /{id}/estado`.

### 3. Gestión de reservaciones

Endpoint base: `/api/v1/reservaciones`

Implementa las operaciones necesarias para administrar las estancias de los huéspedes.

- Creación y consulta de reservaciones.
- Actualización de fechas, habitación, personas y estado.
- Cálculo automático de noches y costo total.
- Conservación de `tarifaAplicada` al momento de la reservación.
- Validación de disponibilidad.
- Protección contra traslapes de fechas.
- Coordinación de solicitudes concurrentes mediante `RoomLockManager`.
- Cancelación explícita mediante `POST /{id}/cancelar`.
- Eliminación física controlada.

#### Control de traslapes

Las estancias se representan mediante intervalos semiabiertos:

\[
[Entrada, Salida)
\]

La condición de traslape implementada es:

\[
E_1 < S_2 \land E_2 < S_1
\]

Donde `E` representa la fecha de entrada y `S` la fecha de salida.

Esto permite realizar una nueva reservación el mismo día en que termina la estancia anterior, siempre que no existan otros conflictos.

El sistema complementa esta validación con `RoomLockManager`, que coordina las operaciones concurrentes dentro de la instancia del backend.

### 4. Consulta de disponibilidad

Endpoint: `/api/v1/disponibilidad`

Permite buscar habitaciones disponibles considerando:

- Fecha de entrada.
- Fecha de salida.
- Número de personas.
- Tipo de habitación.
- Capacidad.
- Estado operativo.
- Reservaciones existentes.
- Costo estimado de la estancia.

El resultado permite iniciar directamente el proceso de reservación desde la habitación seleccionada.

### 5. Reportes administrativos

Endpoint: `/api/v1/reportes/resumen`

Genera información administrativa correspondiente a un periodo determinado.

Incluye:

- Total de reservaciones.
- Desglose de reservaciones por estado.
- Total de habitaciones.
- Habitaciones ocupadas.
- Habitaciones disponibles.
- Habitaciones no operativas.
- Porcentaje de ocupación.
- Ingresos estimados, excluyendo reservaciones canceladas.

El rango de consulta está limitado a un máximo de 366 días.

### 6. Salud del sistema

Endpoint: `/api/v1/health`

Permite verificar el estado de la API y la conectividad con MongoDB Atlas.

| Código HTTP | Resultado |
|---|---|
| `200 OK` | API operativa y conexión a MongoDB disponible. |
| `503 Service Unavailable` | Dependencia de MongoDB temporalmente no disponible. |

---

## Estructura del repositorio

```text
hotel-management-system/
│
├── backend/                         # API REST C++20
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── vcpkg.json
│   ├── .env.example
│   │
│   ├── config/
│   │   └── drogon.config.example.json
│   │
│   ├── include/
│   │   └── hotel/
│   │       ├── config/
│   │       ├── core/
│   │       ├── database/
│   │       ├── models/
│   │       ├── dto/
│   │       ├── validators/
│   │       ├── repositories/
│   │       ├── services/
│   │       ├── middleware/
│   │       └── controllers/
│   │
│   └── src/
│
├── frontend/                        # Panel administrativo Vue 3
│   ├── .env.example
│   └── src/
│       ├── types/
│       ├── services/
│       ├── stores/
│       ├── router/
│       ├── components/
│       └── views/
│
├── tests/
│   ├── backend/
│   │   ├── unit/                    # 16 pruebas unitarias
│   │   └── integration/             # 8 pruebas de integración
│   │
│   └── api/                         # 22 pruebas HTTP
│       └── hotel_api_tests.ps1
│
└── docs/                             # Documentación del proyecto
    ├── arquitectura.md
    ├── modelo_datos.md
    ├── openapi.yaml
    ├── auditoria_seguridad.md
    ├── plan_pruebas_resultados.md
    ├── manual_instalacion_windows.md
    ├── conclusiones.md
    ├── guion_demostracion.md
    │
    └── evidencias/
        └── README.md
```

---

## Requisitos previos

El entorno de desarrollo e instalación documentado corresponde a **Windows 10/11 x64**.

Antes de ejecutar el proyecto se requiere:

| Herramienta | Propósito |
|---|---|
| Visual Studio 2022 / Build Tools | Compilación de C++ mediante MSVC x64. |
| Windows SDK | Bibliotecas y herramientas del sistema. |
| Git | Clonado y control de versiones. |
| CMake | Configuración del proyecto. |
| Ninja | Sistema de compilación. |
| vcpkg | Administración de dependencias C++. |
| Node.js LTS | Ejecución del entorno frontend. |
| npm | Instalación de dependencias frontend. |
| MongoDB Atlas | Servicio de base de datos. |
| MongoDB Compass | Administración y verificación de información. |

En Visual Studio debe instalarse la carga de trabajo **Desarrollo para el escritorio con C++**.

---

## Inicio rápido

Para realizar la instalación completa de Visual Studio, vcpkg, CMake, MongoDB Atlas y las dependencias necesarias, consulta:

**[Manual de instalación y compilación en Windows](docs/manual_instalacion_windows.md)**

Los siguientes pasos resumen el arranque local del proyecto.

### 1. Clonar el repositorio

```powershell
git clone <URL_DEL_REPOSITORIO>

cd hotel-management-system
```

### 2. Configurar las variables de entorno

Crear los archivos locales a partir de las plantillas:

```powershell
Copy-Item .\backend\.env.example .\backend\.env

Copy-Item .\frontend\.env.example .\frontend\.env
```

Editar `backend/.env` y configurar la conexión con MongoDB Atlas:

```env
MONGODB_URI=mongodb+srv://<USUARIO_APP>:<PASSWORD_SEGURA>@<CLUSTER>.mongodb.net/?retryWrites=true&w=majority
MONGODB_DB_NAME=HotelDB

MONGODB_MAX_POOL_SIZE=16
MONGODB_SERVER_SELECTION_TIMEOUT_MS=5000
MONGODB_CONNECT_TIMEOUT_MS=5000

SERVER_HOST=127.0.0.1
SERVER_PORT=8080

CORS_ALLOWED_ORIGIN=http://localhost:5173
LOG_LEVEL=INFO
```

En `frontend/.env`, establecer la URL de la API:

```env
VITE_API_BASE_URL=http://127.0.0.1:8080/api/v1
```

**Importante:** los archivos `.env` contienen información sensible y deben permanecer excluidos del control de versiones. Únicamente se deben subir las plantillas `.env.example` sin credenciales reales.

### 3. Compilar el Backend C++20

Abrir una **Developer PowerShell for VS** y ejecutar desde la carpeta `backend/`:

```powershell
Set-Location .\backend

# Configuración del proyecto.
cmake --preset windows-x64-debug

# Compilación del backend y ejecutables de pruebas.
cmake --build --preset windows-x64-debug
```

La primera compilación puede tardar varios minutos porque vcpkg debe descargar y compilar las dependencias necesarias, incluyendo Drogon, MongoDB C++ Driver, OpenSSL y GoogleTest.

### 4. Ejecutar el Backend

Desde `backend/`:

```powershell
.\build\debug\hotel_api.exe
```

El servidor estará disponible en:

```text
http://127.0.0.1:8080/api/v1
```

### 5. Ejecutar el Frontend Vue 3

Abrir una segunda terminal PowerShell:

```powershell
Set-Location .\frontend

# Instalar dependencias.
npm install

# Iniciar el servidor de desarrollo.
npm run dev
```

Abrir en el navegador:

```text
http://localhost:5173
```

Para generar la compilación de producción:

```powershell
npm run build
```

---

## Ejecución de pruebas

El proyecto dispone de **46 casos de prueba automatizados**, distribuidos entre pruebas unitarias, de integración y pruebas HTTP.

| Tipo de prueba | Casos | Descripción |
|---|---:|---|
| Unitarias C++ | 16 | Validación de componentes internos sin conexión de red. |
| Integración C++ | 8 | Validación de operaciones contra MongoDB Atlas. |
| Pruebas HTTP | 22 | Validación de endpoints mediante solicitudes HTTP. |
| **Total** | **46** | |

### 1. Pruebas unitarias y de integración

Desde `backend/`:

```powershell
Set-Location .\backend

ctest --test-dir build/debug --output-on-failure
```

Las pruebas unitarias no requieren conexión con MongoDB Atlas.

Las pruebas de integración necesitan una conexión funcional con la base de datos y la configuración correspondiente.

### 2. Pruebas HTTP de la API

Con el servidor backend activo en el puerto `8080`, abrir otra terminal desde la raíz del proyecto:

```powershell
Set-Location C:\Proyectos\hotel-management-system

.\tests\api\hotel_api_tests.ps1
```

Este ejecutable realiza las pruebas HTTP sobre los endpoints de la API REST.

### 3. Cobertura de validaciones

Entre los escenarios contemplados se encuentran:

- Operaciones CRUD.
- Validación de datos.
- Restricciones de unicidad.
- Control de reservaciones y traslapes.
- Respuestas HTTP de error.
- Validación de CORS.
- Validación de `Content-Type`.
- Protección frente a operadores NoSQL no permitidos.
- Comprobación de conectividad con MongoDB Atlas.
- Manejo de permisos insuficientes.

---

## Seguridad y privilegios mínimos

El proyecto incorpora controles de seguridad orientados a proteger la información, la comunicación entre componentes y las operaciones realizadas sobre MongoDB Atlas.

### 1. Protección de credenciales

- Los archivos `.env` locales están excluidos de Git.
- Se proporcionan plantillas `.env.example` sin credenciales reales.
- Las URIs de conexión se mantienen fuera del código fuente.
- Los registros del backend aplican redacción de información sensible.
- Las evidencias visuales están sujetas a un proceso de anonimización.

### 2. Privilegios de MongoDB Atlas

Se contemplan tres cuentas diferenciadas:

| Usuario | Privilegios | Propósito |
|---|---|---|
| Cuenta administrativa | Administración | Gestión de Atlas y Compass. |
| `hotel_app_user` | `readWrite` sobre `HotelDB` | Conexión del backend. |
| `hotel_readonly_user` | `read` sobre `HotelDB` | Pruebas de permisos y denegación de escritura. |

La cuenta administrativa permanece separada de las credenciales utilizadas por la aplicación.

### 3. Seguridad de red

MongoDB Atlas utiliza una configuración de IP Allowlist restringida para limitar las direcciones autorizadas a establecer conexiones.

No se recomienda mantener una regla global `0.0.0.0/0` habilitada permanentemente.

### 4. Protección de la API REST

Entre los mecanismos implementados se encuentran:

- CORS con origen permitido explícito.
- Validación de `Content-Type: application/json`.
- Límite de tamaño de payload de 1 MB.
- Cabeceras HTTP de seguridad.
- Filtros BSON tipados.
- Protección contra operadores NoSQL no permitidos.
- Sanitización de expresiones regulares.
- Manejo centralizado de errores.
- Redacción de URIs y credenciales en los logs.

---

## Documentación

El repositorio cuenta con documentación complementaria para facilitar la instalación, comprensión, mantenimiento y evaluación del sistema.

| Documento | Descripción |
|---|---|
| [Manual de instalación](docs/manual_instalacion_windows.md) | Instalación de herramientas, configuración de Atlas, compilación y ejecución en Windows. |
| [Arquitectura](docs/arquitectura.md) | Diseño y organización interna de los componentes del sistema. |
| [Modelo de datos](docs/modelo_datos.md) | Estructura de colecciones BSON, referencias e índices de MongoDB. |
| [Especificación OpenAPI](docs/openapi.yaml) | Contrato formal de la API REST y definición de endpoints. |
| [Auditoría de seguridad](docs/auditoria_seguridad.md) | Controles de seguridad, privilegios y revisión de riesgos. |
| [Plan de pruebas](docs/plan_pruebas_resultados.md) | Planificación, ejecución y resultados de las pruebas automatizadas. |
| [Conclusiones técnicas y académicas](docs/conclusiones.md) | Evaluación de objetivos, decisiones arquitectónicas y mejoras futuras. |
| [Guion de demostración](docs/guion_demostracion.md) | Procedimiento para presentar el sistema en una demostración funcional. |
| [Índice de evidencias](docs/evidencias/README.md) | Catálogo y auditoría de capturas anonimizadas. |

---

## Consideraciones adicionales

- La instalación documentada está orientada a Windows 10/11 x64.
- El backend se compila utilizando MSVC x64, CMake, Ninja y vcpkg.
- MongoDB Atlas debe estar configurado antes de ejecutar las pruebas de integración.
- El backend y frontend se ejecutan en terminales independientes durante el desarrollo.
- MongoDB Compass se utiliza para administración y verificación visual, no como intermediario de comunicación entre la aplicación y la base de datos.
- Las credenciales y datos sensibles nunca deben incorporarse al repositorio.

---

**Sistema de Gestión Hotelera**

*C++20 · Drogon Framework · MongoDB Atlas · Vue 3 · TypeScript*

