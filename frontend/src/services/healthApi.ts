import { apiClient } from './apiClient';
import type { HealthResponse } from '@/types/api';

export const healthApi = {
  async check(): Promise<HealthResponse> {
    const { data } = await apiClient.get<HealthResponse>('/health');
    return data;
  },
};