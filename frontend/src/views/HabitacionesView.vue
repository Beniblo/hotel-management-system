<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue';
import DataTable, { type TableColumn } from '@/components/ui/DataTable.vue';
import StatusBadge from '@/components/ui/StatusBadge.vue';
import ConfirmDialog from '@/components/ui/ConfirmDialog.vue';
import HabitacionFormModal from '@/components/forms/HabitacionFormModal.vue';
import { habitacionesApi } from '@/services/habitacionesApi';
import { ApiClientError } from '@/services/apiClient';
import { useNotificationStore } from '@/stores/notifications';
import type { PaginationMeta } from '@/types/api';
import type {
  CreateHabitacionPayload,
  EstadoHabitacion,
  Habitacion,
  TipoHabitacion,
  UpdateHabitacionPayload,
} from '@/types/hotel';

const notifications = useNotificationStore();

const loading = ref(false);
const rows = ref<Habitacion[]>([]);
const meta = ref<PaginationMeta | null>(null);
const updatingEstadoId = ref<string | null>(null);

const filters = reactive({
  tipo: '' as TipoHabitacion | '',
  estado: '' as EstadoHabitacion | '',
  capacidadMin: '',
  tarifaMax: '',
  page: 1,
  limit: 10,
});

// Modal alta/edición
const formModalOpen = ref(false);
const selectedHabitacion = ref<Habitacion | null>(null);
const submittingForm = ref(false);
const serverFieldErrors = ref<Record<string, string>>({});

// Confirmación de borrado
const confirmState = reactive<{
  open: boolean;
  target: Habitacion | null;
  loading: boolean;
}>({
  open: false,
  target: null,
  loading: false,
});

const columns: TableColumn[] = [
  { key: 'numero', label: 'Número' },
  { key: 'tipo', label: 'Tipo' },
  { key: 'capacidad', label: 'Capacidad', align: 'center' },
  { key: 'tarifa', label: 'Tarifa / Noche', align: 'right' },
  { key: 'servicios', label: 'Servicios' },
  { key: 'estado', label: 'Estado Operativo' },
];

const estadoOptions: { value: EstadoHabitacion; label: string }[] = [
  { value: 'disponible', label: 'Disponible' },
  { value: 'ocupada', label: 'Ocupada' },
  { value: 'mantenimiento', label: 'Mantenimiento' },
  { value: 'fuera_de_servicio', label: 'Fuera de servicio' },
];

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

async function fetchHabitaciones(targetPage = filters.page): Promise<void> {
  filters.page = targetPage;
  loading.value = true;

  try {
    const capacidadMinVal = filters.capacidadMin.trim()
      ? Number(filters.capacidadMin)
      : undefined;
    const tarifaMaxVal = filters.tarifaMax.trim()
      ? Number(filters.tarifaMax)
      : undefined;

    const response = await habitacionesApi.list({
      tipo: filters.tipo || undefined,
      estado: filters.estado || undefined,
      capacidadMin:
        capacidadMinVal !== undefined && Number.isFinite(capacidadMinVal)
          ? capacidadMinVal
          : undefined,
      tarifaMax:
        tarifaMaxVal !== undefined && Number.isFinite(tarifaMaxVal)
          ? tarifaMaxVal
          : undefined,
      page: filters.page,
      limit: filters.limit,
    });

    rows.value = response.data;
    meta.value = response.meta;
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible obtener el catálogo de habitaciones.';
    notifications.error('Error al cargar habitaciones', message);
  } finally {
    loading.value = false;
  }
}

function resetFilters(): void {
  filters.tipo = '';
  filters.estado = '';
  filters.capacidadMin = '';
  filters.tarifaMax = '';
  void fetchHabitaciones(1);
}

function openCreateModal(): void {
  selectedHabitacion.value = null;
  serverFieldErrors.value = {};
  formModalOpen.value = true;
}

function openEditModal(habitacion: Habitacion): void {
  selectedHabitacion.value = habitacion;
  serverFieldErrors.value = {};
  formModalOpen.value = true;
}

async function handleCreate(payload: CreateHabitacionPayload): Promise<void> {
  submittingForm.value = true;
  serverFieldErrors.value = {};

  try {
    await habitacionesApi.create(payload);
    formModalOpen.value = false;
    notifications.success(
      'Habitación registrada',
      `La habitación ${payload.numero} se guardó en HotelDB.`
    );
    await fetchHabitaciones(1);
  } catch (err) {
    if (err instanceof ApiClientError) {
      serverFieldErrors.value = err.fieldErrorsMap;
      notifications.error(`No se pudo registrar (${err.status})`, err.message);
    } else {
      notifications.error('Error inesperado', 'No fue posible registrar la habitación.');
    }
  } finally {
    submittingForm.value = false;
  }
}

