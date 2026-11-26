<script setup lang="ts">
import { onMounted, ref } from 'vue';
import StatusBadge from '@/components/ui/StatusBadge.vue';
import { healthApi } from '@/services/healthApi';
import { reportesApi } from '@/services/reportesApi';
import { ApiClientError } from '@/services/apiClient';
import { useNotificationStore } from '@/stores/notifications';
import type { HealthResponse } from '@/types/api';
import type { ResumenReporte } from '@/types/hotel';

const notifications = useNotificationStore();

const loading = ref(true);
const health = ref<HealthResponse | null>(null);
const healthError = ref<string | null>(null);
const resumen = ref<ResumenReporte | null>(null);
const resumenError = ref<string | null>(null);

function formatCurrency(value: number): string {
  return new Intl.NumberFormat('es-MX', {
    style: 'currency',
    currency: 'MXN',
    minimumFractionDigits: 2,
  }).format(value);
}

function getTodayIsoDate(): string {
  const now = new Date();
  const y = now.getFullYear();
  const m = String(now.getMonth() + 1).padStart(2, '0');
  const d = String(now.getDate()).padStart(2, '0');
  return `${y}-${m}-${d}`;
}

function addDaysIsoDate(baseIso: string, days: number): string {
  const [y, m, d] = baseIso.split('-').map(Number);
  const dt = new Date(Date.UTC(y, m - 1, d + days));
  const ry = dt.getUTCFullYear();
  const rm = String(dt.getUTCMonth() + 1).padStart(2, '0');
  const rd = String(dt.getUTCDate()).padStart(2, '0');
  return `${ry}-${rm}-${rd}`;
}

async function loadDashboard(): Promise<void> {
  loading.value = true;
  healthError.value = null;
  resumenError.value = null;

  const fechaInicio = getTodayIsoDate();
  const fechaFin = addDaysIsoDate(fechaInicio, 30);

  try {
    health.value = await healthApi.check();
  } catch (err) {
    health.value = null;
    healthError.value =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible consultar el estado de salud de la API.';
  }

  try {
    resumen.value = await reportesApi.resumen({ fechaInicio, fechaFin });
  } catch (err) {
    resumen.value = null;
    resumenError.value =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible cargar las métricas operativas.';
  } finally {
    loading.value = false;
  }
}

async function refreshManual(): Promise<void> {
  await loadDashboard();
  if (health.value?.status === 'ok') {
    notifications.info('Estado actualizado', 'Conectividad con la API y MongoDB Atlas verificada.');
  }
}

onMounted(() => {
  void loadDashboard();
});
</script>

