<template>
  <v-app>
    <v-navigation-drawer v-model="drawer" app clipped>
      <v-list density="compact">
        <v-list-item
          prepend-icon="$home"
          title="Monitor del banco"
          to="/"
        />
      </v-list>
    </v-navigation-drawer>

    <v-app-bar app clipped-left color="blue-grey-darken-3">
      <v-app-bar-nav-icon @click="drawer = !drawer" />
      <v-app-bar-title>BPGA Aeronáutico · Banco de Prueba de Generadores</v-app-bar-title>
    </v-app-bar>

    <v-main>
      <v-container fill-height fluid>
        <router-view />
      </v-container>
    </v-main>

    <v-footer app class="px-4 py-3" color="blue-grey-darken-4">
      <v-row align="center" justify="space-between" no-gutters>
        <v-col cols="auto">
          <span class="text-grey-lighten-3 text-body-2">
            BPGA Aeronáutico · IMPA Taller Regional Quilmes · {{ new Date().getFullYear() }}
          </span>
        </v-col>
      </v-row>
    </v-footer>
  </v-app>
</template>

<script setup>
  import { ref } from 'vue'

  const drawer = ref(null)
</script>
