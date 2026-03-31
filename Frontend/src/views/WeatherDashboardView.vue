<template>
  <v-container fluid class="pa-6">
    <v-row class="mb-2">
      <v-col cols="12" class="d-flex justify-space-between align-center">
        <div>
          <h1 class="text-h4 font-weight-bold">Weather Data Dashboard</h1>
          <div class="text-medium-emphasis">Latest reading, trends, and history</div>
        </div>

        <v-btn color="primary" prepend-icon="mdi-refresh" @click="refresh">
          Refresh
        </v-btn>
      </v-col>
    </v-row>

    <v-alert
      v-if="error"
      type="error"
      variant="tonal"
      class="mb-4"
    >
      {{ error }}
    </v-alert>

    <v-row>
      <v-col cols="12" md="3">
        <v-card rounded="xl" elevation="2">
          <v-card-text>
            <div class="text-overline">Temperature</div>
            <div class="text-h4 font-weight-bold">{{ latest?.temperature ?? '--' }} °C</div>
            <v-sparkline
              :model-value="tempSeries"
              line-width="2"
              padding="8"
              smooth
              auto-draw
            />
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="3">
        <v-card rounded="xl" elevation="2">
          <v-card-text>
            <div class="text-overline">Humidity</div>
            <div class="text-h4 font-weight-bold">{{ latest?.humidity ?? '--' }} %</div>
            <v-sparkline
              :model-value="humiditySeries"
              line-width="2"
              padding="8"
              smooth
              auto-draw
            />
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="3">
        <v-card rounded="xl" elevation="2">
          <v-card-text>
            <div class="text-overline">Pressure</div>
            <div class="text-h4 font-weight-bold">{{ latest?.pressure ?? '--' }} hPa</div>
            <v-sparkline
              :model-value="pressureSeries"
              line-width="2"
              padding="8"
              smooth
              auto-draw
            />
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="3">
        <v-card rounded="xl" elevation="2">
          <v-card-text>
            <div class="text-overline">Soil Moisture</div>
            <div class="text-h4 font-weight-bold">{{ latest?.soil_moisture ?? '--' }} %</div>
            <v-sparkline
              :model-value="soilSeries"
              line-width="2"
              padding="8"
              smooth
              auto-draw
            />
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12" md="6">
        <v-card rounded="xl" elevation="2">
          <v-card-title>Derived Values</v-card-title>
          <v-card-text>
            <v-row>
              <v-col cols="6">
                <div class="text-overline">Heat Index</div>
                <div class="text-h5">{{ latest?.heat_index ?? '--' }} °C</div>
              </v-col>
              <v-col cols="6">
                <div class="text-overline">Altitude</div>
                <div class="text-h5">{{ latest?.altitude ?? '--' }} m</div>
              </v-col>
              <v-col cols="6">
                <div class="text-overline">Device</div>
                <div class="text-body-1">{{ latest?.device_id ?? '--' }}</div>
              </v-col>
              <v-col cols="6">
                <div class="text-overline">Received At</div>
                <div class="text-body-1">{{ latest?.received_at ?? '--' }}</div>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" md="6">
        <v-card rounded="xl" elevation="2">
          <v-card-title>Current Levels</v-card-title>
          <v-card-text>
            <div class="mb-4">
              <div class="d-flex justify-space-between mb-1">
                <span>Temperature</span>
                <span>{{ latest?.temperature ?? 0 }} °C</span>
              </div>
              <v-progress-linear :model-value="Number(latest?.temperature || 0) * 2" height="18" rounded />
            </div>

            <div class="mb-4">
              <div class="d-flex justify-space-between mb-1">
                <span>Humidity</span>
                <span>{{ latest?.humidity ?? 0 }} %</span>
              </div>
              <v-progress-linear :model-value="Number(latest?.humidity || 0)" height="18" rounded color="blue" />
            </div>

            <div class="mb-4">
              <div class="d-flex justify-space-between mb-1">
                <span>Soil Moisture</span>
                <span>{{ latest?.soil_moisture ?? 0 }} %</span>
              </div>
              <v-progress-linear :model-value="Number(latest?.soil_moisture || 0)" height="18" rounded color="green" />
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12">
        <v-card rounded="xl" elevation="2">
          <v-card-title>Last 50 Readings</v-card-title>
          <v-card-text>
            <v-data-table
              :headers="headers"
              :items="readings"
              :loading="loading"
              items-per-page="10"
              class="elevation-0"
            />
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { useWeatherApi } from '../composables/useWeatherApi'

const {
  latest,
  readings,
  loading,
  error,
  refresh,
  tempSeries,
  humiditySeries,
  pressureSeries,
  soilSeries,
} = useWeatherApi()

const headers = [
  { title: 'Received At', key: 'received_at' },
  { title: 'Device', key: 'device_id' },
  { title: 'Temperature (°C)', key: 'temperature' },
  { title: 'Humidity (%)', key: 'humidity' },
  { title: 'Pressure (hPa)', key: 'pressure' },
  { title: 'Soil Moisture (%)', key: 'soil_moisture' },
  { title: 'Heat Index (°C)', key: 'heat_index' },
  { title: 'Altitude (m)', key: 'altitude' },
]
</script>