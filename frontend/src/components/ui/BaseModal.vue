<script setup lang="ts">
import { computed, onMounted, onUnmounted } from 'vue';

const props = withDefaults(
  defineProps<{
    open: boolean;
    title: string;
    size?: 'sm' | 'md' | 'lg';
  }>(),
  {
    size: 'md',
  }
);

const emit = defineEmits<{
  close: [];
}>();

const sizeClass = computed(() => {
  switch (props.size) {
    case 'sm':
      return 'max-w-md';
    case 'lg':
      return 'max-w-2xl';
    default:
      return 'max-w-lg';
  }
});

function handleKeydown(event: KeyboardEvent): void {
  if (event.key === 'Escape' && props.open) {
    emit('close');
  }
}

onMounted(() => window.addEventListener('keydown', handleKeydown));
onUnmounted(() => window.removeEventListener('keydown', handleKeydown));
</script>

<template>
  <Teleport to="body">
    <div
      v-if="open"
      class="fixed inset-0 z-40 flex items-center justify-center bg-slate-900/50 p-4 backdrop-blur-[1px]"
      role="dialog"
      aria-modal="true"
      :aria-label="title"
      @click.self="emit('close')"
    >
      <div
        :class="sizeClass"
        class="flex max-h-[90vh] w-full flex-col overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-xl"
      >
        <div class="flex items-center justify-between border-b border-slate-200 px-6 py-4">
          <h2 class="text-lg font-semibold text-slate-900">{{ title }}</h2>
          <button
            type="button"
            class="rounded-lg p-1.5 text-slate-400 transition hover:bg-slate-100 hover:text-slate-700"
            aria-label="Cerrar modal"
            @click="emit('close')"
          >
            ✕
          </button>
        </div>

        <div class="overflow-y-auto px-6 py-5">
          <slot />
        </div>

        <div
          v-if="$slots.footer"
          class="flex items-center justify-end gap-3 border-t border-slate-200 bg-slate-50 px-6 py-3.5"
        >
          <slot name="footer" />
        </div>
      </div>
    </div>
  </Teleport>
</template>