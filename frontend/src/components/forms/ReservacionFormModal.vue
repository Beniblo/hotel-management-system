<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue';
import BaseModal from '@/components/ui/BaseModal.vue';
import { disponibilidadApi } from '@/services/disponibilidadApi';
import type {
  CreateReservacionPayload,
  EstadoReservacion,
  Habitacion,
  HabitacionDisponible,
  Huesped,
  Reservacion,
  UpdateReservacionPayload,
} from '@/types/hotel';

export interface ReservacionPrefill {
  habitacionId?: string;
  fechaEntrada?: string;
  fechaSalida?: string;
  personas?: number;
}

const props = defineProps<{
  open: boolean;
  reservacion?: Reservacion | null;
  prefill?: ReservacionPrefill | null;
  huespedes: Huesped[];
  habitaciones: Habitacion[];
  submitting?: boolean;
  serverFieldErrors?: Record<string, string>;
}>();

const emit = defineEmits<{
  close: [];
  saveCreate: [payload: CreateReservacionPayload];
  saveUpdate: [id: string, payload: UpdateReservacionPayload];
}>();

const isEditing = computed(() => Boolean(props.reservacion));

function getTodayIso(): string {
  const now = new Date();
  const y = now.getFullYear();
  const m = String(now.getMonth() + 1).padStart(2, '0');
  const d = String(now.getDate()).padStart(2, '0');
  return `${y}-${m}-${d}`;
}

function addDaysIso(baseIso: string, days: number): string {
  const parts = baseIso.split('-').map(Number);
  if (parts.length !== 3) return baseIso;
  const dt = new Date(Date.UTC(parts[0], parts[1] - 1, parts[2] + days));
  const ry = dt.getUTCFullYear();
  const rm = String(dt.getUTCMonth() + 1).padStart(2, '0');
  const rd = String(dt.getUTCDate()).padStart(2, '0');
  return `${ry}-${rm}-${rd}`;
}

function calculateNightsLocal(entrada: string, salida: string): number {
  if (!entrada || !salida || salida <= entrada) return 0;
  const p1 = entrada.split('-').map(Number);
  const p2 = salida.split('-').map(Number);
  if (p1.length !== 3 || p2.length !== 3) return 0;
  const t1 = Date.UTC(p1[0], p1[1] - 1, p1[2]);
  const t2 = Date.UTC(p2[0], p2[1] - 1, p2[2]);
  const diff = Math.round((t2 - t1) / (24 * 60 * 60 * 1000));
  return diff > 0 ? diff : 0;
}

function formatCurrency(value: number): string {
  return new Intl.NumberFormat('es-MX', {
    style: 'currency',
    currency: 'MXN',
    minimumFractionDigits: 2,
  }).format(value);
}

const form = reactive<{
  huespedId: string;
  habitacionId: string;
  fechaEntrada: string;
  fechaSalida: string;
  personas: string;
  estado: EstadoReservacion;
}>({
  huespedId: '',
  habitacionId: '',
  fechaEntrada: '',
  fechaSalida: '',
  personas: '1',
  estado: 'confirmada',
});

const localErrors = reactive<Record<string, string>>({});
const loadingAvailability = ref(false);
const availableRooms = ref<HabitacionDisponible[]>([]);
const availabilityChecked = ref(false);

const activeHuespedes = computed(() =>
  props.huespedes.filter((h) => h.activo)
);

const selectableRooms = computed<Habitacion[]>(() => {
  if (!isEditing.value && availabilityChecked.value) {
    return availableRooms.value;
  }
  return props.habitaciones.filter(
    (h) =>
      h.estado === 'disponible' ||
      (props.reservacion && h.id === props.reservacion.habitacionId)
  );
});

const selectedRoom = computed<Habitacion | undefined>(() => {
  return (
    availableRooms.value.find((r) => r.id === form.habitacionId) ??
    props.habitaciones.find((r) => r.id === form.habitacionId)
  );
});

const nochesCalculadas = computed(() =>
  calculateNightsLocal(form.fechaEntrada, form.fechaSalida)
);

