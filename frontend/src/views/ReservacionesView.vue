<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import DataTable, { type TableColumn } from '@/components/ui/DataTable.vue';
import StatusBadge from '@/components/ui/StatusBadge.vue';
import ConfirmDialog from '@/components/ui/ConfirmDialog.vue';
import ReservacionFormModal, {
  type ReservacionPrefill,
} from '@/components/forms/ReservacionFormModal.vue';
import { reservacionesApi } from '@/services/reservacionesApi';
import { huespedesApi } from '@/services/huespedesApi';
import { habitacionesApi } from '@/services/habitacionesApi';
import { ApiClientError } from '@/services/apiClient';
import { useNotificationStore } from '@/stores/notifications';
import type { PaginationMeta } from '@/types/api';
import type {
  CreateReservacionPayload,
  EstadoReservacion,
  Habitacion,
  Huesped,
  Reservacion,
  UpdateReservacionPayload,
} from '@/types/hotel';

const route = useRoute();
const router = useRouter();
const notifications = useNotificationStore();

const loading = ref(false);
const rows = ref<Reservacion[]>([]);
const meta = ref<PaginationMeta | null>(null);

const huespedesCatalog = ref<Huesped[]>([]);
const habitacionesCatalog = ref<Habitacion[]>([]);

const filters = reactive({
  huespedId: '',
  habitacionId: '',
  estado: '' as EstadoReservacion | '',
  fechaDesde: '',
  fechaHasta: '',
  page: 1,
  limit: 10,
});

// Modal alta / edición
const formModalOpen = ref(false);
const selectedReservacion = ref<Reservacion | null>(null);
const prefillData = ref<ReservacionPrefill | null>(null);
const submittingForm = ref(false);
const serverFieldErrors = ref<Record<string, string>>({});

// Diálogo de confirmación (cancelar o eliminar)
const confirmState = reactive<{
  open: boolean;
  mode: 'cancel' | 'delete';
  target: Reservacion | null;
  loading: boolean;
}>({
  open: false,
  mode: 'cancel',
  target: null,
  loading: false,
});

const columns: TableColumn[] = [
  { key: 'huespedId', label: 'Huésped' },
  { key: 'habitacionId', label: 'Habitación' },
  { key: 'fechas', label: 'Estancia' },
  { key: 'personas', label: 'Personas', align: 'center' },
  { key: 'total', label: 'Total (MXN)', align: 'right' },
  { key: 'estado', label: 'Estado', align: 'center' },
];

const huespedMap = computed(() => {
  const map = new Map<string, Huesped>();
  for (const h of huespedesCatalog.value) {
    map.set(h.id, h);
  }
  return map;
});

const habitacionMap = computed(() => {
  const map = new Map<string, Habitacion>();
  for (const hab of habitacionesCatalog.value) {
    map.set(hab.id, hab);
  }
  return map;
});

function formatCurrency(value: number): string {
  return new Intl.NumberFormat('es-MX', {
    style: 'currency',
    currency: 'MXN',
    minimumFractionDigits: 2,
  }).format(value);
}

function getHuespedLabel(id: string): string {
  const h = huespedMap.value.get(id);
  return h ? h.nombre : id.slice(-6);
}

function getHuespedSub(id: string): string {
  return huespedMap.value.get(id)?.correo ?? '';
}

function getHabitacionLabel(id: string): string {
  const hab = habitacionMap.value.get(id);
  return hab ? `#${hab.numero} (${hab.tipo})` : id.slice(-6);
}

function canEditOrCancel(r: Reservacion): boolean {
  return r.estado !== 'completada' && r.estado !== 'cancelada';
}

async function loadCatalogs(): Promise<void> {
  try {
    const [hRes, habRes] = await Promise.all([
      huespedesApi.list({ page: 1, limit: 100 }),
      habitacionesApi.list({ page: 1, limit: 100 }),
    ]);
    huespedesCatalog.value = hRes.data;
    habitacionesCatalog.value = habRes.data;
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No se pudieron cargar los catálogos de huéspedes y habitaciones.';
    notifications.warning('Catálogos incompletos', message);
  }
}

