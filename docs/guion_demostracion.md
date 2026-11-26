# Guion de Demostración Funcional en Vivo

**Sistema de Gestión Hotelera — C++20 · Drogon · MongoDB Atlas · Vue 3**

<text>Duración estimada: 8 a 10 minutos.</text>

## 1. Objetivo de la demostración

Presentar el funcionamiento del Sistema de Gestión Hotelera mediante una demostración práctica que permita verificar:

- La configuración y el modelado de la base de datos MongoDB Atlas.
- La persistencia de información mediante operaciones CRUD.
- La implementación de controles de seguridad y privilegios mínimos.
- La comunicación entre el backend C++20 y MongoDB Atlas.
- El funcionamiento del frontend Vue 3 y las funcionalidades administrativas.
- La ejecución de pruebas automatizadas.

La exposición está organizada de acuerdo con los cinco criterios de evaluación de la rúbrica.

---

## 2. Preparación previa

Realizar estas actividades antes de iniciar la exposición.

### 2.1. MongoDB Compass

Abrir MongoDB Compass con una conexión funcional a `HotelDB`.

Dejar visibles las siguientes colecciones:

- `huespedes`
- `habitaciones`
- `reservaciones`

Verificar que la barra superior de conexión no exponga la URI real del clúster.

### 2.2. MongoDB Atlas

Abrir el panel administrativo de MongoDB Atlas.

Dejar preparadas las siguientes secciones:

- Database Access.
- Network Access.
- Clúster activo.

No mostrar contraseñas, credenciales ni información sensible.

### 2.3. Visual Studio Code

Abrir el proyecto desde:

```text
C:\Proyectos\hotel-management-system
```

Preparar tres terminales PowerShell:

| Terminal | Responsabilidad |
|---|---|
| Terminal 1 | Ejecución del backend C++20. |
| Terminal 2 | Ejecución del frontend Vue 3. |
| Terminal 3 | Ejecución de pruebas y verificaciones. |

### 2.4. Comprobaciones adicionales

Antes de iniciar:

- Verificar que MongoDB Atlas permita la conexión desde la IP actual.
- Confirmar que los archivos `.env` contienen la configuración local correcta.
- Comprobar que el backend puede conectarse con `HotelDB`.
- Verificar que existen datos ficticios de demostración.
- Revisar que no existan reservaciones de prueba que interfieran con los escenarios de disponibilidad.
- Tener preparado un registro de demostración para crear, modificar y eliminar durante la exposición.

---

## 3. Desarrollo de la demostración

### Paso 1. Arquitectura y modelo de datos en Atlas/Compass

<text>Tiempo estimado: 1 minuto y 30 segundos.</text>

**Criterio 1 — Base de datos y modelado NoSQL.**

1. Abrir MongoDB Compass.
2. Mostrar la base de datos `HotelDB`.
3. Identificar las colecciones `huespedes`, `habitaciones` y `reservaciones`.
4. Abrir un documento de la colección `reservaciones`.
5. Explicar las referencias mediante `ObjectId`:
   - `huespedId`.
   - `habitacionId`.
6. Mostrar el campo `tarifaAplicada`, destacando que conserva la tarifa utilizada al momento de crear la reservación.
7. Mostrar los arreglos incrustados `preferencias` y `servicios` en las colecciones correspondientes.

Posteriormente, abrir la pestaña **Indexes** y mostrar:

- Índice único `{ correo: 1 }` en `huespedes`.
- Índice único `{ numero: 1 }` en `habitaciones`.
- Índice compuesto `{ habitacionId: 1, estado: 1, fechaEntrada: 1, fechaSalida: 1 }` en `reservaciones`.

**Explicación sugerida:**

"La base de datos utiliza documentos BSON y combina referencias entre colecciones con arreglos incrustados. También se configuraron índices para reforzar la unicidad de los datos y apoyar las consultas de disponibilidad."

---

### Paso 2. Conexión C++ (`mongocxx`) y arranque del sistema

<text>Tiempo estimado: 1 minuto y 30 segundos.</text>

**Criterio 4 — Backend y conexión con MongoDB Atlas.**

En la Terminal 1, ejecutar:

```powershell
Set-Location "C:\Proyectos\hotel-management-system\backend"

.\build\debug\hotel_api.exe
```

Mostrar en consola, cuando aparezcan:

