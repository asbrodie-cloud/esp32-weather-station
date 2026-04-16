<template>
  <v-container fluid class="pa-6">
    <v-row class="mb-2">
      <v-col cols="12" class="d-flex justify-space-between align-center flex-wrap ga-3">
        <div>
          <h1 class="text-h4 font-weight-bold">Weather Data Dashboard</h1>
          <div class="text-medium-emphasis">Latest reading, trends, and history</div>
        </div>

        <div class="d-flex flex-wrap ga-3">
          <v-btn-toggle v-model="temperatureUnit" mandatory divided density="comfortable">
            <v-btn value="C">°C</v-btn>
            <v-btn value="F">°F</v-btn>
          </v-btn-toggle>

          <v-btn-toggle v-model="pressureUnit" mandatory divided density="comfortable">
            <v-btn value="hPa">hPa</v-btn>
            <v-btn value="kPa">kPa</v-btn>
          </v-btn-toggle>

          <v-btn-toggle v-model="altitudeUnit" mandatory divided density="comfortable">
            <v-btn value="m">m</v-btn>
            <v-btn value="ft">ft</v-btn>
          </v-btn-toggle>

          <v-btn color="primary" prepend-icon="mdi-refresh" @click="refresh">
            Refresh
          </v-btn>
        </div>
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
            <div class="text-h4 font-weight-bold">
              {{ displayTemperature(latest?.temperature) }} {{ temperatureUnitSymbol }}
            </div>
            <v-sparkline
              :model-value="displayTempSeries"
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
            <div class="text-h4 font-weight-bold">
              {{ displayPressure(latest?.pressure) }} {{ pressureUnit }}
            </div>
            <v-sparkline
              :model-value="displayPressureSeries"
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
                <div class="text-h5">
                  {{ displayTemperature(latest?.heat_index) }} {{ temperatureUnitSymbol }}
                </div>
              </v-col>
              <v-col cols="6">
                <div class="text-overline">Altitude</div>
                <div class="text-h5">
                  {{ displayAltitude(latest?.altitude) }} {{ altitudeUnit }}
                </div>
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
                <span>{{ displayTemperature(latest?.temperature) }} {{ temperatureUnitSymbol }}</span>
              </div>
              <v-progress-linear
                :model-value="temperatureBarValue"
                height="18"
                rounded
              />
            </div>

            <div class="mb-4">
              <div class="d-flex justify-space-between mb-1">
                <span>Humidity</span>
                <span>{{ latest?.humidity ?? 0 }} %</span>
              </div>
              <v-progress-linear
                :model-value="Number(latest?.humidity || 0)"
                height="18"
                rounded
                color="blue"
              />
            </div>

            <div class="mb-4">
              <div class="d-flex justify-space-between mb-1">
                <span>Soil Moisture</span>
                <span>{{ latest?.soil_moisture ?? 0 }} %</span>
              </div>
              <v-progress-linear
                :model-value="Number(latest?.soil_moisture || 0)"
                height="18"
                rounded
                color="green"
              />
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
              :items="formattedReadings"
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
import { ref, computed } from 'vue'
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
  soilSeries
} = useWeatherApi()

const temperatureUnit = ref('C')
const pressureUnit = ref('hPa')
const altitudeUnit = ref('m')

const temperatureUnitSymbol = computed(() => temperatureUnit.value === 'C' ? '°C' : '°F')

function convertTemperature(value) {
  if (value === null || value === undefined || value === '') return '--'
  const num = Number(value)
  if (Number.isNaN(num)) return '--'
  return temperatureUnit.value === 'C'
    ? num
    : (num * 9 / 5) + 32
}

function convertPressure(value) {
  if (value === null || value === undefined || value === '') return '--'
  const num = Number(value)
  if (Number.isNaN(num)) return '--'
  return pressureUnit.value === 'hPa'
    ? num
    : num / 10
}

function convertAltitude(value) {
  if (value === null || value === undefined || value === '') return '--'
  const num = Number(value)
  if (Number.isNaN(num)) return '--'
  return altitudeUnit.value === 'm'
    ? num
    : num * 3.28084
}

function displayTemperature(value) {
  const converted = convertTemperature(value)
  return converted === '--' ? '--' : converted.toFixed(1)
}

function displayPressure(value) {
  const converted = convertPressure(value)
  return converted === '--'
    ? '--'
    : (pressureUnit.value === 'hPa' ? converted.toFixed(1) : converted.toFixed(2))
}

function displayAltitude(value) {
  const converted = convertAltitude(value)
  return converted === '--'
    ? '--'
    : (altitudeUnit.value === 'm' ? converted.toFixed(1) : converted.toFixed(1))
}

const displayTempSeries = computed(() =>
  tempSeries.value.map(v => Number(convertTemperature(v)))
)

const displayPressureSeries = computed(() =>
  pressureSeries.value.map(v => Number(convertPressure(v)))
)

const temperatureBarValue = computed(() => {
  const tempC = Number(latest.value?.temperature || 0)
  return Math.max(0, Math.min(tempC * 2, 100))
})

const formattedReadings = computed(() =>
  readings.value.map(r => ({
    ...r,
    temperature_display: `${displayTemperature(r.temperature)} ${temperatureUnitSymbol.value}`,
    humidity_display: `${r.humidity ?? '--'} %`,
    pressure_display: `${displayPressure(r.pressure)} ${pressureUnit.value}`,
    soil_display: `${r.soil_moisture ?? '--'} %`,
    heat_index_display: `${displayTemperature(r.heat_index)} ${temperatureUnitSymbol.value}`,
    altitude_display: `${displayAltitude(r.altitude)} ${altitudeUnit.value}`,
  }))
)

const headers = computed(() => [
  { title: 'Received At', key: 'received_at' },
  { title: 'Device', key: 'device_id' },
  { title: `Temperature (${temperatureUnitSymbol.value})`, key: 'temperature_display' },
  { title: 'Humidity (%)', key: 'humidity_display' },
  { title: `Pressure (${pressureUnit.value})`, key: 'pressure_display' },
  { title: 'Soil Moisture (%)', key: 'soil_display' },
  { title: `Heat Index (${temperatureUnitSymbol.value})`, key: 'heat_index_display' },
  { title: `Altitude (${altitudeUnit.value})`, key: 'altitude_display' }
])
</script>