async function fetchReservaciones(targetPage = filters.page): Promise<void> {
  if (
    filters.fechaDesde &&
    filters.fechaHasta &&
    filters.fechaHasta < filters.fechaDesde
  ) {
    notifications.warning(
      'Rango inválido',
      'La fecha "Hasta" debe ser mayor o igual a la fecha "Desde".'
    );
    return;
  }

  filters.page = targetPage;
  loading.value = true;

  try {
    const response = await reservacionesApi.list({
      huespedId: filters.huespedId || undefined,
      habitacionId: filters.habitacionId || undefined,
      estado: filters.estado || undefined,
      fechaDesde: filters.fechaDesde || undefined,
      fechaHasta: filters.fechaHasta || undefined,
      page: filters.page,
      limit: filters.limit,
    });
    rows.value = response.data;
    meta.value = response.meta;
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible consultar las reservaciones.';
    notifications.error('Error al cargar reservaciones', message);
  } finally {
    loading.value = false;
  }
}

function resetFilters(): void {
  filters.huespedId = '';
  filters.habitacionId = '';
  filters.estado = '';
  filters.fechaDesde = '';
  filters.fechaHasta = '';
  void fetchReservaciones(1);
}

function openCreateModal(prefill: ReservacionPrefill | null = null): void {
  selectedReservacion.value = null;
  prefillData.value = prefill;
  serverFieldErrors.value = {};
  formModalOpen.value = true;
}

function openEditModal(reservacion: Reservacion): void {
  selectedReservacion.value = reservacion;
  prefillData.value = null;
  serverFieldErrors.value = {};
  formModalOpen.value = true;
}

function closeFormModal(): void {
  formModalOpen.value = false;
  prefillData.value = null;
  if (route.query.action === 'new') {
    void router.replace({ name: 'reservaciones' });
  }
}

async function handleCreate(payload: CreateReservacionPayload): Promise<void> {
  submittingForm.value = true;
  serverFieldErrors.value = {};
  try {
    const created = await reservacionesApi.create(payload);
    closeFormModal();
    notifications.success(
      'Reservación creada',
      `Estancia confirmada por ${created.noches} noche(s) — Total: ${formatCurrency(created.total)}.`
    );
    await fetchReservaciones(1);
  } catch (err) {
    if (err instanceof ApiClientError) {
      serverFieldErrors.value = err.fieldErrorsMap;
      notifications.error(`No se pudo reservar (${err.status})`, err.message);
    } else {
      notifications.error('Error inesperado', 'No fue posible crear la reservación.');
    }
  } finally {
    submittingForm.value = false;
  }
}

async function handleUpdate(
  id: string,
  payload: UpdateReservacionPayload
): Promise<void> {
  submittingForm.value = true;
  serverFieldErrors.value = {};
  try {
    const updated = await reservacionesApi.update(id, payload);
    closeFormModal();
    notifications.success(
      'Reservación actualizada',
      `Nuevo total: ${formatCurrency(updated.total)} (${updated.noches} noche(s)).`
    );
    await fetchReservaciones(filters.page);
  } catch (err) {
    if (err instanceof ApiClientError) {
      serverFieldErrors.value = err.fieldErrorsMap;
      notifications.error(`No se pudo actualizar (${err.status})`, err.message);
    } else {
      notifications.error(
        'Error inesperado',
        'No fue posible actualizar la reservación.'
      );
    }
  } finally {
    submittingForm.value = false;
  }
}

function askAction(reservacion: Reservacion, mode: 'cancel' | 'delete'): void {
  confirmState.target = reservacion;
  confirmState.mode = mode;
  confirmState.open = true;
}

function closeConfirm(): void {
  if (!confirmState.loading) {
    confirmState.open = false;
    confirmState.target = null;
  }
}

