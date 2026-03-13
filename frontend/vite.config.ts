import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

const serverHost = process.env.VITE_SERVER_HOST ?? 'localhost'

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/ingest': `http://${serverHost}:8080`,
      '/stream': {
        target: `http://${serverHost}:8080`,
        changeOrigin: true,
      },
      '/health': `http://${serverHost}:8080`
    }
  }
})
