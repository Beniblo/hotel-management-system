import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router';
import DashboardView from '@/views/DashboardView.vue';
import HuespedesView from '@/views/HuespedesView.vue';
import HabitacionesView from '@/views/HabitacionesView.vue';
import ReservacionesView from '@/views/ReservacionesView.vue';
import DisponibilidadView from '@/views/DisponibilidadView.vue';
import ReportesView from '@/views/ReportesView.vue';
import NotFoundView from '@/views/NotFoundView.vue';

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
    component: HabitacionesView,
    meta: { title: 'Habitaciones' },
  },
  {
    path: '/reservaciones',
    name: 'reservaciones',
    component: ReservacionesView,
    meta: { title: 'Reservaciones' },
  },
  {
    path: '/disponibilidad',
    name: 'disponibilidad',
    component: DisponibilidadView,
    meta: { title: 'Disponibilidad' },
  },
  {
    path: '/reportes',
    name: 'reportes',
    component: ReportesView,
    meta: { title: 'Reportes' },
  },
  {
    path: '/:pathMatch(.*)*',
    name: 'not-found',
    component: NotFoundView,
    meta: { title: 'Página no encontrada' },
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