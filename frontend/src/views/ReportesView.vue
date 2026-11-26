<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue';
import StatusBadge from '@/components/ui/StatusBadge.vue';
import { reportesApi } from '@/services/reportesApi';
import { ApiClientError } from '@/services/apiClient';
import { useNotificationStore } from '@/stores/notifications';
import type { EstadoReservacion, ResumenReporte } from '@/types/hotel';

const notifications = useNotificationStore();

function getMonthBounds(year: number, monthZeroBased: number): {
  inicio: string;
  fin: string;
} {
  const first = new Date(Date.UTC(year, monthZeroBased, 1));
  const last = new Date(Date.UTC(year, monthZeroBased + 1, 0));
  const fmt = (d: Date) =>
    `${d.getUTCFullYear()}-${String(d.getUTCMonth() + 1).padStart(2, '0')}-${String(d.getUTCDate()).padStart(2, '0')}`;
  return { inicio: fmt(first), fin: fmt(last) };
}

function calculateDaysDiff(inicio: string, fin: string): number {
  const p1 = inicio.split('-').map(Number);
  const p2 = fin.split('-').map(Number);
  if (p1.length !== 3 || p2.length !== 3) return -1;
  const t1 = Date.UTC(p1[0], p1[1] - 1, p1[2]);
  const t2 = Date.UTC(p2[0], p2[1] - 1, p2[2]);
  return Math.round((t2 - t1) / (24 * 60 * 60 * 1000));
}

const now = new Date();
const defaultRange = getMonthBounds(now.getFullYear(), now.getMonth());

const form = reactive({
  fechaInicio: defaultRange.inicio,
  fechaFin: defaultRange.fin,
});

const loading = ref(false);
const validationError = ref<string | null>(null);
const reporte = ref<ResumenReporte | null>(null);

const estadosOrdenados: { key: EstadoReservacion; label: string; desc: string }[] = [
  { key: 'pendiente', label: 'Pendiente', desc: 'En espera de confirmación' },
  { key: 'confirmada', label: 'Confirmada', desc: 'Reserva asegurada' },
  { key: 'en_curso', label: 'En curso', desc: 'Huésped hospedado actualmente' },
  { key: 'completada', label: 'Completada', desc: 'Estancia finalizada' },
  { key: 'cancelada', label: 'Cancelada', desc: 'Excluida de ingresos estimados' },
];

const clampedOccupancy = computed(() => {
  const pct = reporte.value?.porcentajeOcupacion ?? 0;
  return Math.max(0, Math.min(100, pct));
});

function formatCurrency(value: number): string {
  return new Intl.NumberFormat('es-MX', {
    style: 'currency',
    currency: 'MXN',
    minimumFractionDigits: 2,
  }).format(value);
}

async function generateReport(): Promise<void> {
  validationError.value = null;

  if (!form.fechaInicio || !form.fechaFin) {
    validationError.value = 'Selecciona la fecha inicial y la fecha final del periodo.';
    return;
  }

  const diffDays = calculateDaysDiff(form.fechaInicio, form.fechaFin);
  if (diffDays < 0) {
    validationError.value = 'La fecha final debe ser mayor o igual a la fecha inicial.';
    return;
  }
  if (diffDays > 366) {
    validationError.value = 'El periodo máximo permitido por consulta es de 366 días.';
    return;
  }

  loading.value = true;
  try {
    reporte.value = await reportesApi.resumen({
      fechaInicio: form.fechaInicio,
      fechaFin: form.fechaFin,
    });
  } catch (err) {
    reporte.value = null;
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible generar el reporte administrativo.';
    notifications.error('Error al generar reporte', message);
  } finally {
    loading.value = false;
  }
}

function applyPreset(preset: 'currentMonth' | 'nextMonth' | 'dec2026'): void {
  const today = new Date();
  if (preset === 'currentMonth') {
    const r = getMonthBounds(today.getFullYear(), today.getMonth());
    form.fechaInicio = r.inicio;
    form.fechaFin = r.fin;
  } else if (preset === 'nextMonth') {
    const r = getMonthBounds(today.getFullYear(), today.getMonth() + 1);
    form.fechaInicio = r.inicio;
    form.fechaFin = r.fin;
  } else {
    form.fechaInicio = '2026-12-01';
    form.fechaFin = '2026-12-31';
  }
  void generateReport();
}

onMounted(() => {
  void generateReport();
});
</script>

