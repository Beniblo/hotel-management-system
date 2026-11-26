<script setup lang="ts">
import { ref } from 'vue';
import Sidebar from './Sidebar.vue';
import Header from './Header.vue';
import ToastContainer from '@/components/ui/ToastContainer.vue';

const mobileMenuOpen = ref(false);
</script>

<template>
  <div class="flex min-h-screen bg-slate-50 text-slate-900">
    <Sidebar
      :mobile-open="mobileMenuOpen"
      @close-mobile="mobileMenuOpen = false"
    />

    <div class="flex min-w-0 flex-1 flex-col">
      <Header @toggle-mobile-menu="mobileMenuOpen = !mobileMenuOpen" />

      <main class="mx-auto w-full max-w-7xl flex-1 p-4 sm:p-6">
        <slot />
      </main>
    </div>

    <ToastContainer />
  </div>
</template>