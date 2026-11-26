// Script de inicializacion de HotelDB para ejecutar en MongoDB Compass (_MONGOSH)
// Uso desde _MONGOSH:
//   load("C:/Proyectos/hotel-management-system/docs/init_hoteldb.js")

const hotelDb = db.getSiblingDB("HotelDB");

// 1. Crear colecciones explicitas si no existen
const existentes = hotelDb.getCollectionNames();
for (const nombre of ["huespedes", "habitaciones", "reservaciones"]) {
  if (!existentes.includes(nombre)) {
    hotelDb.createCollection(nombre);
    print("[OK] Coleccion creada: " + nombre);
  } else {
    print("[INFO] Coleccion ya existente: " + nombre);
  }
}

// 2. Crear indices obligatorios
hotelDb.huespedes.createIndex({ correo: 1 }, { unique: true, name: "ux_huespedes_correo" });
hotelDb.huespedes.createIndex({ activo: 1, nombre: 1 }, { name: "ix_huespedes_activo_nombre" });

hotelDb.habitaciones.createIndex({ numero: 1 }, { unique: true, name: "ux_habitaciones_numero" });
hotelDb.habitaciones.createIndex({ estado: 1, tipo: 1, capacidad: 1 }, { name: "ix_habitaciones_busqueda" });

hotelDb.reservaciones.createIndex(
  { habitacionId: 1, estado: 1, fechaEntrada: 1, fechaSalida: 1 },
  { name: "ix_reservaciones_traslape" }
);
hotelDb.reservaciones.createIndex({ huespedId: 1 }, { name: "ix_reservaciones_huesped" });
hotelDb.reservaciones.createIndex({ fechaEntrada: 1, fechaSalida: 1 }, { name: "ix_reservaciones_fechas" });
hotelDb.reservaciones.createIndex({ estado: 1 }, { name: "ix_reservaciones_estado" });

print("[OK] Indices verificados/creados en HotelDB.");

// 3. Cargar datos semilla ficticios solo si las colecciones estan vacias
if (
  hotelDb.huespedes.countDocuments() === 0 &&
  hotelDb.habitaciones.countDocuments() === 0 &&
  hotelDb.reservaciones.countDocuments() === 0
) {
  const ahora = new Date();

  const h1 = hotelDb.huespedes.insertOne({
    nombre: "Ana Lopez",
    correo: "ana.lopez@ejemplo.com",
    telefono: "4770000001",
    fechaRegistro: ahora,
    preferencias: ["piso alto", "almohada extra"],
    activo: true,
    actualizadoEn: ahora
  });

  const h2 = hotelDb.huespedes.insertOne({
    nombre: "Carlos Mendoza",
    correo: "carlos.mendoza@ejemplo.com",
    telefono: "4770000002",
    fechaRegistro: ahora,
    preferencias: ["silenciosa"],
    activo: true,
    actualizadoEn: ahora
  });

  const hab1 = hotelDb.habitaciones.insertOne({
    numero: "101",
    tipo: "sencilla",
    capacidad: 1,
    tarifa: 950.0,
    estado: "disponible",
    servicios: ["wifi", "tv", "aire_acondicionado"],
    actualizadoEn: ahora
  });

  const hab2 = hotelDb.habitaciones.insertOne({
    numero: "102",
    tipo: "doble",
    capacidad: 2,
    tarifa: 1450.0,
    estado: "disponible",
    servicios: ["wifi", "tv", "aire_acondicionado", "caja_fuerte"],
    actualizadoEn: ahora
  });

  const hab3 = hotelDb.habitaciones.insertOne({
    numero: "201",
    tipo: "suite",
    capacidad: 4,
    tarifa: 2600.0,
    estado: "disponible",
    servicios: ["wifi", "tv", "aire_acondicionado", "frigobar", "balcon"],
    actualizadoEn: ahora
  });

  hotelDb.reservaciones.insertOne({
    huespedId: h1.insertedId,
    habitacionId: hab2.insertedId,
    fechaEntrada: ISODate("2026-12-05T00:00:00Z"),
    fechaSalida: ISODate("2026-12-08T00:00:00Z"),
    personas: 2,
    estado: "confirmada",
    tarifaAplicada: 1450.0,
    total: 4350.0,
    creadoEn: ahora,
    actualizadoEn: ahora
  });

  print("[OK] Datos semilla ficticios insertados correctamente.");
} else {
  print("[INFO] Ya existen documentos en HotelDB; se omitio la carga semilla.");
}

// 4. Resumen de verificacion
print("--- RESUMEN HotelDB ---");
print("huespedes: " + hotelDb.huespedes.countDocuments());
print("habitaciones: " + hotelDb.habitaciones.countDocuments());
print("reservaciones: " + hotelDb.reservaciones.countDocuments());