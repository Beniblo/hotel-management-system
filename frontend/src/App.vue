<script setup lang="ts">
const apiBaseUrl: string = import.meta.env.VITE_API_BASE_URL || 'http://localhost:8080/api/v1';
</script>

<template>
  <main class="min-h-screen flex items-center justify-center bg-slate-100 p-6">
    <section class="w-full max-w-lg rounded-2xl bg-white p-8 shadow-md border border-slate-200">
      <div class="flex items-center gap-3 mb-4">
        <span class="inline-block h-3 w-3 rounded-full bg-emerald-500"></span>
        <span class="text-xs font-semibold uppercase tracking-wider text-emerald-700">
          Entorno Frontend Operativo (Fase 2C)
        </span>
      </div>

      <h1 class="text-2xl font-bold text-slate-900 mb-2">
        Sistema de Gestión Hotelera
      </h1>
      <p class="text-sm text-slate-600 mb-6">
        Vue 3 + TypeScript + Vite + Tailwind CSS configurados correctamente.
      </p>

      <div class="rounded-lg bg-slate-50 border border-slate-200 p-4 text-xs text-slate-700">
        <p class="font-semibold text-slate-900 mb-1">API REST objetivo:</p>
        <code class="text-indigo-700">{{ apiBaseUrl }}</code>
      </div>
    </section>
  </main>
</template>