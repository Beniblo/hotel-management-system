import { apiClient, cleanQueryParams } from './apiClient';
import type { PagedResponse } from '@/types/api';
import type {
  CreateReservacionPayload,
  Reservacion,
  ReservacionQuery,
  UpdateReservacionPayload,
} from '@/types/hotel';

export const reservacionesApi = {
  async list(query: ReservacionQuery = {}): Promise<PagedResponse<Reservacion>> {
    const { data } = await apiClient.get<PagedResponse<Reservacion>>(
      '/reservaciones',
      { params: cleanQueryParams(query) }
    );
    return data;
  },

  async getById(id: string): Promise<Reservacion> {
    const { data } = await apiClient.get<Reservacion>(`/reservaciones/${id}`);
    return data;
  },

  async create(payload: CreateReservacionPayload): Promise<Reservacion> {
    const { data } = await apiClient.post<Reservacion>(
      '/reservaciones',
      payload
    );
    return data;
  },

  async update(
    id: string,
    payload: UpdateReservacionPayload
  ): Promise<Reservacion> {
    const { data } = await apiClient.patch<Reservacion>(
      `/reservaciones/${id}`,
      payload
    );
    return data;
  },

  async cancel(id: string): Promise<Reservacion> {
    const { data } = await apiClient.post<Reservacion>(
      `/reservaciones/${id}/cancelar`
    );
    return data;
  },

  async remove(id: string): Promise<void> {
    await apiClient.delete(`/reservaciones/${id}`);
  },
};