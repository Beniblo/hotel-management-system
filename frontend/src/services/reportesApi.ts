import { apiClient, cleanQueryParams } from './apiClient';
import type { ReporteQuery, ResumenReporte } from '@/types/hotel';

export const reportesApi = {
  async resumen(query: ReporteQuery): Promise<ResumenReporte> {
    const { data } = await apiClient.get<ResumenReporte>('/reportes/resumen', {
      params: cleanQueryParams(query),
    });
    return data;
  },
};