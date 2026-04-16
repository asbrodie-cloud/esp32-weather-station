<template>
  <v-container fluid class="pa-6">
    <v-row class="mb-2">
      <v-col cols="12">
        <h1 class="text-h4 font-weight-bold">Automation View</h1>
        <div class="text-medium-emphasis">
          Automatic and manual device control based on live sensor thresholds
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
      <!-- AC -->
      <v-col cols="12" md="4">
        <v-card rounded="xl" elevation="3" class="fill-height">
          <v-card-title class="d-flex justify-space-between align-center">
            <span>Air Conditioner</span>
            <v-chip :color="acDisplayOn ? 'green' : 'grey'" variant="flat">
              {{ acDisplayOn ? 'ON' : 'OFF' }}
            </v-chip>
          </v-card-title>

          <v-card-text class="text-center">
            <div class="device-stage">
              <div class="ac-unit">
                <v-icon size="80" :class="{ 'fan-spin': acDisplayOn }">
                  mdi-air-conditioner
                </v-icon>
              </div>

              <div class="air-stream" :class="{ active: acDisplayOn }">
                <span></span><span></span><span></span>
              </div>
            </div>

            <div class="text-body-1 mt-4">
              Temperature: <strong>{{ latest?.temperature ?? '--' }} °C</strong>
            </div>
            <div class="text-caption text-medium-emphasis mb-4">
              Auto turns on when temperature ≥ 25°C
            </div>

            <v-btn-toggle
              :model-value="acMode"
              mandatory
              divided
              class="mb-2"
            >
              <v-btn @click="setDeviceMode('ac', 'auto')">Auto</v-btn>
              <v-btn @click="setDeviceMode('ac', 'on')">On</v-btn>
              <v-btn @click="setDeviceMode('ac', 'off')">Off</v-btn>
            </v-btn-toggle>

            <div class="text-caption">
              Mode: <strong>{{ acModeLabel }}</strong>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- Sprinkler -->
      <v-col cols="12" md="4">
        <v-card rounded="xl" elevation="3" class="fill-height">
          <v-card-title class="d-flex justify-space-between align-center">
            <span>Sprinklers</span>
            <v-chip :color="sprinklerDisplayOn ? 'green' : 'grey'" variant="flat">
              {{ sprinklerDisplayOn ? 'ON' : 'OFF' }}
            </v-chip>
          </v-card-title>

          <v-card-text class="text-center">
            <div class="device-stage">
              <div class="sprinkler-head">
                <v-icon size="72">mdi-sprinkler-variant</v-icon>
              </div>

              <div class="water-spray" :class="{ active: sprinklerDisplayOn }">
                <span></span><span></span><span></span><span></span><span></span>
              </div>
            </div>

            <div class="text-body-1 mt-4">
              Soil Moisture: <strong>{{ latest?.soil_moisture ?? '--' }} %</strong>
            </div>
            <div class="text-caption text-medium-emphasis mb-4">
              Auto turns on when soil moisture ≤ 30%
            </div>

            <v-btn-toggle
              :model-value="sprinklerMode"
              mandatory
              divided
              class="mb-2"
            >
              <v-btn @click="setDeviceMode('sprinkler', 'auto')">Auto</v-btn>
              <v-btn @click="setDeviceMode('sprinkler', 'on')">On</v-btn>
              <v-btn @click="setDeviceMode('sprinkler', 'off')">Off</v-btn>
            </v-btn-toggle>

            <div class="text-caption">
              Mode: <strong>{{ sprinklerModeLabel }}</strong>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- Dehumidifier -->
      <v-col cols="12" md="4">
        <v-card rounded="xl" elevation="3" class="fill-height">
          <v-card-title class="d-flex justify-space-between align-center">
            <span>Dehumidifier</span>
            <v-chip :color="dehumidifierDisplayOn ? 'green' : 'grey'" variant="flat">
              {{ dehumidifierDisplayOn ? 'ON' : 'OFF' }}
            </v-chip>
          </v-card-title>

          <v-card-text class="text-center">
            <div class="device-stage">
              <div class="dehumidifier-box">
                <v-icon size="76">mdi-air-humidifier-off</v-icon>
              </div>

              <div class="drop-capture" :class="{ active: dehumidifierDisplayOn }">
                <span></span><span></span><span></span>
              </div>
            </div>

            <div class="text-body-1 mt-4">
              Humidity: <strong>{{ latest?.humidity ?? '--' }} %</strong>
            </div>
            <div class="text-caption text-medium-emphasis mb-4">
              Auto turns on when humidity ≥ 70%
            </div>

            <v-btn-toggle
              :model-value="dehumidifierMode"
              mandatory
              divided
              class="mb-2"
            >
              <v-btn @click="setDeviceMode('dehumidifier', 'auto')">Auto</v-btn>
              <v-btn @click="setDeviceMode('dehumidifier', 'on')">On</v-btn>
              <v-btn @click="setDeviceMode('dehumidifier', 'off')">Off</v-btn>
            </v-btn-toggle>

            <div class="text-caption">
              Mode: <strong>{{ dehumidifierModeLabel }}</strong>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-1">
      <v-col cols="12">
        <v-card rounded="xl" elevation="2">
          <v-card-title>Automation Summary</v-card-title>
          <v-card-text>
            <v-row>
              <v-col cols="12" md="4">
                <v-progress-linear
                  :model-value="acDisplayOn ? 100 : 0"
                  height="20"
                  rounded
                  color="blue"
                >
                  <strong>AC {{ acDisplayOn ? 'ACTIVE' : 'IDLE' }}</strong>
                </v-progress-linear>
              </v-col>

              <v-col cols="12" md="4">
                <v-progress-linear
                  :model-value="sprinklerDisplayOn ? 100 : 0"
                  height="20"
                  rounded
                  color="green"
                >
                  <strong>SPRINKLER {{ sprinklerDisplayOn ? 'ACTIVE' : 'IDLE' }}</strong>
                </v-progress-linear>
              </v-col>

              <v-col cols="12" md="4">
                <v-progress-linear
                  :model-value="dehumidifierDisplayOn ? 100 : 0"
                  height="20"
                  rounded
                  color="deep-purple"
                >
                  <strong>DEHUMIDIFIER {{ dehumidifierDisplayOn ? 'ACTIVE' : 'IDLE' }}</strong>
                </v-progress-linear>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { computed } from 'vue'
