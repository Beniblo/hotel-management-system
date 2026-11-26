<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue';
import DataTable, { type TableColumn } from '@/components/ui/DataTable.vue';
import StatusBadge from '@/components/ui/StatusBadge.vue';
import ConfirmDialog from '@/components/ui/ConfirmDialog.vue';
import HuespedFormModal from '@/components/forms/HuespedFormModal.vue';
import { huespedesApi } from '@/services/huespedesApi';
import { ApiClientError } from '@/services/apiClient';
import { useNotificationStore } from '@/stores/notifications';
import type { PaginationMeta } from '@/types/api';
import type {
  CreateHuespedPayload,
  Huesped,
  UpdateHuespedPayload,
} from '@/types/hotel';

const notifications = useNotificationStore();

const loading = ref(false);
const rows = ref<Huesped[]>([]);
const meta = ref<PaginationMeta | null>(null);

const filters = reactive({
  nombre: '',
  correo: '',
  activoFilter: 'all' as 'all' | 'true' | 'false',
  page: 1,
  limit: 10,
});

// Modal alta/edición
const formModalOpen = ref(false);
const selectedHuesped = ref<Huesped | null>(null);
const submittingForm = ref(false);
const serverFieldErrors = ref<Record<string, string>>({});

// Confirmación de baja lógica o eliminación física
const confirmState = reactive<{
  open: boolean;
  fisico: boolean;
  target: Huesped | null;
  loading: boolean;
}>({
  open: false,
  fisico: false,
  target: null,
  loading: false,
});

const columns: TableColumn[] = [
  { key: 'nombre', label: 'Nombre' },
  { key: 'correo', label: 'Correo' },
  { key: 'telefono', label: 'Teléfono' },
  { key: 'preferencias', label: 'Preferencias' },
  { key: 'activo', label: 'Estado', align: 'center' },
];

async function fetchHuespedes(targetPage = filters.page): Promise<void> {
  filters.page = targetPage;
  loading.value = true;

  try {
    const activoParam =
      filters.activoFilter === 'all'
        ? undefined
        : filters.activoFilter === 'true';

    const response = await huespedesApi.list({
      nombre: filters.nombre.trim() || undefined,
      correo: filters.correo.trim() || undefined,
      activo: activoParam,
      page: filters.page,
      limit: filters.limit,
    });
    rows.value = response.data;
    meta.value = response.meta;
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible obtener el listado de huéspedes.';
    notifications.error('Error al cargar huéspedes', message);
  } finally {
    loading.value = false;
  }
}

function resetFilters(): void {
  filters.nombre = '';
  filters.correo = '';
  filters.activoFilter = 'all';
  void fetchHuespedes(1);
}

function openCreateModal(): void {
  selectedHuesped.value = null;
  serverFieldErrors.value = {};
  formModalOpen.value = true;
}

function openEditModal(huesped: Huesped): void {
  selectedHuesped.value = huesped;
  serverFieldErrors.value = {};
  formModalOpen.value = true;
}

async function handleCreate(payload: CreateHuespedPayload): Promise<void> {
  submittingForm.value = true;
  serverFieldErrors.value = {};
  try {
    await huespedesApi.create(payload);
    formModalOpen.value = false;
    notifications.success('Huésped registrado', `${payload.nombre} fue guardado en HotelDB.`);
    await fetchHuespedes(1);
  } catch (err) {
    if (err instanceof ApiClientError) {
      serverFieldErrors.value = err.fieldErrorsMap;
      notifications.error(`No se pudo registrar (${err.status})`, err.message);
    } else {
      notifications.error('Error inesperado', 'No fue posible registrar el huésped.');
    }
  } finally {
    submittingForm.value = false;
  }
}

async function handleUpdate(id: string, payload: UpdateHuespedPayload): Promise<void> {
  submittingForm.value = true;
  serverFieldErrors.value = {};
  try {
    await huespedesApi.update(id, payload);
    formModalOpen.value = false;
    notifications.success('Huésped actualizado', 'Los cambios se guardaron correctamente.');
    await fetchHuespedes(filters.page);
  } catch (err) {
    if (err instanceof ApiClientError) {
      serverFieldErrors.value = err.fieldErrorsMap;
      notifications.error(`No se pudo actualizar (${err.status})`, err.message);
    } else {
      notifications.error('Error inesperado', 'No fue posible actualizar el huésped.');
    }
  } finally {
    submittingForm.value = false;
  }
}

function askDelete(huesped: Huesped, fisico: boolean): void {
  confirmState.target = huesped;
  confirmState.fisico = fisico;
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
  const fisico = confirmState.fisico;

  confirmState.loading = true;
  try {
    await huespedesApi.remove(target.id, fisico);
    confirmState.open = false;
    confirmState.target = null;

    if (fisico) {
      notifications.success(
        'Eliminación física ejecutada',
        `Se eliminó definitivamente a ${target.nombre}.`
      );
    } else {
      notifications.success(
        'Baja lógica aplicada',
        `${target.nombre} quedó marcado como inactivo.`
      );
    }
    await fetchHuespedes(filters.page);
  } catch (err) {
    const message =
      err instanceof ApiClientError
        ? err.message
        : 'No fue posible completar la baja del huésped.';
    const status = err instanceof ApiClientError ? err.status : 500;
    notifications.error(`Operación rechazada (${status})`, message);
    confirmState.open = false;
  } finally {
    confirmState.loading = false;
  }
}

