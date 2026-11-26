# Conclusiones Técnicas y Académicas

## 1. Cumplimiento de objetivos y arquitectura

Se diseñó, implementó y validó un **Sistema de Gestión Hotelera** mediante una arquitectura web desacoplada, desarrollada para un entorno Windows x64.

La solución integra un backend REST construido con **C++20 y Drogon Framework**, conectado directamente a **MongoDB Atlas (`HotelDB`)** mediante el controlador oficial de MongoDB para C++ (`mongocxx` / `bsoncxx`). Adicionalmente, incorpora un panel administrativo desarrollado con **Vue 3, TypeScript, Vite y Tailwind CSS**.

MongoDB Compass se utilizó como herramienta de administración y verificación visual de la información almacenada, sin intervenir en el flujo operativo de comunicación entre el frontend y el backend.

La separación de responsabilidades en capas (`config`, `core`, `database`, `models`, `validators`, `repositories`, `services`, `dto`, `middleware` y `controllers`) permitió mantener diferenciadas las reglas de negocio, las operaciones HTTP y la interacción con MongoDB.

Esta organización facilita el mantenimiento, la extensión de funcionalidades y la ejecución de pruebas unitarias sin depender necesariamente de servicios externos.

## 2. Modelado NoSQL y reglas de negocio

El diseño de la base de datos permitió aplicar diferentes estrategias de modelado según las características de la información.

### 2.1. Equilibrio entre documentos incrustados y referencias

Se utilizaron arreglos incrustados para atributos de cardinalidad acotada, como `preferencias` en huéspedes y `servicios` en habitaciones.

Por otra parte, las reservaciones utilizan referencias mediante `ObjectId` para identificar a los huéspedes y habitaciones relacionados.

Esta combinación permite mantener información complementaria dentro de los documentos principales y evitar duplicaciones innecesarias de las entidades maestras.

### 2.2. Integridad y rendimiento mediante índices

Se definieron índices para reforzar las restricciones de unicidad y apoyar las consultas utilizadas por el sistema:

- Índice único sobre `huespedes.correo`.
- Índice único sobre `habitaciones.numero`.
- Índice compuesto sobre `reservaciones` utilizando `habitacionId`, `estado`, `fechaEntrada` y `fechaSalida`.

Los índices únicos delegan determinadas restricciones de integridad al propio motor de MongoDB, mientras que los índices de reservaciones ayudan a las consultas relacionadas con la disponibilidad.

### 2.3. Control de traslapes y concurrencia

Uno de los principales desafíos funcionales fue evitar que una misma habitación pudiera reservarse para periodos incompatibles.

Para resolverlo, las estancias se modelaron mediante intervalos semiabiertos `[entrada, salida)`. La condición de traslape utilizada es:

\[
E_1 < S_2 \land E_2 < S_1
\]

Donde:

- \(E_1\): fecha de entrada de la primera reservación.
- \(S_1\): fecha de salida de la primera reservación.
- \(E_2\): fecha de entrada de la segunda reservación.
- \(S_2\): fecha de salida de la segunda reservación.

Esta representación permite que una habitación pueda reservarse nuevamente el mismo día en que termina la estancia anterior, siempre que no exista otro conflicto.

El control se complementa con el mecanismo `RoomLockManager`, que utiliza cerrojos asociados a habitaciones para coordinar operaciones concurrentes dentro del proceso del backend.

La implementación se verificó mediante pruebas con cuatro hilos concurrentes, obteniendo un resultado de una operación exitosa y tres solicitudes rechazadas mediante `HTTP 409` ante el conflicto de reservación.

Este mecanismo cumple su propósito dentro de una única instancia de la aplicación. Una futura arquitectura con múltiples instancias del backend requeriría incorporar coordinación distribuida para conservar la misma garantía entre procesos independientes.

### 2.4. Administración de bajas y dependencias

Se implementó un mecanismo dual para la eliminación de huéspedes.

Por defecto, `DELETE /api/v1/huespedes/{id}` aplica una baja lógica mediante `activo: false`, permitiendo conservar la información histórica relacionada con reservaciones.

La eliminación física se encuentra disponible mediante `?fisico=true`, utilizando `delete_one` cuando se cumplen las condiciones de integridad y no existen dependencias que impidan la operación.

Este comportamiento permite mantener un historial de información sin impedir operaciones administrativas controladas.

## 3. Seguridad y confiabilidad

El proyecto incorpora controles de seguridad en diferentes componentes de la arquitectura.

### 3.1. Seguridad en MongoDB Atlas

Se estableció una separación de cuentas con diferentes niveles de privilegios:

- Cuenta administrativa para tareas de administración de Atlas y Compass.
- `hotel_app_user` con privilegios `readWrite` limitados a `HotelDB`.
- `hotel_readonly_user` con privilegios de lectura sobre `HotelDB`.

También se configuró una lista de direcciones IP autorizadas y se estableció como criterio de seguridad no utilizar una regla permanente de acceso global `0.0.0.0/0`.

Las credenciales reales se mantienen fuera del control de versiones mediante archivos `.env` locales y plantillas `.env.example` anonimizadas.

### 3.2. Seguridad en la API REST

El backend incorpora un conjunto de mecanismos de protección:

