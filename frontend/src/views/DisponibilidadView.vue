<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue';
import { useRouter } from 'vue-router';
import StatusBadge from '@/components/ui/StatusBadge.vue';
import { disponibilidadApi } from '@/services/disponibilidadApi';
import { ApiClientError } from '@/services/apiClient';
import { useNotificationStore } from '@/stores/notifications';
import type {
  DisponibilidadResponse,
  HabitacionDisponible,
  TipoHabitacion,
} from '@/types/hotel';

const router = useRouter();
const notifications = useNotificationStore();

function getTodayIso(): string {
  const now = new Date();
  const y = now.getFullYear();
  const m = String(now.getMonth() + 1).padStart(2, '0');
  const d = String(now.getDate()).padStart(2, '0');
  return `${y}-${m}-${d}`;
}

function addDaysIso(baseIso: string, days: number): string {
  const [y, m, d] = baseIso.split('-').map(Number);
  const dt = new Date(Date.UTC(y, m - 1, d + days));
  const ry = dt.getUTCFullYear();
  const rm = String(dt.getUTCMonth() + 1).padStart(2, '0');
  const rd = String(dt.getUTCDate()).padStart(2, '0');
  return `${ry}-${rm}-${rd}`;
}

const defaultEntrada = getTodayIso();
const defaultSalida = addDaysIso(defaultEntrada, 2);

const form = reactive({
  fechaEntrada: defaultEntrada,
  fechaSalida: defaultSalida,
  personas: '',
  tipo: '' as TipoHabitacion | '',
});

const loading = ref(false);
const hasSearched = ref(false);
const validationError = ref<string | null>(null);
const result = ref<DisponibilidadResponse | null>(null);

function formatCurrency(value: number): string {
  return new Intl.NumberFormat('es-MX', {
    style: 'currency',
    currency: 'MXN',
    minimumFractionDigits: 2,
  }).format(value);
}

function capitalize(value: string): string {
  if (!value) return '';
  return value.charAt(0).toUpperCase() + value.slice(1);
}

async function searchAvailability(): Promise<void> {
  validationError.value = null;

  if (!form.fechaEntrada || !form.fechaSalida) {
    validationError.value = 'Selecciona tanto la fecha de entrada como la fecha de salida.';
    return;
  }

  if (form.fechaSalida <= form.fechaEntrada) {
    validationError.value = 'La fecha de salida debe ser estrictamente posterior a la de entrada.';
    return;
  }

  let personasVal: number | undefined;
  if (form.personas.trim() !== '') {
    const parsed = Number(form.personas);
    if (!Number.isInteger(parsed) || parsed < 1 || parsed > 20) {
      validationError.value = 'El número de personas debe ser un entero entre 1 y 20.';
      return;
    }
    personasVal = parsed;
  }

  loading.value = true;
  hasSearched.value = true;

  try {
    result.value = await disponibilidadApi.consultar({
      fechaEntrada: form.fechaEntrada,
      fechaSalida: form.fechaSalida,
      personas: personasVal,
      tipo: form.tipo || undefined,
    });
  } catch (err) {
    result.value = null;
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible consultar la disponibilidad.';
    notifications.error('Error al consultar disponibilidad', message);
  } finally {
    loading.value = false;
  }
}

function jumpToReserve(hab: HabitacionDisponible): void {
  const personasQuery = form.personas.trim()
    ? String(Math.min(Number(form.personas), hab.capacidad))
    : '1';

  void router.push({
    name: 'reservaciones',
    query: {
      action: 'new',
      habitacionId: hab.id,
      fechaEntrada: result.value?.fechaEntrada ?? form.fechaEntrada,
      fechaSalida: result.value?.fechaSalida ?? form.fechaSalida,
      personas: personasQuery,
    },
  });
}

onMounted(() => {
  void searchAvailability();
});
</script>