const tarifaEstimada = computed(() => {
  if (
    props.reservacion &&
    form.habitacionId === props.reservacion.habitacionId
  ) {
    return props.reservacion.tarifaAplicada;
  }
  return selectedRoom.value?.tarifa ?? 0;
});

const totalEstimado = computed(() => {
  if (nochesCalculadas.value <= 0 || tarifaEstimada.value <= 0) return 0;
  return Math.round(nochesCalculadas.value * tarifaEstimada.value * 100) / 100;
});

const editEstadoOptions = computed<
  { value: EstadoReservacion; label: string }[]
>(() => {
  if (!props.reservacion) {
    return [
      { value: 'confirmada', label: 'Confirmada' },
      { value: 'pendiente', label: 'Pendiente' },
    ];
  }
  const current = props.reservacion.estado;
  if (current === 'pendiente') {
    return [
      { value: 'pendiente', label: 'Pendiente' },
      { value: 'confirmada', label: 'Confirmada' },
      { value: 'en_curso', label: 'En curso' },
      { value: 'cancelada', label: 'Cancelada' },
    ];
  }
  if (current === 'confirmada') {
    return [
      { value: 'confirmada', label: 'Confirmada' },
      { value: 'en_curso', label: 'En curso' },
      { value: 'completada', label: 'Completada' },
      { value: 'cancelada', label: 'Cancelada' },
    ];
  }
  if (current === 'en_curso') {
    return [
      { value: 'en_curso', label: 'En curso' },
      { value: 'completada', label: 'Completada' },
      { value: 'cancelada', label: 'Cancelada' },
    ];
  }
  return [{ value: current, label: current }];
});

function clearLocalErrors(): void {
  localErrors.huespedId = '';
  localErrors.habitacionId = '';
  localErrors.fechaEntrada = '';
  localErrors.fechaSalida = '';
  localErrors.personas = '';
  localErrors.estado = '';
}

async function refreshLiveAvailability(): Promise<void> {
  if (isEditing.value || !props.open) return;

  const nights = calculateNightsLocal(form.fechaEntrada, form.fechaSalida);
  if (nights <= 0) {
    availableRooms.value = [];
    availabilityChecked.value = false;
    return;
  }

  loadingAvailability.value = true;
  try {
    const res = await disponibilidadApi.consultar({
      fechaEntrada: form.fechaEntrada,
      fechaSalida: form.fechaSalida,
    });
    availableRooms.value = res.habitaciones;
    availabilityChecked.value = true;

    // Mantener la selección solo si sigue disponible en las nuevas fechas
    if (
      form.habitacionId &&
      !res.habitaciones.some((h) => h.id === form.habitacionId)
    ) {
      form.habitacionId = '';
    }
  } catch {
    // Si falla la consulta auxiliar, el backend seguirá validando al guardar
    availabilityChecked.value = false;
  } finally {
    loadingAvailability.value = false;
  }
}

function resetForm(): void {
  clearLocalErrors();
  availableRooms.value = [];
  availabilityChecked.value = false;

  if (props.reservacion) {
    form.huespedId = props.reservacion.huespedId;
    form.habitacionId = props.reservacion.habitacionId;
    form.fechaEntrada = props.reservacion.fechaEntrada;
    form.fechaSalida = props.reservacion.fechaSalida;
    form.personas = String(props.reservacion.personas);
    form.estado = props.reservacion.estado;
  } else {
    const today = getTodayIso();
    form.huespedId = activeHuespedes.value[0]?.id ?? '';
    form.habitacionId = props.prefill?.habitacionId ?? '';
    form.fechaEntrada = props.prefill?.fechaEntrada || today;
    form.fechaSalida = props.prefill?.fechaSalida || addDaysIso(today, 2);
    form.personas = String(props.prefill?.personas ?? 1);
    form.estado = 'confirmada';
    void refreshLiveAvailability();
  }
}

watch(
  () => [props.open, props.reservacion, props.prefill],
  ([isOpen]) => {
    if (isOpen) {
      resetForm();
    }
  },
  { immediate: true }
);