- `SecurityMiddleware` para controles de CORS.
- Validación de `Content-Type: application/json`.
- Límites de tamaño de solicitudes de 1 MB.
- Cabeceras HTTP de seguridad.
- Bloqueo de operadores NoSQL no permitidos.
- Validación de entradas y filtros BSON tipados.
- Sanitización de expresiones regulares.
- Manejo centralizado de errores mediante `ErrorMapper`.
- Redacción de credenciales y cadenas de conexión en los registros.

La combinación de estos controles reduce la exposición a entradas maliciosas, errores de configuración y filtraciones accidentales de información sensible.

La administración de privilegios de MongoDB Atlas no sustituye la autenticación de usuarios de la aplicación. La incorporación de autenticación administrativa y autorización por roles a nivel de la API permanece como una posible evolución del proyecto.

## 4. Dificultades superadas y aprendizajes clave

### 4.1. Configuración de la herramienta de compilación C++ en Windows

Uno de los principales retos fue establecer un entorno reproducible de compilación para un backend C++ moderno en Windows.

La combinación de las siguientes herramientas permitió estructurar el proceso:

- MSVC x64 mediante Visual Studio 2022.
- vcpkg en modo manifiesto.
- CMake y `CMakePresets.json`.
- Ninja como generador de compilación.
- Dependencias como Drogon, MongoDB C++ Driver, OpenSSL y GoogleTest.

Este enfoque permitió centralizar la configuración y administración de dependencias, reduciendo la necesidad de configuraciones manuales individuales para cada biblioteca.

### 4.2. Precisión de fechas y zonas horarias

La gestión de reservaciones requiere mantener consistencia entre las fechas seleccionadas en el navegador y las almacenadas en MongoDB.

Para ello, se estableció una normalización de fechas ISO `YYYY-MM-DD` a una representación temporal uniforme, evitando desfases de un día ocasionados por conversiones de zona horaria.

También se identificó la importancia de considerar las limitaciones de la aritmética de punto flotante IEEE 754 al realizar cálculos monetarios.

Una futura mejora sería adoptar una representación monetaria decimal explícita para reforzar la precisión de las operaciones financieras.

### 4.3. Validación integral

La suite de pruebas permitió evaluar diferentes niveles del sistema:

| Tipo de prueba | Casos |
|---|---:|
| Pruebas unitarias C++ | 16 |
| Pruebas de integración con MongoDB Atlas | 8 |
| Pruebas HTTP de la API REST | 22 |
| **Total** | **46** |

Las pruebas unitarias permiten verificar componentes internos, las pruebas de integración comprueban el comportamiento del backend con MongoDB Atlas y las pruebas HTTP evalúan los endpoints mediante solicitudes reales al servidor.

La combinación de estos niveles proporciona una validación más completa que depender exclusivamente de pruebas manuales desde la interfaz.

## 5. Mejoras futuras recomendadas

Aunque la solución cubre las funcionalidades contempladas en el alcance del proyecto, existen oportunidades para ampliar sus capacidades.

1. **Autenticación y autorización:** incorporar autenticación administrativa basada en JWT, almacenamiento seguro de contraseñas mediante Argon2id o bcrypt y control de permisos para perfiles de recepción y gerencia.

2. **Protección adicional de la API:** implementar rate limiting, políticas de bloqueo ante solicitudes excesivas y terminación TLS mediante un proxy inverso como Caddy o Nginx en un entorno de producción.

3. **Escalabilidad y concurrencia distribuida:** incorporar un mecanismo de coordinación entre instancias del backend para mantener la integridad de reservaciones en despliegues con múltiples réplicas.

4. **Reportes y exportación:** ampliar los reportes administrativos mediante exportación a CSV y PDF, además de incorporar indicadores históricos y comparaciones entre periodos.

5. **Transacciones multidocumento:** evaluar el uso de transacciones de MongoDB Atlas para operaciones que involucren varios documentos o procesos administrativos relacionados.

6. **Precisión monetaria:** utilizar representaciones decimales para los cálculos financieros y evitar las limitaciones propias de los números de punto flotante.

## 6. Conclusión general

El desarrollo permitió comprobar la viabilidad de construir una aplicación web administrativa utilizando C++20 como tecnología principal del backend y MongoDB Atlas como servicio de almacenamiento de datos.

La integración de Drogon con el controlador oficial de MongoDB para C++ permitió implementar operaciones persistentes sin depender de una API intermediaria para acceder a la base de datos.

Asimismo, la separación entre frontend, backend y base de datos facilitó la organización del código y permitió establecer responsabilidades claramente diferenciadas.

El proyecto no se limitó a implementar operaciones CRUD, sino que incorporó reglas de negocio relacionadas con reservaciones, disponibilidad, integridad de información, concurrencia y seguridad.

Finalmente, la documentación técnica y las 46 pruebas automatizadas proporcionan una base para reproducir el entorno, verificar las funcionalidades y continuar evolucionando el sistema.

En términos académicos, el proyecto permitió aplicar conocimientos de programación orientada a objetos, arquitectura de software, bases de datos NoSQL, desarrollo de APIs REST, desarrollo frontend, administración de dependencias, pruebas automatizadas y seguridad en aplicaciones web.