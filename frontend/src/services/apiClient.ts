import axios, { AxiosError } from 'axios';
import type { ApiErrorPayload, FieldError } from '@/types/api';

export class ApiClientError extends Error {
  readonly status: number;
  readonly errorCode: string;
  readonly requestId: string;
  readonly details: FieldError[];

  constructor(
    message: string,
    status: number,
    errorCode = 'UNKNOWN_ERROR',
    requestId = '',
    details: FieldError[] = []
  ) {
    super(message);
    this.name = 'ApiClientError';
    this.status = status;
    this.errorCode = errorCode;
    this.requestId = requestId;
    this.details = details;
  }

  get fieldErrorsMap(): Record<string, string> {
    const map: Record<string, string> = {};
    for (const item of this.details) {
      map[item.field] = item.message;
    }
    return map;
  }
}

export const apiClient = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || 'http://127.0.0.1:8080/api/v1',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
    Accept: 'application/json',
  },
});

apiClient.interceptors.response.use(
  (response) => response,
  (error: AxiosError<ApiErrorPayload>) => {
    if (error.response) {
      const payload = error.response.data;
      const status = payload?.status ?? error.response.status;
      const message =
        payload?.message ||
        `Error HTTP ${status} al comunicarse con el servidor.`;
      const errorCode = payload?.errorCode || `HTTP_${status}`;
      const requestId = payload?.requestId || '';
      const details = Array.isArray(payload?.details) ? payload.details : [];

      return Promise.reject(
        new ApiClientError(message, status, errorCode, requestId, details)
      );
    }

    if (error.code === 'ECONNABORTED') {
      return Promise.reject(
        new ApiClientError(
          'Tiempo de espera agotado al conectar con la API.',
          504,
          'TIMEOUT'
        )
      );
    }

    return Promise.reject(
      new ApiClientError(
        'No se pudo conectar con el servidor backend. Verifica que la API C++ esté en ejecución.',
        0,
        'NETWORK_ERROR'
      )
    );
  }
);

export function cleanQueryParams<T extends object>(
  params: T
): Record<string, string | number | boolean> {
  const cleaned: Record<string, string | number | boolean> = {};
  for (const [key, value] of Object.entries(params)) {
    if (value !== undefined && value !== null && value !== '') {
      cleaned[key] = value as string | number | boolean;
    }
  }
  return cleaned;
}