<template>
  <div class="space-y-6">
    <!-- Tarjeta de estado del backend y MongoDB Atlas -->
    <section class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
      <div class="flex flex-wrap items-center justify-between gap-4">
        <div class="space-y-1">
          <div class="flex items-center gap-2.5">
            <h2 class="text-base font-bold text-slate-900">Estado del Sistema y Base de Datos</h2>
            <StatusBadge
              v-if="health"
              :status="health.status"
              :label="health.status === 'ok' ? 'API y Atlas en línea' : 'Atlas degradado'"
            />
            <StatusBadge
              v-else-if="!loading"
              status="degraded"
              label="Backend no disponible"
            />
          </div>
          <p v-if="health" class="text-xs text-slate-500">
            Base de datos: <strong class="text-slate-700">{{ health.database.name }}</strong>
            ({{ health.database.status }}) · Última verificación UTC: {{ health.timestamp }}
          </p>
          <p v-else-if="healthError" class="text-xs text-rose-600">
            {{ healthError }}
          </p>
          <p v-else class="text-xs text-slate-500">Consultando /api/v1/health...</p>
        </div>

        <button
          type="button"
          class="rounded-lg border border-slate-300 bg-white px-3.5 py-2 text-xs font-semibold text-slate-700 transition hover:bg-slate-50 disabled:opacity-50"
          :disabled="loading"
          @click="refreshManual"
        >
          {{ loading ? 'Verificando...' : 'Actualizar estado' }}
        </button>
      </div>
    </section>

    <!-- KPIs operativos (ventana de 30 días desde hoy) -->
    <section>
      <div class="mb-3 flex items-center justify-between">
        <h2 class="text-sm font-semibold uppercase tracking-wider text-slate-500">
          Indicadores Operativos (Próximos 30 días)
        </h2>
        <span v-if="resumen" class="text-xs text-slate-500">
          Periodo: {{ resumen.fechaInicio }} a {{ resumen.fechaFin }}
        </span>
      </div>

      <div v-if="resumenError" class="rounded-xl border border-amber-200 bg-amber-50 p-4 text-sm text-amber-900">
        {{ resumenError }}
      </div>

      <div v-else class="grid grid-cols-1 gap-4 sm:grid-cols-2 xl:grid-cols-4">
        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Habitaciones Disponibles</p>
          <p class="mt-2 text-2xl font-bold text-emerald-700">
            {{ loading ? '—' : (resumen?.habitacionesDisponibles ?? 0) }}
            <span class="text-sm font-normal text-slate-400">
              / {{ resumen?.totalHabitaciones ?? 0 }}
            </span>
          </p>
          <p class="mt-1 text-xs text-slate-500">
            Ocupadas en periodo: {{ resumen?.habitacionesOcupadas ?? 0 }}
          </p>
        </article>

        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Ocupación del Periodo</p>
          <p class="mt-2 text-2xl font-bold text-indigo-700">
            {{ loading ? '—' : `${(resumen?.porcentajeOcupacion ?? 0).toFixed(2)}%` }}
          </p>
          <p class="mt-1 text-xs text-slate-500">
            No operativas: {{ resumen?.habitacionesNoOperativas ?? 0 }}
          </p>
        </article>

        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Reservaciones en Periodo</p>
          <p class="mt-2 text-2xl font-bold text-slate-900">
            {{ loading ? '—' : (resumen?.totalReservaciones ?? 0) }}
          </p>
          <p class="mt-1 text-xs text-slate-500">
            Confirmadas: {{ resumen?.reservacionesPorEstado?.confirmada ?? 0 }} ·
            Canceladas: {{ resumen?.reservacionesPorEstado?.cancelada ?? 0 }}
          </p>
        </article>

        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Ingresos Estimados</p>
          <p class="mt-2 text-2xl font-bold text-slate-900">
            {{ loading ? '—' : formatCurrency(resumen?.ingresosEstimados ?? 0) }}
          </p>
          <p class="mt-1 text-xs text-slate-500">Excluye reservaciones canceladas</p>
        </article>
      </div>
    </section>

    <!-- Accesos directos -->
    <section class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
      <h2 class="text-base font-bold text-slate-900">Accesos Rápidos</h2>
      <p class="mt-1 text-xs text-slate-500">
        Administra huéspedes, catálogo de habitaciones, reservaciones, disponibilidad y reportes.
      </p>

      <div class="mt-4 grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-3">
        <RouterLink
          to="/huespedes"
          class="rounded-lg border border-slate-200 p-4 transition hover:border-indigo-300 hover:bg-indigo-50/40"
        >
          <p class="text-sm font-semibold text-slate-900">Gestión de Huéspedes</p>
          <p class="mt-1 text-xs text-slate-500">Altas, edición, preferencias, baja lógica y borrado controlado.</p>
        </RouterLink>

        <RouterLink
          to="/disponibilidad"
          class="rounded-lg border border-slate-200 p-4 transition hover:border-indigo-300 hover:bg-indigo-50/40"
        >
          <p class="text-sm font-semibold text-slate-900">Buscador de Disponibilidad</p>
          <p class="mt-1 text-xs text-slate-500">Consulta habitaciones libres por fechas y capacidad sin traslapes.</p>
        </RouterLink>

        <RouterLink
          to="/reservaciones"
          class="rounded-lg border border-slate-200 p-4 transition hover:border-indigo-300 hover:bg-indigo-50/40"
        >
          <p class="text-sm font-semibold text-slate-900">Reservaciones</p>
          <p class="mt-1 text-xs text-slate-500">Crea estancias con cálculo automático de total y cancela reservas.</p>
        </RouterLink>
      </div>
    </section>
  </div>
</template>