- Inicialización de `mongocxx::pool`.
- Conexión con MongoDB Atlas.
- URI redactada en los registros.
- Verificación `ping()` sobre `HotelDB`.
- Inicialización de `SecurityMiddleware`.
- Inicio del servidor HTTP.

La URI debe aparecer anonimizada, por ejemplo:

```text
mongodb+srv://***:***@<redacted>
```

En la Terminal 2, ejecutar el frontend:

```powershell
Set-Location "C:\Proyectos\hotel-management-system\frontend"

npm run dev
```

Abrir en el navegador:

```text
http://localhost:5173
```

Mostrar el Dashboard y señalar:

- Estado de la API.
- Estado de conexión con Atlas.
- Indicadores administrativos.
- Información cargada desde la base de datos.

Comprobar el endpoint:

```text
GET http://127.0.0.1:8080/api/v1/health
```

Resultado esperado:

```text
HTTP 200 OK
```

**Explicación sugerida:**

"El backend está desarrollado en C++20 utilizando Drogon. La conexión con MongoDB se realiza directamente mediante el controlador oficial `mongocxx`, mientras que el frontend consume la API REST mediante solicitudes HTTP y respuestas JSON."

---

### Paso 3. Demostración CRUD desde la web y reflejo en Compass

<text>Tiempo estimado: 2 minutos.</text>

**Criterio 2 — Operaciones CRUD y persistencia.**

#### 3.1. Gestión de huéspedes

Ingresar a:

```text
/huespedes
```

Crear un huésped ficticio con los siguientes datos:

```text
Nombre: Elena Demo
Correo: elena.demo@ejemplo.com
Teléfono: 4775550199
```

Después:

1. Abrir MongoDB Compass.
2. Actualizar la colección `huespedes`.
3. Mostrar el documento recién insertado.
4. Intentar crear otro huésped con el mismo correo.
5. Mostrar el rechazo controlado mediante `HTTP 409`.
6. Modificar el teléfono del huésped.
7. Aplicar la baja lógica.
8. Ejecutar la eliminación física controlada, siempre que no existan reservaciones asociadas.
9. Verificar en Compass que el documento haya desaparecido.

Relacionar las operaciones con:

- `insert_one`.
- `update_one`.
- `delete_one`.

#### 3.2. Gestión de habitaciones

Ingresar a:

```text
/habitaciones
```

Mostrar:

- Listado de habitaciones.
- Filtros disponibles.
- Información de capacidad y tarifa.
- Estado operativo.

Cambiar temporalmente el estado de una habitación a `mantenimiento` y posteriormente regresarla a `disponible`.

Identificar la operación:

```text
PATCH /api/v1/habitaciones/{id}/estado
```

**Explicación sugerida:**

"Las operaciones se ejecutan desde el frontend mediante la API y se reflejan directamente en MongoDB Atlas. Compass permite verificar que los cambios realmente se almacenan en la base de datos."

---

### Paso 4. Disponibilidad, traslapes y reportes

<text>Tiempo estimado: 2 minutos.</text>

**Criterio 5 — Funcionalidades del sistema.**

#### 4.1. Consulta de disponibilidad

Ingresar a:

```text
/disponibilidad
```

Realizar la primera consulta con:

```text
Fecha de entrada: 2026-12-06
Fecha de salida: 2026-12-08
```

De acuerdo con los datos de demostración preparados, la habitación 102 debe quedar excluida porque cuenta con una reservación del 5 al 8 de diciembre.

Mostrar las habitaciones disponibles y sus costos estimados.

A continuación, cambiar las fechas:

```text
Fecha de entrada: 2026-12-08
Fecha de salida: 2026-12-10
```

Mostrar el caso frontera permitido: la habitación 102 puede volver a aparecer disponible porque la nueva estancia comienza exactamente el día en que finaliza la anterior, siempre que no existan otros conflictos.

Explicar la representación mediante intervalos semiabiertos:

```text
[fechaEntrada, fechaSalida)
```

#### 4.2. Creación de reservaciones

Seleccionar una habitación disponible y utilizar la opción para iniciar una reservación.

Mostrar:

- Formulario prellenado.
- Información del huésped.
- Habitación seleccionada.
- Fechas.
- Número de personas.
- Cálculo del total.

Crear la reservación y mostrar el registro en MongoDB Compass, dentro de la colección `reservaciones`.

Después, intentar generar otra reservación para la misma habitación con fechas incompatibles.

