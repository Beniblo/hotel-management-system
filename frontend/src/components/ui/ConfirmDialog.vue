<script setup lang="ts">
import BaseModal from './BaseModal.vue';

withDefaults(
  defineProps<{
    open: boolean;
    title: string;
    message: string;
    confirmText?: string;
    cancelText?: string;
    variant?: 'danger' | 'warning' | 'primary';
    loading?: boolean;
  }>(),
  {
    confirmText: 'Confirmar',
    cancelText: 'Cancelar',
    variant: 'danger',
    loading: false,
  }
);

const emit = defineEmits<{
  confirm: [];
  cancel: [];
}>();

const confirmClasses: Record<'danger' | 'warning' | 'primary', string> = {
  danger: 'bg-rose-600 hover:bg-rose-700 text-white',
  warning: 'bg-amber-600 hover:bg-amber-700 text-white',
  primary: 'bg-indigo-600 hover:bg-indigo-700 text-white',
};
</script>

<template>
  <BaseModal :open="open" :title="title" size="sm" @close="emit('cancel')">
    <p class="text-sm leading-6 text-slate-600">{{ message }}</p>

    <template #footer>
      <button
        type="button"
        class="rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 transition hover:bg-slate-50 disabled:opacity-50"
        :disabled="loading"
        @click="emit('cancel')"
      >
        {{ cancelText }}
      </button>
      <button
        type="button"
        :class="confirmClasses[variant]"
        class="rounded-lg px-4 py-2 text-sm font-medium transition disabled:opacity-50"
        :disabled="loading"
        @click="emit('confirm')"
      >
        {{ loading ? 'Procesando...' : confirmText }}
      </button>
    </template>
  </BaseModal>
</template>