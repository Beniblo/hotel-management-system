import { apiClient, cleanQueryParams } from './apiClient';
import type {
  DisponibilidadQuery,
  DisponibilidadResponse,
} from '@/types/hotel';

export const disponibilidadApi = {
  async consultar(query: DisponibilidadQuery): Promise<DisponibilidadResponse> {
    const { data } = await apiClient.get<DisponibilidadResponse>(
      '/disponibilidad',
      { params: cleanQueryParams(query) }
    );
    return data;
  },
};