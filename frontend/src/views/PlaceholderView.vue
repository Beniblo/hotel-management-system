<script setup lang="ts">
import { computed } from 'vue';
import { useRoute } from 'vue-router';

const route = useRoute();
const title = computed(() => String(route.meta.title ?? 'Vista en construcción'));
</script>

<template>
  <section class="rounded-xl border border-slate-200 bg-white p-6 shadow-sm">
    <h1 class="text-xl font-bold text-slate-900">{{ title }}</h1>
    <p class="mt-2 text-sm text-slate-600">
      Ruta activa: <code class="text-indigo-600">{{ route.fullPath }}</code>
    </p>
  </section>
</template>