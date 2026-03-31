import 'vuetify/styles'
import { createVuetify } from 'vuetify'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'
import '@mdi/font/css/materialdesignicons.css'

export default createVuetify({
  components,
  directives,
  theme: {
    defaultTheme: 'light',
    themes: {
      light: {
        colors: {
          primary: '#1e3a5f',
          secondary: '#1976d2',
          accent: '#43a047',
          background: '#f4f6f8',
          surface: '#ffffff'
        }
      }
    }
  }
})