<template>
  <div class="space-y-6">
    <!-- Buscador -->
    <section class="rounded-xl border border-slate-200 bg-white p-5 shadow-sm">
      <div class="mb-4">
        <h2 class="text-base font-bold text-slate-900">Buscador de Disponibilidad</h2>
        <p class="text-xs text-slate-500">
          Filtra habitaciones operativas libres de traslapes en el intervalo [entrada, salida). El día de salida permite nuevo check-in.
        </p>
      </div>

      <form
        class="grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-5"
        @submit.prevent="searchAvailability"
      >
        <div>
          <label class="block text-xs font-semibold text-slate-700" for="disp-entrada">
            Fecha de entrada *
          </label>
          <input
            id="disp-entrada"
            v-model="form.fechaEntrada"
            type="date"
            required
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="disp-salida">
            Fecha de salida *
          </label>
          <input
            id="disp-salida"
            v-model="form.fechaSalida"
            type="date"
            required
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="disp-personas">
            Personas (opcional)
          </label>
          <input
            id="disp-personas"
            v-model="form.personas"
            type="number"
            min="1"
            max="20"
            placeholder="Cualquiera"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="disp-tipo">
            Tipo (opcional)
          </label>
          <select
            id="disp-tipo"
            v-model="form.tipo"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="">Todos los tipos</option>
            <option value="sencilla">Sencilla</option>
            <option value="doble">Doble</option>
            <option value="triple">Triple</option>
            <option value="suite">Suite</option>
            <option value="familiar">Familiar</option>
          </select>
        </div>

        <div class="flex items-end">
          <button
            type="submit"
            :disabled="loading"
            class="w-full rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-700 disabled:opacity-50"
          >
            {{ loading ? 'Buscando...' : 'Consultar' }}
          </button>
        </div>
      </form>

      <p v-if="validationError" class="mt-3 text-xs font-medium text-rose-600">
        {{ validationError }}
      </p>
    </section>

    <!-- Resumen de resultados -->
    <section v-if="result && !loading" class="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-indigo-100 bg-indigo-50/70 px-5 py-3.5 text-sm">
      <div class="flex flex-wrap items-center gap-x-5 gap-y-1 text-slate-700">
        <span>
          Estancia: <strong class="text-slate-900">{{ result.fechaEntrada }}</strong> a
          <strong class="text-slate-900">{{ result.fechaSalida }}</strong>
        </span>
        <span>
          Noches: <strong class="text-slate-900">{{ result.noches }}</strong>
        </span>
      </div>
      <span class="rounded-full bg-indigo-600 px-3 py-1 text-xs font-semibold text-white">
        {{ result.totalDisponibles }}
        {{ result.totalDisponibles === 1 ? 'habitación disponible' : 'habitaciones disponibles' }}
      </span>
    </section>

    <!-- Estado cargando -->
    <section v-if="loading" class="grid grid-cols-1 gap-4 md:grid-cols-2 xl:grid-cols-3">
      <div
        v-for="n in 3"
        :key="`sk-${n}`"
        class="h-52 animate-pulse rounded-xl border border-slate-200 bg-white p-5"
      >
        <div class="h-5 w-1/3 rounded bg-slate-200" />
        <div class="mt-4 h-4 w-2/3 rounded bg-slate-200" />
        <div class="mt-6 h-10 w-full rounded bg-slate-200" />
      </div>
    </section>

    <!-- Estado vacío -->
    <section
      v-else-if="hasSearched && result && result.habitaciones.length === 0"
      class="rounded-xl border border-slate-200 bg-white px-6 py-12 text-center shadow-sm"
    >
      <p class="text-sm font-semibold text-slate-900">
        No hay habitaciones disponibles para los criterios elegidos
      </p>
      <p class="mt-1 text-xs text-slate-500">
        Prueba ampliando el rango de fechas, quitando el filtro de tipo o ajustando la capacidad solicitada.
      </p>
    </section>

    <!-- Tarjetas de habitaciones disponibles -->
    <section
      v-else-if="result && result.habitaciones.length > 0"
      class="grid grid-cols-1 gap-4 md:grid-cols-2 xl:grid-cols-3"
    >
      <article
        v-for="hab in result.habitaciones"
        :key="hab.id"
        class="flex flex-col justify-between rounded-xl border border-slate-200 bg-white p-5 shadow-sm transition hover:border-indigo-300"
      >
        <div>
          <div class="flex items-start justify-between gap-2">
            <div>
              <h3 class="text-lg font-bold text-slate-900">Habitación #{{ hab.numero }}</h3>
              <p class="text-xs font-medium text-slate-500">
                {{ capitalize(hab.tipo) }} · Hasta {{ hab.capacidad }}
                {{ hab.capacidad === 1 ? 'persona' : 'personas' }}
              </p>
            </div>
            <StatusBadge :status="hab.estado" />
          </div>

          <div class="mt-4 flex flex-wrap gap-1.5">
            <span
              v-for="srv in hab.servicios"
              :key="srv"
              class="rounded-md bg-slate-100 px-2 py-0.5 text-xs text-slate-700"
            >
              {{ srv }}
            </span>
            <span v-if="hab.servicios.length === 0" class="text-xs text-slate-400">
              Sin servicios adicionales registrados
            </span>
          </div>
        </div>

        <div class="mt-5 border-t border-slate-100 pt-4">
          <div class="flex items-baseline justify-between">
            <span class="text-xs text-slate-500">
              {{ formatCurrency(hab.tarifa) }} × {{ hab.noches }}
              {{ hab.noches === 1 ? 'noche' : 'noches' }}
            </span>
            <span class="text-lg font-bold text-indigo-700">
              {{ formatCurrency(hab.costoEstimado) }}
            </span>
          </div>

          <button
            type="button"
            class="mt-3 w-full rounded-lg bg-slate-900 px-4 py-2 text-sm font-semibold text-white transition hover:bg-indigo-600"
            @click="jumpToReserve(hab)"
          >
            Reservar esta habitación →
          </button>
        </div>
      </article>
    </section>
  </div>
</template>