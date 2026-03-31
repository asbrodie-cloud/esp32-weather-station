import { createRouter, createWebHistory } from 'vue-router'
import WeatherDashboardView from '../views/WeatherDashboardView.vue'
import AutomationView from '../views/AutomationView.vue'

const routes = [
  {
    path: '/',
    name: 'dashboard',
    component: WeatherDashboardView,
  },
  {
    path: '/automation',
    name: 'automation',
    component: AutomationView,
  },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router