async function executeConfirmedAction(): Promise<void> {
  if (!confirmState.target) return;
  const target = confirmState.target;
  const mode = confirmState.mode;

  confirmState.loading = true;
  try {
    if (mode === 'cancel') {
      await reservacionesApi.cancel(target.id);
      notifications.success(
        'Reservación cancelada',
        'La habitación quedó liberada para esas fechas.'
      );
    } else {
      await reservacionesApi.remove(target.id);
      notifications.success(
        'Reservación eliminada',
        'El registro fue eliminado definitivamente de HotelDB.'
      );
    }
    confirmState.open = false;
    confirmState.target = null;
    await fetchReservaciones(filters.page);
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible completar la acción solicitada.';
    const status = err instanceof ApiClientError ? err.status : 500;
    notifications.error(`Acción rechazada (${status})`, message);
    confirmState.open = false;
  } finally {
    confirmState.loading = false;
  }
}

onMounted(async () => {
  await Promise.all([loadCatalogs(), fetchReservaciones(1)]);

  // Abrir modal prellenado si venimos desde DisponibilidadView
  if (route.query.action === 'new') {
    const personasRaw = Number(route.query.personas);
    openCreateModal({
      habitacionId:
        typeof route.query.habitacionId === 'string'
          ? route.query.habitacionId
          : undefined,
      fechaEntrada:
        typeof route.query.fechaEntrada === 'string'
          ? route.query.fechaEntrada
          : undefined,
      fechaSalida:
        typeof route.query.fechaSalida === 'string'
          ? route.query.fechaSalida
          : undefined,
      personas: Number.isInteger(personasRaw) && personasRaw >= 1 ? personasRaw : 1,
    });
  }
});
</script>

