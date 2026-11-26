<script setup lang="ts">
import { useRoute } from 'vue-router';

defineProps<{
  mobileOpen: boolean;
}>();

const emit = defineEmits<{
  closeMobile: [];
}>();

const route = useRoute();

interface NavItem {
  to: string;
  name: string;
  label: string;
  shortIcon: string;
}

const navItems: NavItem[] = [
  { to: '/', name: 'dashboard', label: 'Dashboard', shortIcon: 'DB' },
  { to: '/huespedes', name: 'huespedes', label: 'Huéspedes', shortIcon: 'HU' },
  { to: '/habitaciones', name: 'habitaciones', label: 'Habitaciones', shortIcon: 'HA' },
  { to: '/reservaciones', name: 'reservaciones', label: 'Reservaciones', shortIcon: 'RE' },
  { to: '/disponibilidad', name: 'disponibilidad', label: 'Disponibilidad', shortIcon: 'DI' },
  { to: '/reportes', name: 'reportes', label: 'Reportes', shortIcon: 'RP' },
];

function isActive(name: string): boolean {
  return route.name === name;
}
</script>

<template>
  <!-- Fondo oscuro en móvil -->
  <div
    v-if="mobileOpen"
    class="fixed inset-0 z-30 bg-slate-900/50 lg:hidden"
    @click="emit('closeMobile')"
  />

  <aside
    :class="mobileOpen ? 'translate-x-0' : '-translate-x-full lg:translate-x-0'"
    class="fixed inset-y-0 left-0 z-40 flex w-64 flex-col bg-slate-900 text-slate-100 transition-transform duration-200 lg:static lg:inset-auto"
  >
    <div class="flex h-16 items-center justify-between border-b border-slate-800 px-5">
      <div class="flex items-center gap-3">
        <div
          class="flex h-9 w-9 items-center justify-center rounded-lg bg-indigo-600 text-sm font-bold text-white"
        >
          HM
        </div>
        <div>
          <p class="text-sm font-bold leading-tight text-white">Hotel Manager</p>
          <p class="text-[11px] text-slate-400">Panel Administrativo</p>
        </div>
      </div>
      <button
        type="button"
        class="rounded-md p-1 text-slate-400 hover:bg-slate-800 hover:text-white lg:hidden"
        aria-label="Cerrar menú"
        @click="emit('closeMobile')"
      >
        ✕
      </button>
    </div>

    <nav class="flex-1 space-y-1 overflow-y-auto px-3 py-4" aria-label="Navegación principal">
      <RouterLink
        v-for="item in navItems"
        :key="item.name"
        :to="item.to"
        :class="
          isActive(item.name)
            ? 'bg-indigo-600 text-white shadow-sm'
            : 'text-slate-300 hover:bg-slate-800 hover:text-white'
        "
        class="flex items-center gap-3 rounded-lg px-3 py-2.5 text-sm font-medium transition"
        @click="emit('closeMobile')"
      >
        <span
          :class="isActive(item.name) ? 'bg-indigo-500 text-white' : 'bg-slate-800 text-slate-400'"
          class="flex h-7 w-7 items-center justify-center rounded-md text-[11px] font-semibold"
        >
          {{ item.shortIcon }}
        </span>
        <span>{{ item.label }}</span>
      </RouterLink>
    </nav>

    <div class="border-t border-slate-800 px-5 py-3 text-[11px] text-slate-400">
      <p class="font-medium text-slate-300">Backend C++20 / Drogon</p>
      <p>MongoDB Atlas · HotelDB</p>
    </div>
  </aside>
</template>