async function handleUpdate(id: string, payload: UpdateHabitacionPayload): Promise<void> {
  submittingForm.value = true;
  serverFieldErrors.value = {};

  try {
    await habitacionesApi.update(id, payload);
    formModalOpen.value = false;
    notifications.success('Habitación actualizada', 'Los cambios se guardaron correctamente.');
    await fetchHabitaciones(filters.page);
  } catch (err) {
    if (err instanceof ApiClientError) {
      serverFieldErrors.value = err.fieldErrorsMap;
      notifications.error(`No se pudo actualizar (${err.status})`, err.message);
    } else {
      notifications.error('Error inesperado', 'No fue posible actualizar la habitación.');
    }
  } finally {
    submittingForm.value = false;
  }
}

async function handleQuickEstadoChange(habitacion: Habitacion, event: Event): Promise<void> {
  const selectEl = event.target as HTMLSelectElement;
  const nuevoEstado = selectEl.value as EstadoHabitacion;
  if (nuevoEstado === habitacion.estado) return;

  updatingEstadoId.value = habitacion.id;
  try {
    const updated = await habitacionesApi.updateEstado(habitacion.id, nuevoEstado);
    habitacion.estado = updated.estado;
    habitacion.actualizadoEn = updated.actualizadoEn;
    notifications.success(
      'Estado actualizado',
      `Habitación ${habitacion.numero} ahora está '${updated.estado.replace('_', ' ')}'.`
    );
  } catch (err) {
    // Restaurar selección visual si el backend rechaza (ej. 409 por reservaciones activas)
    selectEl.value = habitacion.estado;
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible cambiar el estado de la habitación.';
    const status = err instanceof ApiClientError ? err.status : 500;
    notifications.error(`Cambio rechazado (${status})`, message);
  } finally {
    updatingEstadoId.value = null;
  }
}

function askDelete(habitacion: Habitacion): void {
  confirmState.target = habitacion;
  confirmState.open = true;
}

function closeConfirm(): void {
  if (!confirmState.loading) {
    confirmState.open = false;
    confirmState.target = null;
  }
}

async function executeDelete(): Promise<void> {
  if (!confirmState.target) return;
  const target = confirmState.target;

  confirmState.loading = true;
  try {
    await habitacionesApi.remove(target.id);
    confirmState.open = false;
    confirmState.target = null;
    notifications.success(
      'Habitación eliminada',
      `La habitación ${target.numero} fue eliminada de HotelDB.`
    );
    await fetchHabitaciones(filters.page);
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible eliminar la habitación.';
    const status = err instanceof ApiClientError ? err.status : 500;
    notifications.error(`Eliminación rechazada (${status})`, message);
    confirmState.open = false;
  } finally {
    confirmState.loading = false;
  }
}

onMounted(() => {
  void fetchHabitaciones(1);
});
</script>