<template>
  <div class="space-y-6">
    <!-- Selector de periodo -->
    <section class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
      <div class="flex flex-wrap items-start justify-between gap-4">
        <div>
          <h2 class="text-base font-bold text-slate-900">Reporte Operativo e Ingresos</h2>
          <p class="text-xs text-slate-500">
            Consolida ocupación, reservaciones por estado e ingresos estimados desde MongoDB Atlas.
          </p>
        </div>

        <div class="flex flex-wrap gap-2">
          <button
            type="button"
            class="rounded-lg border border-slate-200 bg-slate-50 px-3 py-1.5 text-xs font-medium text-slate-700 transition hover:bg-slate-100"
            @click="applyPreset('currentMonth')"
          >
            Mes actual
          </button>
          <button
            type="button"
            class="rounded-lg border border-slate-200 bg-slate-50 px-3 py-1.5 text-xs font-medium text-slate-700 transition hover:bg-slate-100"
            @click="applyPreset('nextMonth')"
          >
            Próximo mes
          </button>
          <button
            type="button"
            class="rounded-lg border border-indigo-200 bg-indigo-50 px-3 py-1.5 text-xs font-medium text-indigo-700 transition hover:bg-indigo-100"
            @click="applyPreset('dec2026')"
          >
            Diciembre 2026
          </button>
        </div>
      </div>

      <form
        class="mt-4 grid grid-cols-1 gap-3 sm:grid-cols-3"
        @submit.prevent="generateReport"
      >
        <div>
          <label class="block text-xs font-semibold text-slate-700" for="rep-inicio">
            Fecha de inicio *
          </label>
          <input
            id="rep-inicio"
            v-model="form.fechaInicio"
            type="date"
            required
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="rep-fin">
            Fecha de fin *
          </label>
          <input
            id="rep-fin"
            v-model="form.fechaFin"
            type="date"
            required
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div class="flex items-end">
          <button
            type="submit"
            :disabled="loading"
            class="w-full rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-700 disabled:opacity-50"
          >
            {{ loading ? 'Calculando...' : 'Generar reporte' }}
          </button>
        </div>
      </form>

      <p v-if="validationError" class="mt-3 text-xs font-medium text-rose-600">
        {{ validationError }}
      </p>
    </section>

    <!-- Tarjetas de KPIs -->
    <section
      v-if="loading"
      class="grid grid-cols-1 gap-4 sm:grid-cols-2 xl:grid-cols-4"
    >
      <div
        v-for="n in 4"
        :key="`sk-rep-${n}`"
        class="h-28 animate-pulse rounded-xl border border-slate-200 bg-white p-5"
      >
        <div class="h-4 w-1/2 rounded bg-slate-200" />
        <div class="mt-4 h-7 w-2/3 rounded bg-slate-200" />
      </div>
    </section>

    <template v-else-if="reporte">
      <section class="grid grid-cols-1 gap-4 sm:grid-cols-2 xl:grid-cols-4">
        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Total de Reservaciones</p>
          <p class="mt-2 text-3xl font-bold text-slate-900">
            {{ reporte.totalReservaciones }}
          </p>
          <p class="mt-1 text-xs text-slate-500">
            Periodo {{ reporte.fechaInicio }} a {{ reporte.fechaFin }}
          </p>
        </article>

        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Tasa de Ocupación</p>
          <p class="mt-2 text-3xl font-bold text-indigo-700">
            {{ reporte.porcentajeOcupacion.toFixed(2) }}%
          </p>
          <div class="mt-2 h-2 w-full overflow-hidden rounded-full bg-slate-100">
            <div
              class="h-full rounded-full bg-indigo-600 transition-all"
              :style="{ width: `${clampedOccupancy}%` }"
            />
          </div>
        </article>

        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Ocupadas vs Disponibles</p>
          <p class="mt-2 text-3xl font-bold text-emerald-700">
            {{ reporte.habitacionesOcupadas }}
            <span class="text-base font-normal text-slate-400">
              / {{ reporte.totalHabitaciones }}
            </span>
          </p>
          <p class="mt-1 text-xs text-slate-500">
            Disponibles: {{ reporte.habitacionesDisponibles }} · No operativas:
            {{ reporte.habitacionesNoOperativas }}
          </p>
        </article>

        <article class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
          <p class="text-xs font-medium text-slate-500">Ingresos Estimados</p>
          <p class="mt-2 text-3xl font-bold text-slate-900">
            {{ formatCurrency(reporte.ingresosEstimados) }}
          </p>
          <p class="mt-1 text-xs text-slate-500">
            Incluye reservas activas/completadas (sin canceladas)
          </p>
        </article>
      </section>

      <!-- Desglose por estado -->
      <section class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
        <h3 class="text-sm font-bold text-slate-900">
          Desglose de Reservaciones por Estado
        </h3>
        <div class="mt-4 grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-5">
          <div
            v-for="item in estadosOrdenados"
            :key="item.key"
            class="rounded-xl border border-slate-200 bg-slate-50/70 p-4"
          >
            <div class="flex items-center justify-between gap-2">
              <StatusBadge :status="item.key" :label="item.label" />
              <span class="text-xl font-bold text-slate-900">
                {{ reporte.reservacionesPorEstado[item.key] ?? 0 }}
              </span>
            </div>
            <p class="mt-2 text-[11px] text-slate-500">{{ item.desc }}</p>
          </div>
        </div>
      </section>
    </template>
  </div>
</template>