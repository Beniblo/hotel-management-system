<script setup lang="ts">
import { computed, reactive, watch } from 'vue';
import BaseModal from '@/components/ui/BaseModal.vue';
import type {
  CreateHuespedPayload,
  Huesped,
  UpdateHuespedPayload,
} from '@/types/hotel';

const props = defineProps<{
  open: boolean;
  huesped?: Huesped | null;
  submitting?: boolean;
  serverFieldErrors?: Record<string, string>;
}>();

const emit = defineEmits<{
  close: [];
  saveCreate: [payload: CreateHuespedPayload];
  saveUpdate: [id: string, payload: UpdateHuespedPayload];
}>();

const isEditing = computed(() => Boolean(props.huesped));

const form = reactive({
  nombre: '',
  correo: '',
  telefono: '',
  preferenciasText: '',
  activo: true,
});

const localErrors = reactive<Record<string, string>>({});

function resetForm(): void {
  localErrors.nombre = '';
  localErrors.correo = '';
  localErrors.telefono = '';
  localErrors.preferencias = '';

  if (props.huesped) {
    form.nombre = props.huesped.nombre;
    form.correo = props.huesped.correo;
    form.telefono = props.huesped.telefono;
    form.preferenciasText = props.huesped.preferencias.join(', ');
    form.activo = props.huesped.activo;
  } else {
    form.nombre = '';
    form.correo = '';
    form.telefono = '';
    form.preferenciasText = '';
    form.activo = true;
  }
}

watch(
  () => [props.open, props.huesped],
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

function parsePreferences(raw: string): string[] {
  return raw
    .split(',')
    .map((s) => s.trim())
    .filter((s) => s.length > 0);
}

function validateClient(): boolean {
  localErrors.nombre = '';
  localErrors.correo = '';
  localErrors.telefono = '';
  localErrors.preferencias = '';

  let valid = true;
  const trimmedName = form.nombre.trim();
  const trimmedEmail = form.correo.trim();
  const trimmedPhone = form.telefono.trim();

  if (trimmedName.length < 2 || trimmedName.length > 120) {
    localErrors.nombre = 'El nombre debe tener entre 2 y 120 caracteres.';
    valid = false;
  }

  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]{2,}$/;
  if (!emailRegex.test(trimmedEmail)) {
    localErrors.correo = 'Ingresa un correo electrónico válido.';
    valid = false;
  }

  const digitsCount = (trimmedPhone.match(/\d/g) ?? []).length;
  if (digitsCount < 7 || digitsCount > 15) {
    localErrors.telefono = 'El teléfono debe contener entre 7 y 15 dígitos.';
    valid = false;
  }

  const prefs = parsePreferences(form.preferenciasText);
  if (prefs.length > 25) {
    localErrors.preferencias = 'Se permiten máximo 25 preferencias.';
    valid = false;
  }

  return valid;
}

function handleSubmit(): void {
  if (!validateClient()) {
    return;
  }

  const preferencias = parsePreferences(form.preferenciasText);

  if (props.huesped) {
    const payload: UpdateHuespedPayload = {
      nombre: form.nombre.trim(),
      correo: form.correo.trim().toLowerCase(),
      telefono: form.telefono.trim(),
      preferencias,
      activo: form.activo,
    };
    emit('saveUpdate', props.huesped.id, payload);
  } else {
    const payload: CreateHuespedPayload = {
      nombre: form.nombre.trim(),
      correo: form.correo.trim().toLowerCase(),
      telefono: form.telefono.trim(),
      preferencias,
    };
    emit('saveCreate', payload);
  }
}
</script>

<template>
  <BaseModal
    :open="open"
    :title="isEditing ? 'Editar Huésped' : 'Registrar Huésped'"
    @close="emit('close')"
  >
    <form id="huesped-form" class="space-y-4" @submit.prevent="handleSubmit">
      <div>
        <label class="block text-xs font-semibold text-slate-700" for="h-nombre">
          Nombre completo *
        </label>
        <input
          id="h-nombre"
          v-model="form.nombre"
          type="text"
          placeholder="Ej. Ana López"
          class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        />
        <p v-if="errorFor('nombre')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('nombre') }}
        </p>
      </div>

      <div>
        <label class="block text-xs font-semibold text-slate-700" for="h-correo">
          Correo electrónico *
        </label>
        <input
          id="h-correo"
          v-model="form.correo"
          type="email"
          placeholder="Ej. ana.lopez@ejemplo.com"
          class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        />
        <p v-if="errorFor('correo')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('correo') }}
        </p>
      </div>

      <div>
        <label class="block text-xs font-semibold text-slate-700" for="h-telefono">
          Teléfono *
        </label>
        <input
          id="h-telefono"
          v-model="form.telefono"
          type="text"
          placeholder="Ej. 4770000001"
          class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        />
        <p v-if="errorFor('telefono')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('telefono') }}
        </p>
      </div>

      <div>
        <label class="block text-xs font-semibold text-slate-700" for="h-prefs">
          Preferencias (separadas por coma)
        </label>
        <input
          id="h-prefs"
          v-model="form.preferenciasText"
          type="text"
          placeholder="Ej. piso alto, almohada extra"
          class="mt-1 w-full rounded-lg border border-slate-300 px-3 py-2 text-sm text-slate-900 focus:border-indigo-500 focus:outline-none"
        />
        <p v-if="errorFor('preferencias')" class="mt-1 text-xs text-rose-600">
          {{ errorFor('preferencias') }}
        </p>
      </div>

      <div v-if="isEditing" class="flex items-center gap-2 pt-1">
        <input
          id="h-activo"
          v-model="form.activo"
          type="checkbox"
          class="h-4 w-4 rounded border-slate-300 text-indigo-600 focus:ring-indigo-500"
        />
        <label for="h-activo" class="text-sm font-medium text-slate-700">
          Huésped activo
        </label>
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
        form="huesped-form"
        class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white transition hover:bg-indigo-700 disabled:opacity-50"
        :disabled="submitting"
      >
        {{ submitting ? 'Guardando...' : isEditing ? 'Guardar cambios' : 'Registrar huésped' }}
      </button>
    </template>
  </BaseModal>
</template>