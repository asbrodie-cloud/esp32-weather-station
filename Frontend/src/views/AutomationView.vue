<template>
  <v-container fluid class="pa-6">
    <v-row class="mb-2">
      <v-col cols="12">
        <h1 class="text-h4 font-weight-bold">Automation View</h1>
        <div class="text-medium-emphasis">
          Device animations and control state based on live sensor thresholds
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
            <v-chip :color="acOn ? 'green' : 'grey'" variant="flat">
              {{ acOn ? 'ON' : 'OFF' }}
            </v-chip>
          </v-card-title>

          <v-card-text class="text-center">
            <div class="device-stage">
              <div class="ac-unit">
                <v-icon size="80" :class="{ 'fan-spin': acOn }">mdi-air-conditioner</v-icon>
              </div>

              <div class="air-stream" :class="{ active: acOn }">
                <span></span><span></span><span></span>
              </div>
            </div>

            <div class="text-body-1 mt-4">
              Temperature: <strong>{{ latest?.temperature ?? '--' }} °C</strong>
            </div>
            <div class="text-caption text-medium-emphasis">
              AC turns on when temperature ≥ 25°C
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- Sprinkler -->
      <v-col cols="12" md="4">
        <v-card rounded="xl" elevation="3" class="fill-height">
          <v-card-title class="d-flex justify-space-between align-center">
            <span>Sprinklers</span>
            <v-chip :color="sprinklerOn ? 'green' : 'grey'" variant="flat">
              {{ sprinklerOn ? 'ON' : 'OFF' }}
            </v-chip>
          </v-card-title>

          <v-card-text class="text-center">
            <div class="device-stage">
              <div class="sprinkler-head">
                <v-icon size="72">mdi-sprinkler-variant</v-icon>
              </div>

              <div class="water-spray" :class="{ active: sprinklerOn }">
                <span></span><span></span><span></span><span></span><span></span>
              </div>
            </div>

            <div class="text-body-1 mt-4">
              Soil Moisture: <strong>{{ latest?.soil_moisture ?? '--' }} %</strong>
            </div>
            <div class="text-caption text-medium-emphasis">
              Sprinklers turn on when soil moisture ≤ 30%
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- Dehumidifier -->
      <v-col cols="12" md="4">
        <v-card rounded="xl" elevation="3" class="fill-height">
          <v-card-title class="d-flex justify-space-between align-center">
            <span>Dehumidifier</span>
            <v-chip :color="dehumidifierOn ? 'green' : 'grey'" variant="flat">
              {{ dehumidifierOn ? 'ON' : 'OFF' }}
            </v-chip>
          </v-card-title>

          <v-card-text class="text-center">
            <div class="device-stage">
              <div class="dehumidifier-box">
                <v-icon size="76">mdi-air-humidifier-off</v-icon>
              </div>

              <div class="drop-capture" :class="{ active: dehumidifierOn }">
                <span></span><span></span><span></span>
              </div>
            </div>

            <div class="text-body-1 mt-4">
              Humidity: <strong>{{ latest?.humidity ?? '--' }} %</strong>
            </div>
            <div class="text-caption text-medium-emphasis">
              Dehumidifier turns on when humidity ≥ 70%
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
                  :model-value="acOn ? 100 : 0"
                  height="20"
                  rounded
                  color="blue"
                >
                  <strong>AC {{ acOn ? 'ACTIVE' : 'IDLE' }}</strong>
                </v-progress-linear>
              </v-col>

              <v-col cols="12" md="4">
                <v-progress-linear
                  :model-value="sprinklerOn ? 100 : 0"
                  height="20"
                  rounded
                  color="green"
                >
                  <strong>SPRINKLER {{ sprinklerOn ? 'ACTIVE' : 'IDLE' }}</strong>
                </v-progress-linear>
              </v-col>

              <v-col cols="12" md="4">
                <v-progress-linear
                  :model-value="dehumidifierOn ? 100 : 0"
                  height="20"
                  rounded
                  color="deep-purple"
                >
                  <strong>DEHUMIDIFIER {{ dehumidifierOn ? 'ACTIVE' : 'IDLE' }}</strong>
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
import { useWeatherApi } from '../composables/useWeatherApi'

const {
  latest,
  error,
  acOn,
  sprinklerOn,
  dehumidifierOn,
} = useWeatherApi()
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
  border-radius: 50% 50% 50% 50%;
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