Resultado esperado:

```text
HTTP 409 Conflict
RESERVATION_OVERLAP
```

Explicar que la aplicación comprueba la disponibilidad antes de aceptar una operación y coordina las solicitudes concurrentes mediante `RoomLockManager`.

Cancelar y eliminar la reservación temporal de demostración, cuando corresponda, para conservar el conjunto de datos preparado.

#### 4.3. Reportes administrativos

Ingresar a:

```text
/reportes
```

Seleccionar el periodo de diciembre de 2026.

Mostrar:

- Total de reservaciones.
- Reservaciones por estado.
- Porcentaje de ocupación.
- Habitaciones disponibles y no operativas.
- Ingresos estimados.

Como referencia, el conjunto de demostración puede mostrar los siguientes valores:

| Indicador | Valor esperado |
|---|---:|
| Total de reservaciones | 1 |
| Ocupación | 33.33 % |
| Ingresos estimados | $4,350.00 MXN |

**Importante:** estos valores dependen de los datos precargados en `HotelDB`. Deben verificarse antes de la exposición y no presentarse como valores fijos cuando el conjunto de datos sea diferente.

**Explicación sugerida:**

"El módulo de disponibilidad evita conflictos de fechas y permite consultar habitaciones elegibles. Los reportes procesan la información almacenada en MongoDB para obtener indicadores operativos y administrativos."

---

### Paso 5. Seguridad, privilegios mínimos y pruebas automatizadas

<text>Tiempo estimado: 2 minutos.</text>

**Criterios 3 y 5 — Seguridad y validación del sistema.**

#### 5.1. Configuración de privilegios en Atlas

Abrir MongoDB Atlas y mostrar brevemente **Database Access**.

Identificar las cuentas configuradas:

| Usuario | Rol |
|---|---|
| Administrador | Gestión de Atlas y Compass. |
| `hotel_app_user` | `readWrite` sobre `HotelDB`. |
| `hotel_readonly_user` | `read` sobre `HotelDB`. |

Abrir **Network Access** y mostrar que el acceso está limitado mediante IP Allowlist, sin una regla permanente `0.0.0.0/0`.

No mostrar información sensible de las cuentas.

#### 5.2. Prueba de permisos de solo lectura

Cuando la prueba correspondiente esté habilitada, mostrar el comportamiento esperado de `hotel_readonly_user`:

- Las operaciones de lectura permitidas responden correctamente.
- Las operaciones de escritura son rechazadas.
- El backend transforma el error de permisos en una respuesta controlada.

Resultado esperado:

```text
HTTP 403 Forbidden
DB_UNAUTHORIZED
```

#### 5.3. Pruebas automatizadas HTTP

En la Terminal 3, ejecutar:

```powershell
Set-Location "C:\Proyectos\hotel-management-system"

.\tests\api\hotel_api_tests.ps1
```

Mostrar los resultados de los 22 casos de prueba HTTP.

Entre los escenarios que deben destacarse se encuentran:

- Validación de respuestas HTTP.
- Control de CORS.
- Rechazo de `Content-Type` no permitido mediante `415`.
- Protección contra operadores NoSQL no permitidos, como `$ne`.
- Conflictos de reservaciones.
- Validaciones de datos.
- Manejo de recursos inexistentes.

Comprobar también que no existan archivos `.env` reales rastreados por Git:

```powershell
git ls-files | Select-String '(^|/)\.env$'
```

El resultado esperado es que no aparezcan archivos `.env` reales versionados.

Los archivos de plantilla `.env.example` sí pueden estar presentes.

#### 5.4. Pruebas de backend

De manera complementaria, desde `backend/` ejecutar:

```powershell
Set-Location "C:\Proyectos\hotel-management-system\backend"

ctest --test-dir build/debug --output-on-failure
```

Mostrar los resultados esperados:

| Pruebas | Casos |
|---|---:|
| Unitarias | 16 |
| Integración | 8 |
| Total backend | 24 |

En conjunto con las 22 pruebas HTTP, el proyecto cuenta con 46 casos de prueba documentados.

**Explicación sugerida:**

"Finalmente, se verifica que la aplicación utiliza privilegios mínimos en MongoDB, protege las credenciales y cuenta con pruebas automatizadas para validar las operaciones del backend y los endpoints HTTP."

---

## 4. Cierre de la exposición

<text>Tiempo estimado: 30 segundos.</text>

