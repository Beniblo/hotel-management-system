<script setup lang="ts" generic="T extends object">
import type { PaginationMeta } from '@/types/api';

export interface TableColumn {
  key: string;
  label: string;
  align?: 'left' | 'center' | 'right';
}

const props = withDefaults(
  defineProps<{
    columns: TableColumn[];
    rows: T[];
    rowKey?: string;
    loading?: boolean;
    emptyTitle?: string;
    emptyDescription?: string;
    meta?: PaginationMeta | null;
    hasActions?: boolean;
  }>(),
  {
    rowKey: 'id',
    loading: false,
    emptyTitle: 'Sin registros',
    emptyDescription: 'No se encontraron elementos con los criterios seleccionados.',
    meta: null,
    hasActions: false,
  }
);

const emit = defineEmits<{
  pageChange: [page: number];
}>();

function getRowKey(row: T, index: number): string | number {
  const value = (row as Record<string, unknown>)[props.rowKey];
  return typeof value === 'string' || typeof value === 'number' ? value : index;
}

function getCellValue(row: T, key: string): unknown {
  return (row as Record<string, unknown>)[key] ?? '—';
}

function alignClass(align?: 'left' | 'center' | 'right'): string {
  if (align === 'right') return 'text-right';
  if (align === 'center') return 'text-center';
  return 'text-left';
}
</script>

<template>
  <div class="overflow-hidden rounded-xl border border-slate-200 bg-white shadow-sm">
    <div class="overflow-x-auto">
      <table class="min-w-full divide-y divide-slate-200 text-sm">
        <thead class="bg-slate-50">
          <tr>
            <th
              v-for="col in columns"
              :key="col.key"
              :class="alignClass(col.align)"
              class="px-4 py-3 text-xs font-semibold uppercase tracking-wider text-slate-600"
            >
              {{ col.label }}
            </th>
            <th
              v-if="hasActions"
              class="px-4 py-3 text-right text-xs font-semibold uppercase tracking-wider text-slate-600"
            >
              Acciones
            </th>
          </tr>
        </thead>

        <tbody class="divide-y divide-slate-100 bg-white">
          <template v-if="loading">
            <tr v-for="n in 4" :key="`skeleton-${n}`">
              <td
                v-for="col in columns"
                :key="`${n}-${col.key}`"
                class="px-4 py-3.5"
              >
                <div class="h-4 w-3/4 animate-pulse rounded bg-slate-200" />
              </td>
              <td v-if="hasActions" class="px-4 py-3.5">
                <div class="ml-auto h-4 w-20 animate-pulse rounded bg-slate-200" />
              </td>
            </tr>
          </template>

          <tr v-else-if="rows.length === 0">
            <td
              :colspan="columns.length + (hasActions ? 1 : 0)"
              class="px-6 py-12 text-center"
            >
              <p class="text-sm font-semibold text-slate-800">{{ emptyTitle }}</p>
              <p class="mt-1 text-xs text-slate-500">{{ emptyDescription }}</p>
            </td>
          </tr>

          <tr
            v-for="(row, index) in rows"
            v-else
            :key="getRowKey(row, index)"
            class="transition hover:bg-slate-50/80"
          >
            <td
              v-for="col in columns"
              :key="col.key"
              :class="alignClass(col.align)"
              class="px-4 py-3.5 text-slate-700"
            >
              <slot :name="`cell-${col.key}`" :row="row" :value="getCellValue(row, col.key)">
                {{ getCellValue(row, col.key) }}
              </slot>
            </td>
            <td v-if="hasActions" class="px-4 py-3.5 text-right whitespace-nowrap">
              <slot name="actions" :row="row" />
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div
      v-if="meta && meta.totalPages > 1"
      class="flex flex-wrap items-center justify-between gap-3 border-t border-slate-200 bg-slate-50 px-4 py-3 text-xs text-slate-600"
    >
      <span>
        Página <strong class="text-slate-900">{{ meta.page }}</strong> de
        <strong class="text-slate-900">{{ meta.totalPages }}</strong>
        ({{ meta.total }} registros)
      </span>
      <div class="flex items-center gap-2">
        <button
          type="button"
          class="rounded-lg border border-slate-300 bg-white px-3 py-1.5 font-medium text-slate-700 transition hover:bg-slate-100 disabled:opacity-40"
          :disabled="meta.page <= 1 || loading"
          @click="emit('pageChange', meta.page - 1)"
        >
          Anterior
        </button>
        <button
          type="button"
          class="rounded-lg border border-slate-300 bg-white px-3 py-1.5 font-medium text-slate-700 transition hover:bg-slate-100 disabled:opacity-40"
          :disabled="meta.page >= meta.totalPages || loading"
          @click="emit('pageChange', meta.page + 1)"
        >
          Siguiente
        </button>
      </div>
    </div>
  </div>
</template>