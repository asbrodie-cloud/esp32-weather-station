import { ref, computed, onMounted, onUnmounted } from 'vue'

export function useWeatherApi() {
  const latest = ref(null)
  const readings = ref([])
  const loading = ref(false)
  const error = ref('')
  let timer = null

  const tempSeries = computed(() => readings.value.map(r => Number(r.temperature || 0)).reverse())
  const humiditySeries = computed(() => readings.value.map(r => Number(r.humidity || 0)).reverse())
  const pressureSeries = computed(() => readings.value.map(r => Number(r.pressure || 0)).reverse())
  const soilSeries = computed(() => readings.value.map(r => Number(r.soil_moisture || 0)).reverse())

  const acOn = computed(() => Number(latest.value?.temperature || 0) >= 25)
  const sprinklerOn = computed(() => Number(latest.value?.soil_moisture || 0) <= 30)
  const dehumidifierOn = computed(() => Number(latest.value?.humidity || 0) >= 70)

  async function fetchLatest() {
    const res = await fetch('/api/latest')
    if (!res.ok) throw new Error('Failed to fetch latest reading')
    latest.value = await res.json()
  }

  async function fetchReadings() {
    const res = await fetch('/api/readings')
    if (!res.ok) throw new Error('Failed to fetch readings')
    readings.value = await res.json()
  }

  async function refresh() {
    loading.value = true
    error.value = ''
    try {
      await Promise.all([fetchLatest(), fetchReadings()])
    } catch (err) {
      error.value = err.message || 'Unable to load weather data'
    } finally {
      loading.value = false
    }
  }

  onMounted(() => {
    refresh()
    timer = setInterval(refresh, 2000)
  })

  onUnmounted(() => {
    if (timer) clearInterval(timer)
  })

  return {
    latest,
    readings,
    loading,
    error,
    refresh,
    tempSeries,
    humiditySeries,
    pressureSeries,
    soilSeries,
    acOn,
    sprinklerOn,
    dehumidifierOn,
  }
}