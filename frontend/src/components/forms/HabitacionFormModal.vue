<script setup lang="ts">
import { computed, reactive, watch } from 'vue';
import BaseModal from '@/components/ui/BaseModal.vue';
import type {
  CreateHabitacionPayload,
  EstadoHabitacion,
  Habitacion,
  TipoHabitacion,
  UpdateHabitacionPayload,
} from '@/types/hotel';

const props = defineProps<{
  open: boolean;
  habitacion?: Habitacion | null;
  submitting?: boolean;
  serverFieldErrors?: Record<string, string>;
}>();

const emit = defineEmits<{
  close: [];
  saveCreate: [payload: CreateHabitacionPayload];
  saveUpdate: [id: string, payload: UpdateHabitacionPayload];
}>();

const isEditing = computed(() => Boolean(props.habitacion));

const tiposPermitidos: { value: TipoHabitacion; label: string }[] = [
  { value: 'sencilla', label: 'Sencilla' },
  { value: 'doble', label: 'Doble' },
  { value: 'triple', label: 'Triple' },
  { value: 'suite', label: 'Suite' },
  { value: 'familiar', label: 'Familiar' },
];

const estadosPermitidos: { value: EstadoHabitacion; label: string }[] = [
  { value: 'disponible', label: 'Disponible' },
  { value: 'ocupada', label: 'Ocupada' },
  { value: 'mantenimiento', label: 'Mantenimiento' },
  { value: 'fuera_de_servicio', label: 'Fuera de servicio' },
];

const form = reactive<{
  numero: string;
  tipo: TipoHabitacion;
  capacidad: string;
  tarifa: string;
  estado: EstadoHabitacion;
  serviciosText: string;
}>({
  numero: '',
  tipo: 'sencilla',
  capacidad: '1',
  tarifa: '',
  estado: 'disponible',
  serviciosText: '',
});

const localErrors = reactive<Record<string, string>>({});

function toValidTipo(raw: string): TipoHabitacion {
  const lower = raw.toLowerCase();
  const match = tiposPermitidos.find((t) => t.value === lower);
  return match ? match.value : 'sencilla';
}

function clearLocalErrors(): void {
  localErrors.numero = '';
  localErrors.tipo = '';
  localErrors.capacidad = '';
  localErrors.tarifa = '';
  localErrors.estado = '';
  localErrors.servicios = '';
}

function resetForm(): void {
  clearLocalErrors();

  if (props.habitacion) {
    form.numero = props.habitacion.numero;
    form.tipo = toValidTipo(props.habitacion.tipo);
    form.capacidad = String(props.habitacion.capacidad);
    form.tarifa = String(props.habitacion.tarifa);
    form.estado = props.habitacion.estado;
    form.serviciosText = props.habitacion.servicios.join(', ');
  } else {
    form.numero = '';
    form.tipo = 'sencilla';
    form.capacidad = '2';
    form.tarifa = '';
    form.estado = 'disponible';
    form.serviciosText = '';
  }
}

watch(
  () => [props.open, props.habitacion],
  ([isOpen]) => {
    if (isOpen) {
      resetForm();
    }
  },
  { immediate: true }
);

function errorFor(field: string): string {
  return props.serverFieldErrors?.[field] || localErrors[field] || '';
}

function parseServices(raw: string): string[] {
  return raw
    .split(',')
    .map((s) => s.trim())
    .filter((s) => s.length > 0);
}

function validateClient(): boolean {
  clearLocalErrors();
  let valid = true;

  const numero = form.numero.trim();
  if (!numero || numero.length > 20 || !/^[a-zA-Z0-9-]+$/.test(numero)) {
    localErrors.numero =
      'Usa solo letras, números o guion (máximo 20 caracteres, ej. 101 o 204-A).';
    valid = false;
  }

  const capacidad = Number(form.capacidad);
  if (!Number.isInteger(capacidad) || capacidad < 1 || capacidad > 20) {
    localErrors.capacidad = 'La capacidad debe ser un entero entre 1 y 20.';
    valid = false;
  }

  const tarifa = Number(form.tarifa);
  if (!Number.isFinite(tarifa) || tarifa <= 0 || tarifa > 1000000) {
    localErrors.tarifa = 'Ingresa una tarifa por noche mayor a 0.';
    valid = false;
  }

  const servicios = parseServices(form.serviciosText);
  if (servicios.length > 25) {
    localErrors.servicios = 'Se permiten máximo 25 servicios por habitación.';
    valid = false;
  }

  return valid;
}

