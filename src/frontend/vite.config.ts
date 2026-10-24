/// <reference types="vitest/config" />
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Configuração do Vite e Vitest: servidor de desenvolvimento em http://localhost:5173, build em dist/ e testes com Vitest/JSDOM.
// O Recharts, a maior dependência, vai num arquivo à parte porque é importado sob demanda
// (veja `GraficoVelocidade` em `paginas/TelaAoVivo.tsx`).
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
  },
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: ['./src/test/setup.ts'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      include: ['src/**/*.{ts,tsx}'],
      exclude: [
        'src/**/*.test.{ts,tsx}',
        'src/test/**',
        'src/vite-env.d.ts',
        'src/main.tsx',
      ],
    },
  },
});