<template>
  <div class="space-y-5">
    <!-- Encabezado -->
    <div class="flex flex-wrap items-center justify-between gap-3">
      <div>
        <h2 class="text-base font-bold text-slate-900">Gestión de Reservaciones</h2>
        <p class="text-xs text-slate-500">
          El backend valida traslapes bajo exclusión mutua por habitación y congela la tarifa vigente.
        </p>
      </div>

      <button
        type="button"
        class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-700"
        @click="openCreateModal(null)"
      >
        + Nueva reservación
      </button>
    </div>

    <!-- Filtros -->
    <section class="rounded-xl border border-slate-200 bg-white p-4 shadow-sm">
      <form
        class="grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-6"
        @submit.prevent="fetchReservaciones(1)"
      >
        <div>
          <label class="block text-xs font-medium text-slate-600" for="fr-huesped">
            Huésped
          </label>
          <select
            id="fr-huesped"
            v-model="filters.huespedId"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-2.5 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="">Todos</option>
            <option v-for="h in huespedesCatalog" :key="h.id" :value="h.id">
              {{ h.nombre }}
            </option>
          </select>
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fr-hab">
            Habitación
          </label>
          <select
            id="fr-hab"
            v-model="filters.habitacionId"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-2.5 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="">Todas</option>
            <option v-for="hab in habitacionesCatalog" :key="hab.id" :value="hab.id">
              #{{ hab.numero }} ({{ hab.tipo }})
            </option>
          </select>
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fr-estado">
            Estado
          </label>
          <select
            id="fr-estado"
            v-model="filters.estado"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-2.5 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="">Todos</option>
            <option value="pendiente">Pendiente</option>
            <option value="confirmada">Confirmada</option>
            <option value="en_curso">En curso</option>
            <option value="completada">Completada</option>
            <option value="cancelada">Cancelada</option>
          </select>
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fr-desde">
            Desde
          </label>
          <input
            id="fr-desde"
            v-model="filters.fechaDesde"
            type="date"
            class="mt-1 w-full rounded-lg border border-slate-300 px-2.5 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="fr-hasta">
            Hasta
          </label>
          <input
            id="fr-hasta"
            v-model="filters.fechaHasta"
            type="date"
            class="mt-1 w-full rounded-lg border border-slate-300 px-2.5 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
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

    <!-- Tabla de reservaciones -->
    <DataTable
      :columns="columns"
      :rows="rows"
      :loading="loading"
      :meta="meta"
      has-actions
      empty-title="No se encontraron reservaciones"
      empty-description="Ajusta los filtros o crea una nueva reservación."
      @page-change="(p) => fetchReservaciones(p)"
    >
      <template #cell-huespedId="{ row }">
        <div>
          <p class="font-semibold text-slate-900">{{ getHuespedLabel(row.huespedId) }}</p>
          <p v-if="getHuespedSub(row.huespedId)" class="text-xs text-slate-500">
            {{ getHuespedSub(row.huespedId) }}
          </p>
        </div>
      </template>

      <template #cell-habitacionId="{ row }">
        <span class="font-medium text-slate-800">{{ getHabitacionLabel(row.habitacionId) }}</span>
      </template>

      <template #cell-fechas="{ row }">
        <div>
          <p class="font-medium text-slate-800">
            {{ row.fechaEntrada }} → {{ row.fechaSalida }}
          </p>
          <p class="text-xs text-slate-500">
            {{ row.noches }} {{ row.noches === 1 ? 'noche' : 'noches' }} ·
            {{ formatCurrency(row.tarifaAplicada) }}/noche
          </p>
        </div>
      </template>

      <template #cell-total="{ row }">
        <span class="font-bold text-slate-900">{{ formatCurrency(row.total) }}</span>
      </template>

      <template #cell-estado="{ row }">
        <StatusBadge :status="row.estado" />
      </template>

      <template #actions="{ row }">
        <div class="inline-flex items-center justify-end gap-1.5">
          <button
            v-if="canEditOrCancel(row)"
            type="button"
            class="rounded-md border border-slate-200 px-2.5 py-1 text-xs font-medium text-slate-700 transition hover:bg-slate-100"
            @click="openEditModal(row)"
          >
            Editar
          </button>
          <button
            v-if="canEditOrCancel(row)"
            type="button"
            class="rounded-md border border-amber-200 bg-amber-50 px-2.5 py-1 text-xs font-medium text-amber-800 transition hover:bg-amber-100"
            @click="askAction(row, 'cancel')"
          >
            Cancelar
          </button>
          <button
            type="button"
            class="rounded-md border border-rose-200 bg-rose-50 px-2.5 py-1 text-xs font-medium text-rose-700 transition hover:bg-rose-100"
            @click="askAction(row, 'delete')"
          >
            Eliminar
          </button>
        </div>
      </template>
    </DataTable>

    <!-- Modal alta / edición -->
    <ReservacionFormModal
      :open="formModalOpen"
      :reservacion="selectedReservacion"
      :prefill="prefillData"
      :huespedes="huespedesCatalog"
      :habitaciones="habitacionesCatalog"
      :submitting="submittingForm"
      :server-field-errors="serverFieldErrors"
      @close="closeFormModal"
      @save-create="handleCreate"
      @save-update="handleUpdate"
    />

    <!-- Confirmación de cancelación o borrado -->
    <ConfirmDialog
      :open="confirmState.open"
      :title="
        confirmState.mode === 'cancel'
          ? 'Cancelar reservación'
          : 'Eliminar reservación'
      "
      :message="
        confirmState.mode === 'cancel'
          ? `¿Deseas cancelar la reservación de ${confirmState.target ? getHuespedLabel(confirmState.target.huespedId) : ''} (${confirmState.target?.fechaEntrada ?? ''} a ${confirmState.target?.fechaSalida ?? ''})?`
          : '¿Deseas eliminar físicamente esta reservación? El servidor solo lo permite si no está confirmada ni en curso.'
      "
      :confirm-text="
        confirmState.mode === 'cancel' ? 'Sí, cancelar' : 'Eliminar'
      "
      :variant="confirmState.mode === 'cancel' ? 'warning' : 'danger'"
      :loading="confirmState.loading"
      @cancel="closeConfirm"
      @confirm="executeConfirmedAction"
    />
  </div>
</template>