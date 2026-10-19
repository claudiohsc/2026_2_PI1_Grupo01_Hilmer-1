import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Configuração do Vite: servidor de desenvolvimento em http://localhost:5173 e build em dist/.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
  },
});
