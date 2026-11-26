# Índice de Evidencias Anonimizadas

**Sistema de Gestión Hotelera — C++20 · Drogon · MongoDB Atlas · Vue 3**

Esta carpeta contiene las evidencias visuales correspondientes a la implementación, configuración, funcionamiento y validación del Sistema de Gestión Hotelera.

Las capturas están organizadas de acuerdo con los criterios establecidos en la rúbrica de evaluación.

## 1. Política de anonimización

Todas las evidencias deben cumplir las siguientes condiciones de seguridad y privacidad:

- No mostrar contraseñas ni credenciales de acceso.
- No exponer cadenas de conexión reales (`MONGODB_URI`).
- No mostrar direcciones IP sensibles.
- No incluir datos personales reales de huéspedes.
- Ocultar correos electrónicos asociados a cuentas administrativas.
- No mostrar información que permita identificar o acceder directamente al clúster de MongoDB Atlas.

Los datos presentados en las capturas deben ser ficticios y utilizarse exclusivamente para fines de demostración y evaluación académica.

---

## 2. Catálogo de evidencias por criterio de evaluación

| Archivo | Criterio | Descripción de la evidencia |
|---|---|---|
| `01_atlas_cluster.png` | Criterio 1 (20 pts) | Clúster activo en MongoDB Atlas. La captura debe ocultar el correo de la cuenta administrativa. |
| `02_compass_hoteldb_colecciones.png` | Criterio 1 (20 pts) | Base de datos `HotelDB` en MongoDB Compass mostrando las colecciones `huespedes`, `habitaciones` y `reservaciones`. |
| `03_compass_documentos_bson.png` | Criterio 1 (20 pts) | Documentos BSON de ejemplo que muestran referencias mediante `ObjectId` y arreglos incrustados. |
| `04_compass_indices.png` | Criterio 1 (20 pts) | Pestaña *Indexes* mostrando los índices únicos de correo y número de habitación, así como el índice compuesto de reservaciones. |
| `05_crud_api_y_compass.png` | Criterio 2 (25 pts) | Operaciones CRUD ejecutadas desde la API C++ o el frontend, acompañadas de su reflejo en MongoDB Compass. |
| `06_atlas_database_access.png` | Criterio 3 (15 pts) | Usuarios diferenciados en MongoDB Atlas: administrador, `hotel_app_user` con `readWrite` sobre `HotelDB` y `hotel_readonly_user` con `read` sobre `HotelDB`. |
| `07_atlas_network_access.png` | Criterio 3 (15 pts) | Configuración restringida de IP Allowlist. No debe existir una regla permanente `0.0.0.0/0`. |
| `08_prueba_permisos_403.png` | Criterio 3 (15 pts) | Operación de lectura exitosa y rechazo controlado `HTTP 403 DB_UNAUTHORIZED` al intentar escribir con `hotel_readonly_user`. |
| `09_auditoria_git_sin_secretos.png` | Criterio 3 (15 pts) | Verificación mediante `git ls-files` y `git grep` para demostrar que los archivos `.env` reales están excluidos del repositorio. |
| `10_backend_arranque_health.png` | Criterio 4 (20 pts) | Arranque de `hotel_api.exe`, inicialización de `mongocxx::pool`, URI redactada y respuesta satisfactoria de `GET /api/v1/health`. |
| `11_frontend_dashboard_crud.png` | Criterio 5 (20 pts) | Panel administrativo Vue 3 mostrando el Dashboard y las vistas de huéspedes y habitaciones consumiendo la API. |
| `12_frontend_disponibilidad_traslape.png` | Criterio 5 (20 pts) | Consulta de disponibilidad y alerta controlada ante un intento de reservación con fechas traslapadas (`HTTP 409`). |
| `13_frontend_reportes.png` | Criterio 5 (20 pts) | Vista `/reportes` mostrando ocupación, reservaciones por estado e ingresos estimados, contrastados con los datos de Compass. |
| `14_pruebas_automatizadas_46_ok.png` | Criterio 5 (20 pts) | Ejecución satisfactoria de las 24 pruebas de backend y las 22 pruebas HTTP de la API. |

---

## 3. Organización de las evidencias

La estructura esperada de esta carpeta es la siguiente:

```text
docs/
└── evidencias/
    ├── README.md
    ├── 01_atlas_cluster.png
    ├── 02_compass_hoteldb_colecciones.png
    ├── 03_compass_documentos_bson.png
    ├── 04_compass_indices.png
    ├── 05_crud_api_y_compass.png
    ├── 06_atlas_database_access.png
    ├── 07_atlas_network_access.png
    ├── 08_prueba_permisos_403.png
    ├── 09_auditoria_git_sin_secretos.png
    ├── 10_backend_arranque_health.png
    ├── 11_frontend_dashboard_crud.png
    ├── 12_frontend_disponibilidad_traslape.png
    ├── 13_frontend_reportes.png
    └── 14_pruebas_automatizadas_46_ok.png
```

Los nombres indicados constituyen una propuesta de nomenclatura. Cada archivo debe incorporarse únicamente cuando la captura correspondiente haya sido generada y revisada.

---

## 4. Checklist de auditoría de anonimización

Antes de incorporar las capturas al repositorio o entregar el proyecto, verificar los siguientes puntos:

### MongoDB Atlas y Compass

- [ ] Ninguna captura muestra contraseñas o credenciales.
- [ ] No se exponen URIs reales de conexión `mongodb+srv://`.
- [ ] En MongoDB Compass, la barra de conexión está recortada o difuminada cuando muestra información sensible.
- [ ] En Atlas Network Access no aparece una regla permanente `0.0.0.0/0`.
- [ ] Los usuarios y roles mostrados corresponden a la configuración documentada.
- [ ] Los huéspedes registrados en `HotelDB` utilizan nombres, correos y teléfonos ficticios.

### Repositorio y configuración

- [ ] `backend/.env` está excluido de Git.
- [ ] `frontend/.env` está excluido de Git.
- [ ] Los archivos `.env.example` no contienen credenciales reales.
- [ ] No existen cadenas de conexión reales en los archivos versionados.
- [ ] No se incluyen contraseñas en capturas de terminal o editores de código.

### Evidencias funcionales

- [ ] Las capturas corresponden a funcionalidades realmente implementadas.
- [ ] Las respuestas HTTP muestran los códigos y mensajes correspondientes.
- [ ] Los datos de las capturas coinciden con los registros consultables en MongoDB Compass.
- [ ] Las pruebas automatizadas mostradas corresponden a ejecuciones reales.
- [ ] No se presentan resultados simulados como si fueran ejecuciones reales.

---

## 5. Relación con la evaluación

Las evidencias están distribuidas para demostrar los siguientes aspectos del proyecto:

| Criterio | Evidencias principales | Puntuación |
|---|---|---:|
| Criterio 1 | MongoDB Atlas, modelo BSON, colecciones e índices. | 20 pts |
| Criterio 2 | Operaciones CRUD y persistencia de información. | 25 pts |
| Criterio 3 | Seguridad, permisos y protección de credenciales. | 15 pts |
| Criterio 4 | Compilación, ejecución y conectividad del backend. | 20 pts |
| Criterio 5 | Frontend, funcionalidades y pruebas automatizadas. | 20 pts |
| **Total** | | **100 pts** |

---

**Nota:** Este índice funciona como guía de organización y verificación. La presencia de un nombre de archivo en el catálogo no sustituye la captura de evidencia correspondiente.