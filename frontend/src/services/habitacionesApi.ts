import { apiClient, cleanQueryParams } from './apiClient';
import type { PagedResponse } from '@/types/api';
import type {
  CreateHabitacionPayload,
  EstadoHabitacion,
  Habitacion,
  HabitacionQuery,
  UpdateHabitacionPayload,
} from '@/types/hotel';

export const habitacionesApi = {
  async list(query: HabitacionQuery = {}): Promise<PagedResponse<Habitacion>> {
    const { data } = await apiClient.get<PagedResponse<Habitacion>>(
      '/habitaciones',
      { params: cleanQueryParams(query) }
    );
    return data;
  },

  async getById(id: string): Promise<Habitacion> {
    const { data } = await apiClient.get<Habitacion>(`/habitaciones/${id}`);
    return data;
  },

  async create(payload: CreateHabitacionPayload): Promise<Habitacion> {
    const { data } = await apiClient.post<Habitacion>('/habitaciones', payload);
    return data;
  },

  async update(
    id: string,
    payload: UpdateHabitacionPayload
  ): Promise<Habitacion> {
    const { data } = await apiClient.patch<Habitacion>(
      `/habitaciones/${id}`,
      payload
    );
    return data;
  },

  async updateEstado(
    id: string,
    estado: EstadoHabitacion
  ): Promise<Habitacion> {
    const { data } = await apiClient.patch<Habitacion>(
      `/habitaciones/${id}/estado`,
      { estado }
    );
    return data;
  },

  async remove(id: string): Promise<void> {
    await apiClient.delete(`/habitaciones/${id}`);
  },
};