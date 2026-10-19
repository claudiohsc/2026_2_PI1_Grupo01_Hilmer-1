import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Configuração do Vite: servidor de desenvolvimento em http://localhost:5173 e build em dist/.
// O Recharts, a maior dependência, vai num arquivo à parte porque é importado sob demanda
// (veja `GraficoVelocidade` em `paginas/TelaAoVivo.tsx`).
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
  },
});
