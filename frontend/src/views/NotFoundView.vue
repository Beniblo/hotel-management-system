<script setup lang="ts"></script>

<template>
  <section class="mx-auto max-w-lg rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">
    <p class="text-xs font-bold uppercase tracking-widest text-indigo-600">Error 404</p>
    <h2 class="mt-2 text-2xl font-bold text-slate-900">Página no encontrada</h2>
    <p class="mt-2 text-sm text-slate-600">
      La ruta que intentas visitar no existe dentro del panel administrativo.
    </p>
    <RouterLink
      to="/"
      class="mt-6 inline-flex items-center rounded-lg bg-indigo-600 px-4 py-2 text-sm font-semibold text-white transition hover:bg-indigo-700"
    >
      Volver al Dashboard
    </RouterLink>
  </section>
</template>