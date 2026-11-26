<script setup lang="ts">
import { computed } from 'vue';

const props = defineProps<{
  status: string;
  label?: string;
}>();

interface BadgeStyle {
  classes: string;
  defaultLabel: string;
}

const styleMap: Record<string, BadgeStyle> = {
  // Habitaciones
  disponible: {
    classes: 'bg-emerald-100 text-emerald-800 border-emerald-200',
    defaultLabel: 'Disponible',
  },
  ocupada: {
    classes: 'bg-indigo-100 text-indigo-800 border-indigo-200',
    defaultLabel: 'Ocupada',
  },
  mantenimiento: {
    classes: 'bg-amber-100 text-amber-800 border-amber-200',
    defaultLabel: 'Mantenimiento',
  },
  fuera_de_servicio: {
    classes: 'bg-rose-100 text-rose-800 border-rose-200',
    defaultLabel: 'Fuera de servicio',
  },
  // Reservaciones
  pendiente: {
    classes: 'bg-amber-100 text-amber-800 border-amber-200',
    defaultLabel: 'Pendiente',
  },
  confirmada: {
    classes: 'bg-sky-100 text-sky-800 border-sky-200',
    defaultLabel: 'Confirmada',
  },
  en_curso: {
    classes: 'bg-indigo-100 text-indigo-800 border-indigo-200',
    defaultLabel: 'En curso',
  },
  completada: {
    classes: 'bg-emerald-100 text-emerald-800 border-emerald-200',
    defaultLabel: 'Completada',
  },
  cancelada: {
    classes: 'bg-slate-200 text-slate-700 border-slate-300',
    defaultLabel: 'Cancelada',
  },
  // Huéspedes y salud API
  activo: {
    classes: 'bg-emerald-100 text-emerald-800 border-emerald-200',
    defaultLabel: 'Activo',
  },
  inactivo: {
    classes: 'bg-slate-200 text-slate-700 border-slate-300',
    defaultLabel: 'Inactivo',
  },
  ok: {
    classes: 'bg-emerald-100 text-emerald-800 border-emerald-200',
    defaultLabel: 'Operativo',
  },
  degraded: {
    classes: 'bg-rose-100 text-rose-800 border-rose-200',
    defaultLabel: 'Sin conexión',
  },
};

const resolved = computed<BadgeStyle>(() => {
  const key = props.status.toLowerCase();
  return (
    styleMap[key] ?? {
      classes: 'bg-slate-100 text-slate-700 border-slate-200',
      defaultLabel: props.status,
    }
  );
});
</script>

<template>
  <span
    :class="resolved.classes"
    class="inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-medium whitespace-nowrap"
  >
    {{ label ?? resolved.defaultLabel }}
  </span>
</template>