watch(
  () => [form.fechaEntrada, form.fechaSalida],
  () => {
    if (props.open && !isEditing.value) {
      void refreshLiveAvailability();
    }
  }
);

function errorFor(field: string): string {
  return props.serverFieldErrors?.[field] || localErrors[field] || '';
}

function validateClient(): boolean {
  clearLocalErrors();
  let valid = true;

  if (!isEditing.value && !form.huespedId) {
    localErrors.huespedId = 'Selecciona un huésped activo.';
    valid = false;
  }

  if (!form.habitacionId) {
    localErrors.habitacionId = 'Selecciona una habitación.';
    valid = false;
  }

  if (!form.fechaEntrada) {
    localErrors.fechaEntrada = 'Ingresa la fecha de entrada.';
    valid = false;
  }

  if (!form.fechaSalida) {
    localErrors.fechaSalida = 'Ingresa la fecha de salida.';
    valid = false;
  } else if (form.fechaEntrada && form.fechaSalida <= form.fechaEntrada) {
    localErrors.fechaSalida =
      'La fecha de salida debe ser estrictamente posterior a la fecha de entrada.';
    valid = false;
  }

  const personas = Number(form.personas);
  if (!Number.isInteger(personas) || personas < 1 || personas > 20) {
    localErrors.personas = 'Ingresa un número de personas entre 1 y 20.';
    valid = false;
  } else if (selectedRoom.value && personas > selectedRoom.value.capacidad) {
    localErrors.personas = `La habitación #${selectedRoom.value.numero} admite máximo ${selectedRoom.value.capacidad} personas.`;
    valid = false;
  }

  return valid;
}

function handleSubmit(): void {
  if (!validateClient()) return;

  const personas = Number(form.personas);

  if (props.reservacion) {
    const payload: UpdateReservacionPayload = {
      habitacionId: form.habitacionId,
      fechaEntrada: form.fechaEntrada,
      fechaSalida: form.fechaSalida,
      personas,
      estado: form.estado,
    };
    emit('saveUpdate', props.reservacion.id, payload);
  } else {
    const estadoInicial: 'pendiente' | 'confirmada' =
      form.estado === 'pendiente' ? 'pendiente' : 'confirmada';

    const payload: CreateReservacionPayload = {
      huespedId: form.huespedId,
      habitacionId: form.habitacionId,
      fechaEntrada: form.fechaEntrada,
      fechaSalida: form.fechaSalida,
      personas,
      estado: estadoInicial,
    };
    emit('saveCreate', payload);
  }
}
</script>

