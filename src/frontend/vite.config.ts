import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Configuração do Vite: servidor de desenvolvimento em http://localhost:5173 e build em dist/.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
  },
  build: {
    rollupOptions: {
      output: {
        // Separa as bibliotecas grandes em arquivos próprios: o navegador guarda cada uma
        // em cache e o build não passa do limite de 500 kB por arquivo.
        manualChunks: {
          react: ['react', 'react-dom'],
          recharts: ['recharts'],
          supabase: ['@supabase/supabase-js'],
        },
      },
    },
  },
});
