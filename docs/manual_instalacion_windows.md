# Sistema de Gestión Hotelera

**Backend C++20 · Drogon · MongoDB Atlas · Vue 3 · TypeScript**

Sistema web desacoplado para la administración hotelera, desarrollado con **C++20 y Drogon Framework**, utilizando el controlador oficial de MongoDB para C++ (`mongocxx` / `bsoncxx`) y una base de datos alojada en **MongoDB Atlas**.

El proyecto incorpora un panel administrativo desarrollado con **Vue 3, TypeScript, Vite y Tailwind CSS**, junto con una API REST para gestionar huéspedes, habitaciones, reservaciones, disponibilidad y reportes administrativos.

---

## Tabla de contenidos

- [Arquitectura general](#arquitectura-general)
- [Stack tecnológico](#stack-tecnológico)
- [Funcionalidades principales](#funcionalidades-principales)
- [Estructura del repositorio](#estructura-del-repositorio)
- [Requisitos previos](#requisitos-previos)
- [Inicio rápido en Windows x64](#inicio-rápido-en-windows-x64)
- [Ejecución de pruebas](#ejecución-de-pruebas)
- [Seguridad y buenas prácticas](#seguridad-y-buenas-prácticas)
- [Documentación](#documentación)

---

## Arquitectura general

El sistema utiliza una arquitectura desacoplada entre el frontend, el backend y la base de datos.

```text
                  NAVEGADOR WEB
                        |
                        |
             Vue 3 + TypeScript
             Pinia + Tailwind CSS
                  Vite :5173
                        |
                        | HTTP / JSON
                        | REST API v1
                        | CORS
                        |
                        v
              BACKEND C++20 / DROGON
                     :8080
                        |
          +-------------+-------------+
          |             |             |
          v             v             v
     Controllers     Services     Middleware
          |             |             |
          |             |             |
          +-------------+-------------+
                        |
                        v
                  Repositories
                  BSON tipado
                        |
                        v
              MongoConnectionManager
                  mongocxx::pool
                        |
                        | TLS
                        |
                        v
                 MONGODB ATLAS
                     HotelDB
                        |
          +-------------+-------------+
          |             |             |
          v             v             v
      huespedes     habitaciones  reservaciones

                 MONGODB COMPASS
             Administración y verificación
```

### Descripción de las capas

| Componente | Responsabilidad |
|---|---|
| Controllers | Recepción de solicitudes HTTP y respuesta al cliente. |
| DTOs | Estructuración de datos de entrada y salida. |
| Services | Reglas de negocio y coordinación de operaciones. |
| Validators | Validación de datos y restricciones de entrada. |
| Repositories | Operaciones CRUD y consultas sobre MongoDB. |
| Middleware | CORS, cabeceras HTTP, Content-Type y controles de seguridad. |
| MongoConnectionManager | Administración de la instancia y el pool de conexiones de MongoDB. |

---

## Stack tecnológico

| Capa | Tecnologías |
|---|---|
| Lenguaje Backend | C++20 |
| Framework Backend | Drogon Framework |
| Compilador | MSVC x64 (Visual Studio 2022) |
| Sistema de compilación | CMake + Ninja |
| Gestión de dependencias | vcpkg |
| Driver de MongoDB | mongo-cxx-driver (`mongocxx` / `bsoncxx`) |
| Base de datos | MongoDB Atlas |
| Administración de BD | MongoDB Compass |
| Frontend | HTML5, Vue 3 y TypeScript |
| Herramientas Frontend | Vite, Vue Router y Pinia |
| Estilos | Tailwind CSS |
| Comunicación HTTP | Axios |
| Pruebas Backend | GoogleTest + CTest |
| Pruebas API | PowerShell y colección Postman/Bruno |

---

## Funcionalidades principales

### 1. Gestión de huéspedes

Endpoint base: `/api/v1/huespedes`

- Registro de nuevos huéspedes.
- Consulta y listado con paginación.
- Búsqueda mediante filtros por nombre, correo y estado.
- Actualización parcial de información.
- Validación de correos electrónicos únicos.
- Baja lógica mediante `activo: false`.
- Eliminación física controlada mediante `?fisico=true`.

### 2. Gestión de habitaciones

Endpoint base: `/api/v1/habitaciones`

- Alta, consulta, modificación y eliminación de habitaciones.
- Filtros por tipo, estado, capacidad y tarifa.
- Validación de números de habitación únicos.
- Administración del estado operativo.
- Actualización específica mediante `PATCH /{id}/estado`.

### 3. Gestión de reservaciones

Endpoint base: `/api/v1/reservaciones`

- Creación y consulta de reservaciones.
- Actualización de fechas, habitación, personas y estado.
- Cálculo automático de noches y total.
- Conservación de la tarifa aplicada al momento de reservar.
- Cancelación explícita de reservaciones.
- Eliminación física controlada.
- Validación de disponibilidad antes de confirmar una reservación.
- Protección contra traslapes y conflictos de concurrencia.

La detección de traslapes utiliza intervalos semiabiertos `[entrada, salida)`, mediante la condición:

```cpp
S1 < E2 && S2 < E1
```

Donde `S` representa la fecha de entrada y `E` la fecha de salida.

Además, se utiliza un mecanismo de bloqueo particionado por habitación (`RoomLockManager`) para coordinar las operaciones concurrentes.

### 4. Consulta de disponibilidad

Endpoint: `/api/v1/disponibilidad`

Permite consultar habitaciones disponibles considerando:

- Fecha de entrada.
- Fecha de salida.
- Número de personas.
- Tipo de habitación.
- Capacidad y estado operativo.
- Costo estimado de la estancia.

### 5. Reportes administrativos

Endpoint: `/api/v1/reportes/resumen`

Genera información administrativa del periodo solicitado:

- Total de reservaciones.
- Reservaciones agrupadas por estado.
- Total de habitaciones.
- Habitaciones ocupadas y disponibles.
- Habitaciones no operativas.
- Porcentaje de ocupación.
- Ingresos estimados, excluyendo reservaciones canceladas.

El rango de consulta está limitado a un máximo de 366 días.

### 6. Estado del sistema

Endpoint: `/api/v1/health`

Permite verificar el estado de la API y la conectividad con MongoDB Atlas.

- `200 OK`: API operativa y conexión a la base de datos disponible.
- `503 Service Unavailable`: dependencia de MongoDB no disponible.

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
├── frontend/                        # Aplicación Vue 3
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
└── docs/
    ├── arquitectura.md
    ├── modelo_datos.md
    ├── openapi.yaml
    ├── auditoria_seguridad.md
    ├── plan_pruebas_resultados.md
    ├── manual_instalacion_windows.md
    └── evidencias/
```

---

## Requisitos previos

El entorno de instalación documentado corresponde a **Windows 10/11 de 64 bits**.

Antes de compilar y ejecutar el proyecto se requiere:

| Herramienta | Uso |
|---|---|
| Visual Studio 2022 o Build Tools | Compilación C++ mediante MSVC x64. |
| Windows SDK | Herramientas y bibliotecas del sistema. |
| Git | Clonado y control de versiones. |
| CMake | Configuración del proyecto. |
| Ninja | Sistema de compilación. |
| Node.js LTS | Ejecución del entorno frontend. |
| npm | Instalación de dependencias frontend. |
| vcpkg | Gestión de dependencias C++. |
| MongoDB Atlas | Servicio de base de datos. |
| MongoDB Compass | Administración y verificación de datos. |

En Visual Studio debe instalarse la carga de trabajo **Desarrollo para el escritorio con C++**.

---

## Inicio rápido en Windows x64

Para conocer el procedimiento completo de instalación, configuración de MongoDB Atlas y compilación, consulta:

**[Manual de instalación para Windows](docs/manual_instalacion_windows.md)**

Los siguientes pasos resumen el arranque local del proyecto.

### 1. Clonar el repositorio

```powershell
git clone <URL_DEL_REPOSITORIO>
cd hotel-management-system
```

### 2. Configurar las variables de entorno

Crear los archivos de configuración locales a partir de las plantillas:

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

En `frontend/.env`, configurar la URL de la API:

```env
VITE_API_BASE_URL=http://127.0.0.1:8080/api/v1
```

**Importante:** Los archivos `.env` contienen información sensible y no deben subirse al repositorio.

### 3. Compilar el Backend

Abrir una **Developer PowerShell for VS** y ejecutar:

```powershell
Set-Location .\backend

# Configurar el proyecto.
cmake --preset windows-x64-debug

# Compilar la API y los ejecutables de pruebas.
cmake --build --preset windows-x64-debug
```

La primera compilación puede tardar varios minutos debido a la descarga y compilación de dependencias mediante vcpkg, incluyendo Drogon, MongoDB C++ Driver, OpenSSL y GoogleTest.

### 4. Ejecutar el Backend

Desde el directorio `backend/`:

```powershell
.\build\debug\hotel_api.exe
```

El servidor estará disponible en:

```text
http://127.0.0.1:8080/api/v1
```

### 5. Ejecutar el Frontend

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

El proyecto cuenta con **46 casos de prueba documentados**, distribuidos entre pruebas unitarias, pruebas de integración y pruebas HTTP.

| Tipo de prueba | Cantidad | Dependencia |
|---|---:|---|
| Unitarias C++ | 16 | No requiere conexión de red. |
| Integración C++ | 8 | Requiere MongoDB Atlas. |
| Pruebas HTTP de API | 22 | Requiere el servidor activo. |
| **Total** | **46** | |

### 1. Pruebas unitarias

Desde el directorio `backend/`:

```powershell
ctest --test-dir build/debug -L unit --output-on-failure
```

Estas pruebas verifican componentes internos del backend sin requerir conexión con MongoDB Atlas.

### 2. Pruebas de integración

Desde el directorio `backend/`:

```powershell
ctest --test-dir build/debug -L integration --output-on-failure
```

Estas pruebas requieren una conexión funcional con MongoDB Atlas.

De manera opcional, puede configurarse una URI de solo lectura para ejecutar la prueba de denegación de escritura:

```powershell
$env:MONGODB_READONLY_URI = "mongodb+srv://hotel_readonly_user:<PASSWORD>@<CLUSTER>.mongodb.net/?retryWrites=true&w=majority"
```

Al finalizar:

```powershell
Remove-Item Env:MONGODB_READONLY_URI -ErrorAction SilentlyContinue
```

### 3. Pruebas HTTP de la API

Con el backend ejecutándose en el puerto `8080`, abrir otra terminal PowerShell desde la raíz del repositorio:

```powershell
.\tests\api\hotel_api_tests.ps1
```

Este ejecutable realiza las pruebas HTTP sobre los endpoints de la API REST.

---

## Seguridad y buenas prácticas

El proyecto incorpora controles orientados a proteger la comunicación entre componentes, la información almacenada y las operaciones realizadas sobre la base de datos.

### Protección de credenciales

- Los archivos `.env` locales están excluidos del control de versiones.
- Se proporcionan archivos `.env.example` sin credenciales reales.
- Se utiliza una cuenta específica para la conexión del backend.
- La cuenta administrativa de MongoDB Atlas se mantiene separada de las credenciales de la aplicación.

### Control de privilegios en MongoDB Atlas

Se contemplan cuentas con diferentes niveles de acceso:

| Usuario | Privilegios | Uso |
|---|---|---|
| Cuenta administrativa | Administración | Gestión de Atlas y Compass. |
| `hotel_app_user` | `readWrite` sobre `HotelDB` | Conexión del backend. |
| `hotel_readonly_user` | `read` sobre `HotelDB` | Pruebas de permisos y denegación de escritura. |

### Protección de la API

Entre los controles implementados se encuentran:

- Filtros BSON tipados para las operaciones con MongoDB.
- Validación de entradas y restricciones de datos.
- Protección contra operadores NoSQL no permitidos.
- Escape de expresiones regulares.
- Configuración de CORS con origen permitido explícito.
- Validación de `Content-Type`.
- Límites de tamaño de payload de 1 MB.
- Cabeceras HTTP de seguridad.
- Manejo centralizado de errores.
- Redacción de URI y credenciales en los registros.
- Control de conflictos de reservaciones y concurrencia.

### Seguridad de red en MongoDB Atlas

Se recomienda registrar únicamente las direcciones IP públicas autorizadas en **Network Access**.

No se debe mantener `0.0.0.0/0` habilitado de forma permanente en un entorno de trabajo real.

---

## Documentación

El proyecto cuenta con documentación complementaria para facilitar su instalación, mantenimiento, revisión y comprensión técnica.

| Documento | Descripción |
|---|---|
| [Manual de instalación](docs/manual_instalacion_windows.md) | Instalación de herramientas, configuración de Atlas, compilación y ejecución en Windows. |
| [Arquitectura](docs/arquitectura.md) | Organización interna y diseño de los componentes del sistema. |
| [Modelo de datos](docs/modelo_datos.md) | Estructura de colecciones BSON, referencias e índices de MongoDB. |
| [Especificación OpenAPI](docs/openapi.yaml) | Contrato de la API REST y definición de endpoints. |
| [Auditoría de seguridad](docs/auditoria_seguridad.md) | Controles de seguridad, permisos y revisión de riesgos. |
| [Plan de pruebas](docs/plan_pruebas_resultados.md) | Planificación, ejecución y resultados de las pruebas. |
| [Evidencias](docs/evidencias/) | Material complementario y evidencias del proyecto. |

---

## Notas de instalación

- La guía de instalación está orientada a Windows 10/11 x64.
- El backend se compila en configuración `Debug` mediante MSVC, CMake, Ninja y vcpkg.
- MongoDB Atlas debe estar configurado antes de ejecutar las pruebas de integración.
- El backend y el frontend se ejecutan en terminales independientes durante el desarrollo.
- MongoDB Compass es una herramienta de administración y verificación; no forma parte del flujo de comunicación entre el frontend y el backend.

Para instrucciones detalladas, consulta el [manual de instalación](docs/manual_instalacion_windows.md).

---

**Sistema de Gestión Hotelera — Backend C++20 + Drogon | MongoDB Atlas | Frontend Vue 3**