export interface FieldError {
  field: string;
  message: string;
}

export interface ApiErrorPayload {
  timestamp: string;
  status: number;
  errorCode: string;
  message: string;
  requestId: string;
  details?: FieldError[];
}

export interface PaginationMeta {
  total: number;
  page: number;
  limit: number;
  totalPages: number;
}

export interface PagedResponse<T> {
  data: T[];
  meta: PaginationMeta;
}

export interface HealthResponse {
  status: 'ok' | 'degraded';
  timestamp: string;
  database: {
    status: 'connected' | 'disconnected';
    name: string;
  };
}