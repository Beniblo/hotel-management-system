import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router';
import DashboardView from '@/views/DashboardView.vue';
import HuespedesView from '@/views/HuespedesView.vue';
import PlaceholderView from '@/views/PlaceholderView.vue';

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    name: 'dashboard',
    component: DashboardView,
    meta: { title: 'Dashboard' },
  },
  {
    path: '/huespedes',
    name: 'huespedes',
    component: HuespedesView,
    meta: { title: 'Huéspedes' },
  },
  {
    path: '/habitaciones',
    name: 'habitaciones',
    component: PlaceholderView,
    meta: { title: 'Habitaciones' },
  },
  {
    path: '/reservaciones',
    name: 'reservaciones',
    component: PlaceholderView,
    meta: { title: 'Reservaciones' },
  },
  {
    path: '/disponibilidad',
    name: 'disponibilidad',
    component: PlaceholderView,
    meta: { title: 'Disponibilidad' },
  },
  {
    path: '/reportes',
    name: 'reportes',
    component: PlaceholderView,
    meta: { title: 'Reportes' },
  },
  {
    path: '/:pathMatch(.*)*',
    name: 'not-found',
    component: PlaceholderView,
    meta: { title: 'Página no encontrada (404)' },
  },
];

export const router = createRouter({
  history: createWebHistory(),
  routes,
});

router.afterEach((to) => {
  const pageTitle = to.meta.title
    ? `${String(to.meta.title)} | Hotel Management`
    : 'Hotel Management';
  document.title = pageTitle;
});

export default router;