Concluir destacando los siguientes aspectos:

1. Se implementó una API REST en C++20 utilizando Drogon.
2. La aplicación se conecta directamente a MongoDB Atlas mediante `mongocxx`.
3. El frontend Vue 3 consume los servicios de la API de manera desacoplada.
4. Se implementaron operaciones CRUD, disponibilidad, reservaciones y reportes.
5. Se incorporaron controles de seguridad y privilegios mínimos.
6. Se documentaron y ejecutaron 46 casos de prueba automatizados.

**Cierre sugerido:**

"El proyecto demuestra la integración de C++20 con MongoDB Atlas en una arquitectura web desacoplada. Además de implementar las funcionalidades administrativas, se trabajó en la integridad de las reservaciones, el control de concurrencia, la seguridad de la conexión y la validación automatizada del sistema."

---

# 5. Auditoría final preentrega

Esta sección es una verificación técnica adicional. Puede ejecutarse antes de generar la entrega definitiva o realizar el commit de cierre. No forma parte del tiempo estimado de exposición.

## 5.1. Auditoría de secretos en Git

Desde la raíz del proyecto:

```powershell
Set-Location "C:\Proyectos\hotel-management-system"

Write-Host "--- Archivos .env rastreados ---" -ForegroundColor Cyan

$trackedEnv = git ls-files | Where-Object {
    $_ -match '(^|/)\.env$'
}

if ($trackedEnv) {
    Write-Host "ERROR: Se encontraron archivos .env versionados." -ForegroundColor Red
    $trackedEnv
} else {
    Write-Host "OK: No existen archivos .env reales rastreados." -ForegroundColor Green
}

Write-Host "`n--- Plantillas .env.example ---" -ForegroundColor Cyan

git ls-files | Select-String '(^|/)\.env\.example$'
```

La primera comprobación no debe devolver archivos. Las plantillas `.env.example` sí pueden aparecer en la segunda.

## 5.2. Verificar el estado de los archivos locales

```powershell
Write-Host "--- Estado de los archivos de configuración ---" -ForegroundColor Cyan

git status --short --ignored backend/.env frontend/.env
```

Los archivos locales deben estar ignorados por Git y no aparecer como archivos preparados para seguimiento.

## 5.3. Validar la compilación del frontend

```powershell
Write-Host "--- Build Frontend ---" -ForegroundColor Cyan

Set-Location "C:\Proyectos\hotel-management-system\frontend"

npm run build
```

Verificar que Vite finalice sin errores de compilación.

## 5.4. Validar la compilación del backend

Abrir una Developer PowerShell for VS:

```powershell
Write-Host "--- Build Backend ---" -ForegroundColor Cyan

Set-Location "C:\Proyectos\hotel-management-system\backend"

cmake --preset windows-x64-debug

cmake --build --preset windows-x64-debug
```

Verificar que la configuración y compilación finalicen correctamente.

## 5.5. Ejecutar pruebas automatizadas

Pruebas del backend:

```powershell
Set-Location "C:\Proyectos\hotel-management-system\backend"

ctest --test-dir build/debug --output-on-failure
```

Pruebas HTTP (requieren que el backend esté ejecutándose):

```powershell
Set-Location "C:\Proyectos\hotel-management-system"

.\tests\api\hotel_api_tests.ps1
```

Comprobar los resultados de las 24 pruebas de backend y las 22 pruebas HTTP.

## 5.6. Revisar cambios pendientes

Desde la raíz del proyecto:

```powershell
Set-Location "C:\Proyectos\hotel-management-system"

Write-Host "--- Validación de espacios y formato ---" -ForegroundColor Cyan

git diff --check

Write-Host "`n--- Estado del repositorio ---" -ForegroundColor Cyan

git status --short
```

Revisar que los cambios correspondan únicamente a los archivos que se pretende incorporar.

## 5.7. Commit de cierre

Una vez revisadas las modificaciones, las evidencias y los resultados de las pruebas, se puede generar el commit final:

```powershell
Set-Location "C:\Proyectos\hotel-management-system"

git add README.md docs/

git commit -m "docs: finalize project documentation and evidence"
```

Verificar el commit:

```powershell
git log -1 --oneline

git status --short
```

El objetivo es terminar con el repositorio documentado, las evidencias anonimizadas y los cambios pendientes correctamente revisados.

---

**Fin del guion de demostración.**