function handleSubmit(): void {
  if (!validateClient()) {
    return;
  }

  const servicios = parseServices(form.serviciosText);
  const capacidad = Number(form.capacidad);
  const tarifa = Math.round(Number(form.tarifa) * 100) / 100;

  if (props.habitacion) {
    const payload: UpdateHabitacionPayload = {
      numero: form.numero.trim().toUpperCase(),
      tipo: form.tipo,
      capacidad,
      tarifa,
      estado: form.estado,
      servicios,
    };
    emit('saveUpdate', props.habitacion.id, payload);
  } else {
    const payload: CreateHabitacionPayload = {
      numero: form.numero.trim().toUpperCase(),
      tipo: form.tipo,
      capacidad,
      tarifa,
      estado: form.estado,
      servicios,
    };
    emit('saveCreate', payload);
  }
}
</script>

<template>
  <BaseModal
    :open="open"
    :title="isEditing ? 'Editar Habitación' : 'Registrar Habitación'"
    @close="emit('close')"
  >
    <form id="habitacion-form" class="space-y-4" @submit.prevent="handleSubmit">
      <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
        <div>
          <label class="block text-xs font-semibold text-slate-700" for="hab-numero">
            Número de habitación *
          </label>
          <input
            id="hab-numero"
            v-model="form.numero"
            type="text"
            placeholder="Ej. 101 o 204-A"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          />
          <p v-if="errorFor('numero')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('numero') }}
          </p>
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="hab-tipo">
            Tipo *
          </label>
          <select
            id="hab-tipo"
            v-model="form.tipo"
            class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          >
            <option v-for="t in tiposPermitidos" :key="t.value" :value="t.value">
              {{ t.label }}
            </option>
          </select>
          <p v-if="errorFor('tipo')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('tipo') }}
          </p>
        </div>
      </div>

      <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
        <div>
          <label class="block text-xs font-semibold text-slate-700" for="hab-capacidad">
            Capacidad máxima (personas) *
          </label>
          <input
            id="hab-capacidad"
            v-model="form.capacidad"
            type="number"
            min="1"
            max="20"
            step="1"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          />
          <p v-if="errorFor('capacidad')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('capacidad') }}
          </p>
        </div>

        <div>
          <label class="block text-xs font-semibold text-slate-700" for="hab-tarifa">
            Tarifa por noche (MXN) *
          </label>
          <input
            id="hab-tarifa"
            v-model="form.tarifa"
            type="number"
            min="1"
            step="0.01"
            placeholder="Ej. 1450.00"
            class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
          />
          <p v-if="errorFor('tarifa')" class="mt-1 text-xs text-rose-600">
            {{ errorFor('tarifa') }}
          </p>
        </div>
      </div>

      <div>
        <label class="block text-xs font-semibold text-slate-700" for="hab-estado">
          Estado operativo
        </label>
        <select
          id="hab-estado"
          v-model="form.estado"
          class="mt-1 w-full rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        >
          <option v-for="e in estadosPermitidos" :key="e.value" :value="e.value">
            {{ e.label }}
          </option>
        </select>
        <p v-if="errorFor('estado')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('estado') }}
        </p>
      </div>

      <div>
        <label class="block text-xs font-semibold text-slate-700" for="hab-servicios">
          Servicios incluidos (separados por coma)
        </label>
        <input
          id="hab-servicios"
          v-model="form.serviciosText"
          type="text"
          placeholder="Ej. wifi, tv, aire_acondicionado, balcon"
          class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        />
        <p v-if="errorFor('servicios')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('servicios') }}
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
        form="habitacion-form"
        class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white transition hover:bg-indigo-700 disabled:opacity-50"
        :disabled="submitting"
      >
        {{ submitting ? 'Guardando...' : isEditing ? 'Guardar cambios' : 'Registrar habitación' }}
      </button>
    </template>
  </BaseModal>
</template>