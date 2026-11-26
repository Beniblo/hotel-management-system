export type EstadoHabitacion =
  | 'disponible'
  | 'ocupada'
  | 'mantenimiento'
  | 'fuera_de_servicio';

export type TipoHabitacion =
  | 'sencilla'
  | 'doble'
  | 'triple'
  | 'suite'
  | 'familiar';

export type EstadoReservacion =
  | 'pendiente'
  | 'confirmada'
  | 'en_curso'
  | 'completada'
  | 'cancelada';

// --- Huéspedes ---
export interface Huesped {
  id: string;
  nombre: string;
  correo: string;
  telefono: string;
  fechaRegistro: string;
  preferencias: string[];
  activo: boolean;
  actualizadoEn: string;
}

export interface CreateHuespedPayload {
  nombre: string;
  correo: string;
  telefono: string;
  preferencias?: string[];
}

export interface UpdateHuespedPayload {
  nombre?: string;
  correo?: string;
  telefono?: string;
  preferencias?: string[];
  activo?: boolean;
}

export interface HuespedQuery {
  nombre?: string;
  correo?: string;
  activo?: boolean;
  page?: number;
  limit?: number;
}

// --- Habitaciones ---
export interface Habitacion {
  id: string;
  numero: string;
  tipo: TipoHabitacion | string;
  capacidad: number;
  tarifa: number;
  estado: EstadoHabitacion;
  servicios: string[];
  actualizadoEn: string;
}

export interface CreateHabitacionPayload {
  numero: string;
  tipo: TipoHabitacion;
  capacidad: number;
  tarifa: number;
  estado?: EstadoHabitacion;
  servicios?: string[];
}

export interface UpdateHabitacionPayload {
  numero?: string;
  tipo?: TipoHabitacion;
  capacidad?: number;
  tarifa?: number;
  estado?: EstadoHabitacion;
  servicios?: string[];
}

export interface HabitacionQuery {
  tipo?: TipoHabitacion | '';
  estado?: EstadoHabitacion | '';
  capacidadMin?: number;
  tarifaMax?: number;
  page?: number;
  limit?: number;
}

// --- Reservaciones ---
export interface Reservacion {
  id: string;
  huespedId: string;
  habitacionId: string;
  fechaEntrada: string;
  fechaSalida: string;
  noches: number;
  personas: number;
  estado: EstadoReservacion;
  tarifaAplicada: number;
  total: number;
  creadoEn: string;
  actualizadoEn: string;
}

export interface CreateReservacionPayload {
  huespedId: string;
  habitacionId: string;
  fechaEntrada: string;
  fechaSalida: string;
  personas: number;
  estado?: 'pendiente' | 'confirmada';
}

export interface UpdateReservacionPayload {
  habitacionId?: string;
  fechaEntrada?: string;
  fechaSalida?: string;
  personas?: number;
  estado?: EstadoReservacion;
}

export interface ReservacionQuery {
  huespedId?: string;
  habitacionId?: string;
  estado?: EstadoReservacion | '';
  fechaDesde?: string;
  fechaHasta?: string;
  page?: number;
  limit?: number;
}

// --- Disponibilidad ---
export interface HabitacionDisponible extends Habitacion {
  noches: number;
  costoEstimado: number;
}

export interface DisponibilidadResponse {
  fechaEntrada: string;
  fechaSalida: string;
  noches: number;
  totalDisponibles: number;
  habitaciones: HabitacionDisponible[];
}

export interface DisponibilidadQuery {
  fechaEntrada: string;
  fechaSalida: string;
  personas?: number;
  tipo?: TipoHabitacion | '';
}

// --- Reportes ---
export interface ResumenReporte {
  fechaInicio: string;
  fechaFin: string;
  totalReservaciones: number;
  reservacionesPorEstado: Record<EstadoReservacion, number>;
  totalHabitaciones: number;
  habitacionesOcupadas: number;
  habitacionesDisponibles: number;
  habitacionesNoOperativas: number;
  porcentajeOcupacion: number;
  ingresosEstimados: number;
}

export interface ReporteQuery {
  fechaInicio: string;
  fechaFin: string;
}