<template>
  <BaseModal
    :open="open"
    :title="isEditing ? 'Editar Reservación' : 'Nueva Reservación'"
    size="lg"
    @close="emit('close')"
  >
    <form id="reservacion-form" class="space-y-4" @submit.prevent="handleSubmit">
      <!-- Huésped -->
      <div>
        <label class="block text-xs font-semibold text-slate-700" for="res-huesped">
          Huésped *
        </label>
        <select
          id="res-huesped"
          v-model="form.huespedId"
          :disabled="isEditing"
          class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none disabled:bg-slate-100 disabled:text-slate-500"
        >
          <option value="" disabled>Selecciona un huésped activo...</option>
          <option
            v-for="h in isEditing ? huespedes : activeHuespedes"
            :key="h.id"
            :value="h.id"
          >
            {{ h.nombre }} ({{ h.correo }})
          </option>
        </select>
        <p v-if="errorFor('huespedId')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('huespedId') }}
        </p>
      </div>

      <!-- Fechas -->
      <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
        <div>
          <label class="block text-xs font-semibold text-slate-700" for="res-entrada">
            Fecha de entrada *
          </label>
          <input
            id="res-entrada"
            v-model="form.fechaEntrada"
            type="date"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          />
          <p v-if="errorFor('fechaEntrada')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('fechaEntrada') }}
          </p>
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="res-salida">
            Fecha de salida *
          </label>
          <input
            id="res-salida"
            v-model="form.fechaSalida"
            type="date"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          />
          <p v-if="errorFor('fechaSalida')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('fechaSalida') }}
          </p>
        </div>
      </div>

      <!-- Habitación (con disponibilidad en vivo al crear) -->
      <div>
        <div class="flex items-center justify-between">
          <label class="block text-xs font-semibold text-slate-700" for="res-habitacion">
            Habitación *
          </label>
          <span v-if="!isEditing && loadingAvailability" class="text-[11px] text-indigo-600">
            Consultando disponibilidad en tiempo real...
          </span>
          <span
            v-else-if="!isEditing && availabilityChecked"
            class="text-[11px] text-emerald-700"
          >
            {{ availableRooms.length }} disponible(s) para estas fechas
          </span>
        </div>

        <select
          id="res-habitacion"
          v-model="form.habitacionId"
          class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        >
          <option value="" disabled>
            {{
              selectableRooms.length === 0
                ? 'No hay habitaciones libres en estas fechas'
                : 'Selecciona una habitación...'
            }}
          </option>
          <option v-for="hab in selectableRooms" :key="hab.id" :value="hab.id">
            #{{ hab.numero }} · {{ hab.tipo }} (Cap. {{ hab.capacidad }}) —
            {{ formatCurrency(hab.tarifa) }}/noche
          </option>
        </select>
        <p v-if="errorFor('habitacionId')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('habitacionId') }}
        </p>
      </div>

      <!-- Personas y Estado -->
      <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
        <div>
          <label class="block text-xs font-semibold text-slate-700" for="res-personas">
            Número de personas *
          </label>
          <input
            id="res-personas"
            v-model="form.personas"
            type="number"
            min="1"
            :max="selectedRoom?.capacidad ?? 20"
            step="1"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          />
          <p v-if="selectedRoom" class="mt-1 text-[11px] text-slate-500">
            Capacidad máxima de la habitación #{{ selectedRoom.numero }}: {{ selectedRoom.capacidad }}
          </p>
          <p v-if="errorFor('personas')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('personas') }}
          </p>
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="res-estado">
            Estado de la reservación
          </label>
          <select
            id="res-estado"
            v-model="form.estado"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          >
            <option
              v-for="opt in editEstadoOptions"
              :key="opt.value"
              :value="opt.value"
            >
              {{ opt.label }}
            </option>
          </select>
          <p v-if="errorFor('estado')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('estado') }}
          </p>
        </div>
      </div>

      <!-- Previsualización en vivo del cálculo -->
      <div class="rounded-xl border border-indigo-100 bg-indigo-50/70 p-4">
        <div class="flex flex-wrap items-center justify-between gap-2 text-sm">
          <div class="space-y-0.5 text-slate-700">
            <p class="text-xs font-semibold uppercase tracking-wide text-indigo-700">
              Cálculo estimado en vivo
            </p>
            <p class="text-xs text-slate-600">
              {{ nochesCalculadas }} {{ nochesCalculadas === 1 ? 'noche' : 'noches' }} ×
              {{ formatCurrency(tarifaEstimada) }}
            </p>
          </div>
          <p class="text-xl font-bold text-indigo-800">
            {{ formatCurrency(totalEstimado) }}
          </p>
        </div>
        <p class="mt-1.5 text-[11px] text-slate-500">
          El total definitivo y el bloqueo anti-traslapes se validan y congelan en el servidor C++.
        </p>
      </div>
    </form>

    <template #footer>
      <button
        type="button"
        class="rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 transition hover:bg-slate-50 disabled:opacity-50"
        :disabled="submitting"
        @click="emit('close')"
      >
        Cancelar
      </button>
      <button
        type="submit"
        form="reservacion-form"
        class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white transition hover:bg-indigo-700 disabled:opacity-50"
        :disabled="submitting"
      >
        {{
          submitting
            ? 'Guardando...'
            : isEditing
              ? 'Guardar cambios'
              : 'Crear reservación'
        }}
      </button>
    </template>
  </BaseModal>
</template>