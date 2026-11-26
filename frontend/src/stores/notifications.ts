import { defineStore } from 'pinia';
import { ref } from 'vue';

export type ToastType = 'success' | 'error' | 'warning' | 'info';

export interface ToastItem {
  id: number;
  type: ToastType;
  title: string;
  message?: string;
}

export const useNotificationStore = defineStore('notifications', () => {
  const items = ref<ToastItem[]>([]);
  let nextId = 1;

  function dismiss(id: number): void {
    items.value = items.value.filter((item) => item.id !== id);
  }

  function push(
    type: ToastType,
    title: string,
    message?: string,
    durationMs = 4500
  ): void {
    const id = nextId++;
    items.value.push({ id, type, title, message });

    if (durationMs > 0) {
      window.setTimeout(() => {
        dismiss(id);
      }, durationMs);
    }
  }

  function success(title: string, message?: string): void {
    push('success', title, message);
  }

  function error(title: string, message?: string): void {
    push('error', title, message, 6000);
  }

  function warning(title: string, message?: string): void {
    push('warning', title, message);
  }

  function info(title: string, message?: string): void {
    push('info', title, message);
  }

  return {
    items,
    dismiss,
    push,
    success,
    error,
    warning,
    info,
  };
});