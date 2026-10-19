import type { MapaCelula, Orientacao, Posicao } from '../tipos';

/** Deslocamento de cada orientação. Norte aumenta a linha; Leste aumenta a coluna. */
export const DESLOCAMENTO: Record<Orientacao, Posicao> = {
  N: { linha: 1, coluna: 0 },
  S: { linha: -1, coluna: 0 },
  L: { linha: 0, coluna: 1 },
  O: { linha: 0, coluna: -1 },
};

const CAMPO_PAREDE = {
  N: 'parede_norte',
  S: 'parede_sul',
  L: 'parede_leste',
  O: 'parede_oeste',
} as const satisfies Record<Orientacao, keyof MapaCelula>;

/** Valor da parede da célula na orientação dada (`true`, `false` ou `null`). */
export function parede(celula: MapaCelula, orientacao: Orientacao): boolean | null {
  return celula[CAMPO_PAREDE[orientacao]];
}

/**
 * Lê um labirinto no mesmo formato ASCII do firmware (`LabirintoSimulado.h`):
 * cada célula ocupa 4 colunas e 2 linhas de texto, `+` marca os cantos, `---` é parede
 * horizontal e `|` é parede vertical. A primeira linha do texto é a borda Norte e a célula
 * (0, 0) fica no canto inferior esquerdo.
 *
 * @returns Uma célula por posição, com todas as paredes conhecidas (`true`/`false`).
 */
export function lerLabirintoAscii(texto: string): { linhas: number; colunas: number; celulas: MapaCelula[] } {
  const linhasTexto = texto.replace(/\r/g, '').replace(/\n+$/, '').split('\n');
  const linhas = (linhasTexto.length - 1) / 2;
  const colunas = (linhasTexto[0].length - 1) / 4;
  if (!Number.isInteger(linhas) || !Number.isInteger(colunas) || linhas < 1 || colunas < 1) {
    throw new Error('Labirinto ASCII com tamanho inválido');
  }

  const celulas: MapaCelula[] = [];
  for (let linha = 0; linha < linhas; linha++) {
    // A linha mais ao Norte é a primeira do texto.
    const topo = 2 * (linhas - 1 - linha);
    for (let coluna = 0; coluna < colunas; coluna++) {
      const x = 4 * coluna;
      celulas.push({
        linha,
        coluna,
        parede_norte: linhasTexto[topo].slice(x + 1, x + 4) === '---',
        parede_sul: linhasTexto[topo + 2].slice(x + 1, x + 4) === '---',
        parede_oeste: linhasTexto[topo + 1][x] === '|',
        parede_leste: linhasTexto[topo + 1][x + 4] === '|',
      });
    }
  }
  return { linhas, colunas, celulas };
}

/** Busca a célula (linha, coluna) numa lista. */
export function buscarCelula(celulas: MapaCelula[], posicao: Posicao): MapaCelula | undefined {
  return celulas.find((c) => c.linha === posicao.linha && c.coluna === posicao.coluna);
}

/** Orientação do passo entre duas células vizinhas. */
export function orientacaoDoPasso(de: Posicao, para: Posicao): Orientacao {
  if (para.linha > de.linha) return 'N';
  if (para.linha < de.linha) return 'S';
  if (para.coluna > de.coluna) return 'L';
  return 'O';
}

/**
 * Percurso de exploração usado nos dados de exemplo: busca em profundidade a partir da
 * largada até o objetivo, voltando pelos becos sem saída, como um robô explorando.
 * Não é o algoritmo do firmware (flood fill, #183); serve só para desenhar um trajeto
 * plausível e coerente com as paredes.
 *
 * @returns Sequência de células visitadas, incluindo os retornos.
 */
export function explorarAteObjetivo(
  celulas: MapaCelula[],
  linhas: number,
  colunas: number,
  ordem: Orientacao[] = ['N', 'L', 'S', 'O'],
): Posicao[] {
  const objetivo: Posicao = { linha: linhas - 1, coluna: colunas - 1 };
  const chave = (p: Posicao) => `${p.linha},${p.coluna}`;
  const visitadas = new Set<string>();
  const percurso: Posicao[] = [];

  const visitar = (atual: Posicao): boolean => {
    visitadas.add(chave(atual));
    percurso.push(atual);
    if (atual.linha === objetivo.linha && atual.coluna === objetivo.coluna) return true;

    const celula = buscarCelula(celulas, atual);
    for (const orientacao of ordem) {
      if (!celula || parede(celula, orientacao) !== false) continue;
      const passo = DESLOCAMENTO[orientacao];
      const vizinha = { linha: atual.linha + passo.linha, coluna: atual.coluna + passo.coluna };
      const dentro =
        vizinha.linha >= 0 && vizinha.linha < linhas && vizinha.coluna >= 0 && vizinha.coluna < colunas;
      if (!dentro || visitadas.has(chave(vizinha))) continue;
      if (visitar(vizinha)) return true;
      percurso.push(atual); // volta do beco sem saída
    }
    return false;
  };

  visitar({ linha: 0, coluna: 0 });
  return percurso;
}

/**
 * Mapa como o robô o conhece depois de passar pelas células visitadas: as paredes dessas
 * células ficam conhecidas e as demais continuam `null` (não exploradas).
 */
export function mapaConhecido(celulas: MapaCelula[], visitadas: Posicao[]): MapaCelula[] {
  const conhecidas = new Set(visitadas.map((p) => `${p.linha},${p.coluna}`));
  return celulas.map((c) =>
    conhecidas.has(`${c.linha},${c.coluna}`)
      ? c
      : { ...c, parede_norte: null, parede_sul: null, parede_leste: null, parede_oeste: null },
  );
}