onMounted(() => {
  void fetchHuespedes(1);
});
</script>

<template>
  <div class="space-y-5">
    <!-- Barra superior de acción -->
    <div class="flex flex-wrap items-center justify-between gap-3">
      <div>
        <h2 class="text-base font-bold text-slate-900">Catálogo de Huéspedes</h2>
        <p class="text-xs text-slate-500">
          La baja estándar desactiva al huésped; el borrado físico solo procede sin reservaciones vinculadas.
        </p>
      </div>

      <button
        type="button"
        class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-700"
        @click="openCreateModal"
      >
        + Nuevo huésped
      </button>
    </div>

    <!-- Panel de filtros -->
    <section class="rounded-xl border border-slate-200 bg-white p-4 shadow-sm">
      <form
        class="grid grid-cols-1 gap-3 sm:grid-cols-2 lg:grid-cols-4"
        @submit.prevent="fetchHuespedes(1)"
      >
        <div>
          <label class="block text-xs font-medium text-slate-600" for="f-nombre">Nombre</label>
          <input
            id="f-nombre"
            v-model="filters.nombre"
            type="text"
            placeholder="Buscar por nombre..."
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="f-correo">Correo</label>
          <input
            id="f-correo"
            v-model="filters.correo"
            type="text"
            placeholder="Buscar por correo..."
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          />
        </div>

        <div>
          <label class="block text-xs font-medium text-slate-600" for="f-activo">Estado</label>
          <select
            id="f-activo"
            v-model="filters.activoFilter"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-1.5 text-sm focus:border-indigo-500 focus:outline-none"
          >
            <option value="all">Todos</option>
            <option value="true">Activos</option>
            <option value="false">Inactivos</option>
          </select>
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

    <!-- Tabla de huéspedes -->
    <DataTable
      :columns="columns"
      :rows="rows"
      :loading="loading"
      :meta="meta"
      has-actions
      empty-title="No se encontraron huéspedes"
      empty-description="Ajusta los filtros o registra un nuevo huésped."
      @page-change="(p) => fetchHuespedes(p)"
    >
      <template #cell-nombre="{ row }">
        <span class="font-semibold text-slate-900">{{ row.nombre }}</span>
      </template>

      <template #cell-preferencias="{ row }">
        <div v-if="row.preferencias.length > 0" class="flex flex-wrap gap-1">
          <span
            v-for="pref in row.preferencias"
            :key="pref"
            class="rounded-md bg-slate-100 px-2 py-0.5 text-xs text-slate-700"
          >
            {{ pref }}
          </span>
        </div>
        <span v-else class="text-xs text-slate-400">Sin preferencias</span>
      </template>

      <template #cell-activo="{ row }">
        <StatusBadge :status="row.activo ? 'activo' : 'inactivo'" />
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
            v-if="row.activo"
            type="button"
            class="rounded-md border border-amber-200 bg-amber-50 px-2.5 py-1 text-xs font-medium text-amber-800 transition hover:bg-amber-100"
            @click="askDelete(row, false)"
          >
            Baja lógica
          </button>
          <button
            type="button"
            class="rounded-md border border-rose-200 bg-rose-50 px-2.5 py-1 text-xs font-medium text-rose-700 transition hover:bg-rose-100"
            @click="askDelete(row, true)"
          >
            Borrar físico
          </button>
        </div>
      </template>
    </DataTable>

    <!-- Modal alta / edición -->
    <HuespedFormModal
      :open="formModalOpen"
      :huesped="selectedHuesped"
      :submitting="submittingForm"
      :server-field-errors="serverFieldErrors"
      @close="formModalOpen = false"
      @save-create="handleCreate"
      @save-update="handleUpdate"
    />

    <!-- Diálogo de confirmación -->
    <ConfirmDialog
      :open="confirmState.open"
      :title="
        confirmState.fisico
          ? 'Eliminar huésped definitivamente'
          : 'Aplicar baja lógica al huésped'
      "
      :message="
        confirmState.fisico
          ? `¿Deseas eliminar físicamente a '${confirmState.target?.nombre ?? ''}' de la base de datos? Solo procederá si no tiene reservaciones asociadas.`
          : `¿Deseas desactivar a '${confirmState.target?.nombre ?? ''}'? Se conservará su historial.`
      "
      :confirm-text="confirmState.fisico ? 'Eliminar físicamente' : 'Desactivar'"
      :variant="confirmState.fisico ? 'danger' : 'warning'"
      :loading="confirmState.loading"
      @cancel="closeConfirm"
      @confirm="executeDelete"
    />
  </div>
</template>