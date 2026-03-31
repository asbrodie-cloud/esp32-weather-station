<template>
  <v-app>
    <v-app-bar color="primary" density="comfortable">
      <v-app-bar-title>ESP32 Weather Station</v-app-bar-title>

      <v-btn to="/" variant="text">Weather Data</v-btn>
      <v-btn to="/automation" variant="text">Automation</v-btn>
    </v-app-bar>

    <v-main class="bg-grey-lighten-4">
      <router-view />
    </v-main>
  </v-app>
</template>

<script setup>
</script>