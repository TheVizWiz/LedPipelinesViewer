import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// The C++ backend (viewer::startServer) serves the two SSE streams on 127.0.0.1:8420 and nothing else. In dev, Vite
// serves the UI on :5173 and proxies the stream paths to the backend, so the frontend can use relative URLs
// (/stream, /pipeline) with no CORS handling on either side. `open: true` launches the browser on `npm run dev`.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    open: true,
    proxy: {
      '/stream': { target: 'http://127.0.0.1:8420', changeOrigin: true },
      '/pipeline': { target: 'http://127.0.0.1:8420', changeOrigin: true },
    },
  },
});
