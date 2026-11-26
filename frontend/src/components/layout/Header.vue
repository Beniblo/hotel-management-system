<script setup lang="ts">
import { computed } from 'vue';
import { useRoute } from 'vue-router';

const emit = defineEmits<{
  toggleMobileMenu: [];
}>();

const route = useRoute();
const pageTitle = computed(() => String(route.meta.title ?? 'Panel Administrativo'));
</script>

<template>
  <header
    class="sticky top-0 z-20 flex h-16 items-center justify-between border-b border-slate-200 bg-white/95 px-4 backdrop-blur sm:px-6"
  >
    <div class="flex items-center gap-3">
      <button
        type="button"
        class="rounded-lg border border-slate-200 p-2 text-slate-600 transition hover:bg-slate-100 lg:hidden"
        aria-label="Abrir menú de navegación"
        @click="emit('toggleMobileMenu')"
      >
        <svg class="h-5 w-5" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24">
          <path stroke-linecap="round" stroke-linejoin="round" d="M4 6h16M4 12h16M4 18h16" />
        </svg>
      </button>
      <div>
        <h1 class="text-lg font-bold text-slate-900">{{ pageTitle }}</h1>
      </div>
    </div>

    <div class="flex items-center gap-3">
      <span
        class="hidden rounded-full border border-slate-200 bg-slate-50 px-3 py-1 text-xs font-medium text-slate-600 sm:inline-flex"
      >
        API REST v1 · HotelDB
      </span>
    </div>
  </header>
</template>