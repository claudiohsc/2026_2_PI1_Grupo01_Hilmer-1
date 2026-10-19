import type { Labirinto, TipoLabirinto } from '../tipos';

/**
 * Labirintos de exemplo, um de cada tamanho, copiados de
 * `src/firmware/test/test_labirinto_simulado/LabirintosExemplo.h` para que o dashboard
 * e os testes do firmware usem os mesmos mapas. Formato descrito em `lerLabirintoAscii`.
 */
export const LABIRINTOS_ASCII: Record<TipoLabirinto, string> = {
  '4x4': `+---+---+---+---+
|       |       |
+   +---+   +   +
|   |       |   |
+   +   +---+   +
|       |       |
+   +---+   +---+
|   |           |
+---+---+---+---+`,
  '8x4': `+---+---+---+---+---+---+---+---+
|               |               |
+   +---+---+   +   +---+---+   +
|   |       |       |       |   |
+   +   +   +---+---+   +   +   +
|   |   |                   |   |
+   +   +---+---+---+---+   +   +
|       |                       |
+---+---+---+---+---+---+---+---+`,
  '12x4': `+---+---+---+---+---+---+---+---+---+---+---+---+
|                       |                       |
+   +---+---+---+---+   +   +---+---+---+---+   +
|   |               |       |               |   |
+   +   +---+---+   +---+---+   +---+---+   +   +
|   |   |                           |   |   |   |
+   +   +   +---+---+---+---+---+   +   +   +   +
|       |                       |       |       |
+---+---+---+---+---+---+---+---+---+---+---+---+`,
};

/** Mesmo conteúdo do *seed* da tabela `labirintos` (#228). */
export const LABIRINTOS_EXEMPLO: Labirinto[] = [
  { id: 1, tipo: '4x4', linhas: 4, colunas: 4 },
  { id: 2, tipo: '8x4', linhas: 4, colunas: 8 },
  { id: 3, tipo: '12x4', linhas: 4, colunas: 12 },
];
