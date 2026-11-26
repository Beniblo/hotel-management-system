<script setup lang="ts">
import { useNotificationStore, type ToastType } from '@/stores/notifications';

const notifications = useNotificationStore();

const typeClasses: Record<ToastType, string> = {
  success: 'border-emerald-200 bg-emerald-50 text-emerald-900',
  error: 'border-rose-200 bg-rose-50 text-rose-900',
  warning: 'border-amber-200 bg-amber-50 text-amber-900',
  info: 'border-sky-200 bg-sky-50 text-sky-900',
};

const dotClasses: Record<ToastType, string> = {
  success: 'bg-emerald-500',
  error: 'bg-rose-500',
  warning: 'bg-amber-500',
  info: 'bg-sky-500',
};
</script>

<template>
  <Teleport to="body">
    <div
      class="pointer-events-none fixed right-4 top-4 z-50 flex w-full max-w-sm flex-col gap-2.5"
      aria-live="polite"
    >
      <div
        v-for="item in notifications.items"
        :key="item.id"
        :class="typeClasses[item.type]"
        class="pointer-events-auto flex items-start gap-3 rounded-xl border p-4 shadow-lg transition-all"
        role="status"
      >
        <span
          :class="dotClasses[item.type]"
          class="mt-1.5 h-2.5 w-2.5 shrink-0 rounded-full"
        />
        <div class="min-w-0 flex-1">
          <p class="text-sm font-semibold leading-5">{{ item.title }}</p>
          <p v-if="item.message" class="mt-0.5 text-xs opacity-90">
            {{ item.message }}
          </p>
        </div>
        <button
          type="button"
          class="rounded-md p-1 text-xs opacity-60 transition hover:opacity-100"
          aria-label="Cerrar notificación"
          @click="notifications.dismiss(item.id)"
        >
          ✕
        </button>
      </div>
    </div>
  </Teleport>
</template>