import { useWeatherApi } from '../composables/useWeatherApi'

const {
  latest,
  error,
  refresh,
  acOn,
  sprinklerOn,
  dehumidifierOn
} = useWeatherApi()

async function sendControl(device, mode, state) {
  try {
    const res = await fetch(`/api/control/${device}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ mode, state })
    })

    if (!res.ok) {
      throw new Error('Failed to send control command')
    }

    // Refresh soon after sending
    setTimeout(() => {
      refresh()
    }, 300)

    // Refresh again a little later in case backend/db update lags
    setTimeout(() => {
      refresh()
    }, 1200)

  } catch (err) {
    console.error(err)
  }
}

function setDeviceMode(device, selection) {
  if (selection === 'auto') {
    sendControl(device, 'auto', false)
  } else if (selection === 'on') {
    sendControl(device, 'manual', true)
  } else if (selection === 'off') {
    sendControl(device, 'manual', false)
  }
}

const acMode = computed(() => {
  if (latest.value?.ac_manual === true) {
    return latest.value?.ac_state ? 'on' : 'off'
  }
  return 'auto'
})

const sprinklerMode = computed(() => {
  if (latest.value?.sprinkler_manual === true) {
    return latest.value?.sprinkler_state ? 'on' : 'off'
  }
  return 'auto'
})

const dehumidifierMode = computed(() => {
  if (latest.value?.dehumidifier_manual === true) {
    return latest.value?.dehumidifier_state ? 'on' : 'off'
  }
  return 'auto'
})

const acDisplayOn = computed(() => !!latest.value?.ac_state)
const sprinklerDisplayOn = computed(() => !!latest.value?.sprinkler_state)
const dehumidifierDisplayOn = computed(() => !!latest.value?.dehumidifier_state)

const acModeLabel = computed(() => {
  if (acMode.value === 'auto') return 'Automatic'
  if (acMode.value === 'on') return 'Manual ON'
  return 'Manual OFF'
})

const sprinklerModeLabel = computed(() => {
  if (sprinklerMode.value === 'auto') return 'Automatic'
  if (sprinklerMode.value === 'on') return 'Manual ON'
  return 'Manual OFF'
})

const dehumidifierModeLabel = computed(() => {
  if (dehumidifierMode.value === 'auto') return 'Automatic'
  if (dehumidifierMode.value === 'on') return 'Manual ON'
  return 'Manual OFF'
})
</script>

<style scoped>
.device-stage {
  position: relative;
  height: 180px;
  border-radius: 20px;
  background: linear-gradient(180deg, #f8fbff 0%, #eef3f8 100%);
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
}

.fan-spin {
  animation: spin 1.4s linear infinite;
}

.air-stream {
  position: absolute;
  bottom: 26px;
  display: flex;
  gap: 10px;
  opacity: 0.15;
}

.air-stream span {
  width: 8px;
  height: 40px;
  border-radius: 999px;
  background: #64b5f6;
  transform: translateY(0);
}

.air-stream.active {
  opacity: 1;
}

.air-stream.active span {
  animation: breeze 0.9s ease-in-out infinite alternate;
}

.air-stream.active span:nth-child(2) {
  animation-delay: 0.15s;
}

.air-stream.active span:nth-child(3) {
  animation-delay: 0.3s;
}

.water-spray {
  position: absolute;
  top: 55px;
  display: flex;
  gap: 8px;
  opacity: 0;
}

.water-spray.active {
  opacity: 1;
}

.water-spray span {
  width: 6px;
  height: 60px;
  background: linear-gradient(to bottom, #42a5f5, transparent);
  border-radius: 999px;
  animation: waterFall 0.8s infinite ease-in-out;
}

.water-spray span:nth-child(2) { animation-delay: 0.1s; }
.water-spray span:nth-child(3) { animation-delay: 0.2s; }
.water-spray span:nth-child(4) { animation-delay: 0.3s; }
.water-spray span:nth-child(5) { animation-delay: 0.4s; }

.drop-capture {
  position: absolute;
  bottom: 26px;
  display: flex;
  gap: 10px;
  opacity: 0;
}

.drop-capture.active {
  opacity: 1;
}

.drop-capture span {
  width: 10px;
  height: 18px;
  background: #7e57c2;
  border-radius: 50%;
  transform: rotate(45deg);
  animation: dropPulse 0.9s infinite ease-in-out;
}

.drop-capture span:nth-child(2) { animation-delay: 0.15s; }
.drop-capture span:nth-child(3) { animation-delay: 0.3s; }

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

@keyframes breeze {
  from { transform: translateY(0); opacity: 0.5; }
  to { transform: translateY(10px); opacity: 1; }
}

@keyframes waterFall {
  from { transform: translateY(-6px); opacity: 0.4; }
  to { transform: translateY(10px); opacity: 1; }
}

@keyframes dropPulse {
  from { transform: rotate(45deg) translateY(0); opacity: 0.5; }
  to { transform: rotate(45deg) translateY(8px); opacity: 1; }
}
</style>