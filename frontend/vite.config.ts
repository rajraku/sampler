import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

const serverHost = process.env.VITE_SERVER_HOST ?? 'localhost';
const serverPort = process.env.VITE_SERVER_PORT ?? '8080';
const serverOrigin = `http://${serverHost}:${serverPort}`;

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/ingest': serverOrigin,
      '/stream': {
        target: serverOrigin,
        changeOrigin: true,
      },
      '/health': serverOrigin,
    }
  }
})