<template>
  <div class="space-y-5">
    <!-- Encabezado -->
    <div class="flex flex-wrap items-center justify-between gap-3">
      <div>
        <h2 class="text-base font-bold text-slate-900">Catálogo de Habitaciones</h2>
        <p class="text-xs text-slate-500">
          Administra tarifas, capacidad, servicios y cambia el estado operativo en tiempo real.
        </p>
      </div>

      <button
        type="button"
        class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-700"
        @click="openCreateModal"
      >
        + Nueva habitación
      </button>
    </div>

    <!-- Filtros -->
    <section class="rounded-xl border border-slate-200 bg-white p-4 shadow-sm">
      <form
        class="grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-5"
        @submit.prevent="fetchHabitaciones(1)"
      >
        <div>
          <label class="block text-xs font-medium text-slate-600" for="fh-tipo">Tipo</label>
          <select
            id="fh-tipo"
            v-model="filters.tipo"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="">Todos</option>
            <option value="sencilla">Sencilla</option>
            <option value="doble">Doble</option>
            <option value="triple">Triple</option>
            <option value="suite">Suite</option>
            <option value="familiar">Familiar</option>
          </select>
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fh-estado">Estado</label>
          <select
            id="fh-estado"
            v-model="filters.estado"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="">Todos</option>
            <option v-for="e in estadoOptions" :key="e.value" :value="e.value">
              {{ e.label }}
            </option>
          </select>
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fh-cap">
            Capacidad mínima
          </label>
          <input
            id="fh-cap"
            v-model="filters.capacidadMin"
            type="number"
            min="1"
            max="20"
            placeholder="Ej. 2"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fh-tarifa">
            Tarifa máxima (MXN)
          </label>
          <input
            id="fh-tarifa"
            v-model="filters.tarifaMax"
            type="number"
            min="1"
            step="50"
            placeholder="Ej. 2500"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div class="flex items-end gap-2">
          <button
            type="submit"
            class="flex-1 rounded-lg bg-slate-900 px-3 py-1.5 text-sm font-medium text-white transition hover:bg-slate-800"
          >
            Filtrar
          </button>
          <button
            type="button"
            class="rounded-lg border border-slate-300 bg-white px-3 py-1.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
            @click="resetFilters"
          >
            Limpiar
          </button>
        </div>
      </form>
    </section>

    <!-- Tabla de habitaciones -->
    <DataTable
      :columns="columns"
      :rows="rows"
      :loading="loading"
      :meta="meta"
      has-actions
      empty-title="No se encontraron habitaciones"
      empty-description="Ajusta los filtros o registra una nueva habitación en el catálogo."
      @page-change="(p) => fetchHabitaciones(p)"
    >
      <template #cell-numero="{ row }">
        <span class="font-bold text-slate-900">#{{ row.numero }}</span>
      </template>

      <template #cell-tipo="{ row }">
        <span class="font-medium text-slate-700">{{ capitalize(row.tipo) }}</span>
      </template>

      <template #cell-capacidad="{ row }">
        <span>{{ row.capacidad }} {{ row.capacidad === 1 ? 'persona' : 'personas' }}</span>
      </template>

      <template #cell-tarifa="{ row }">
        <span class="font-semibold text-slate-900">{{ formatCurrency(row.tarifa) }}</span>
      </template>

      <template #cell-servicios="{ row }">
        <div v-if="row.servicios.length > 0" class="flex flex-wrap gap-1">
          <span
            v-for="srv in row.servicios"
            :key="srv"
            class="rounded-md bg-slate-100 px-2 py-0.5 text-xs text-slate-700"
          >
            {{ srv }}
          </span>
        </div>
        <span v-else class="text-xs text-slate-400">Sin servicios</span>
      </template>

      <template #cell-estado="{ row }">
        <div class="flex flex-wrap items-center gap-2">
          <StatusBadge :status="row.estado" />
          <select
            :value="row.estado"
            :disabled="updatingEstadoId === row.id"
            :aria-label="`Cambiar estado de habitación ${row.numero}`"
            class="rounded-md border border-slate-200 bg-white px-2 py-1 text-xs text-slate-700 focus:border-indigo-500 focus:outline-none disabled:opacity-50"
            @change="handleQuickEstadoChange(row, $event)"
          >
            <option v-for="opt in estadoOptions" :key="opt.value" :value="opt.value">
              {{ opt.label }}
            </option>
          </select>
        </div>
      </template>

      <template #actions="{ row }">
        <div class="inline-flex items-center justify-end gap-1.5">
          <button
            type="button"
            class="rounded-md border border-slate-200 px-2.5 py-1 text-xs font-medium text-slate-700 transition hover:bg-slate-100"
            @click="openEditModal(row)"
          >
            Editar
          </button>
          <button
            type="button"
            class="rounded-md border border-rose-200 bg-rose-50 px-2.5 py-1 text-xs font-medium text-rose-700 transition hover:bg-rose-100"
            @click="askDelete(row)"
          >
            Eliminar
          </button>
        </div>
      </template>
    </DataTable>

    <!-- Modal alta / edición -->
    <HabitacionFormModal
      :open="formModalOpen"
      :habitacion="selectedHabitacion"
      :submitting="submittingForm"
      :server-field-errors="serverFieldErrors"
      @close="formModalOpen = false"
      @save-create="handleCreate"
      @save-update="handleUpdate"
    />

    <!-- Confirmación de eliminación -->
    <ConfirmDialog
      :open="confirmState.open"
      title="Eliminar habitación"
      :message="`¿Deseas eliminar la habitación #${confirmState.target?.numero ?? ''}? Solo se permitirá si no tiene reservaciones registradas.`"
      confirm-text="Eliminar"
      variant="danger"
      :loading="confirmState.loading"
      @cancel="closeConfirm"
      @confirm="executeDelete"
    />
  </div>
</template>