import { apiClient, cleanQueryParams } from './apiClient';
import type { PagedResponse } from '@/types/api';
import type {
  CreateHuespedPayload,
  Huesped,
  HuespedQuery,
  UpdateHuespedPayload,
} from '@/types/hotel';

export const huespedesApi = {
  async list(query: HuespedQuery = {}): Promise<PagedResponse<Huesped>> {
    const { data } = await apiClient.get<PagedResponse<Huesped>>('/huespedes', {
      params: cleanQueryParams(query),
    });
    return data;
  },

  async getById(id: string): Promise<Huesped> {
    const { data } = await apiClient.get<Huesped>(`/huespedes/${id}`);
    return data;
  },

  async create(payload: CreateHuespedPayload): Promise<Huesped> {
    const { data } = await apiClient.post<Huesped>('/huespedes', payload);
    return data;
  },

  async update(id: string, payload: UpdateHuespedPayload): Promise<Huesped> {
    const { data } = await apiClient.patch<Huesped>(`/huespedes/${id}`, payload);
    return data;
  },

  async remove(id: string, fisico = false): Promise<Huesped | void> {
    const { data } = await apiClient.delete<Huesped | void>(`/huespedes/${id}`, {
      params: fisico ? { fisico: true } : undefined,
    });
    return data;
  },
};