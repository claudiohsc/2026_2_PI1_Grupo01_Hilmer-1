import { describe, expect, it } from 'vitest';
import {
  DESLOCAMENTO,
  buscarCelula,
  explorarAteObjetivo,
  lerLabirintoAscii,
  mapaConhecido,
  orientacaoDoPasso,
  parede,
} from './labirinto';
import type { MapaCelula } from '../tipos';

describe('labirinto', () => {
  const labirintoAsciiExemplo = [
    '+---+---+',
    '|       |',
    '+   +---+',
    '|       |',
    '+---+---+',
  ].join('\n');

  it('DESLOCAMENTO mapeia os eixos corretos', () => {
    expect(DESLOCAMENTO.N).toEqual({ linha: 1, coluna: 0 });
    expect(DESLOCAMENTO.S).toEqual({ linha: -1, coluna: 0 });
    expect(DESLOCAMENTO.L).toEqual({ linha: 0, coluna: 1 });
    expect(DESLOCAMENTO.O).toEqual({ linha: 0, coluna: -1 });
  });

  describe('lerLabirintoAscii', () => {
    it('faz o parsing de matriz ASCII 2x2 corretamente', () => {
      const { linhas, colunas, celulas } = lerLabirintoAscii(labirintoAsciiExemplo);

      expect(linhas).toBe(2);
      expect(colunas).toBe(2);
      expect(celulas).toHaveLength(4);

      // Célula (0, 0) no canto inferior esquerdo
      const c00 = buscarCelula(celulas, { linha: 0, coluna: 0 });
      expect(c00).toBeDefined();
      expect(c00?.parede_sul).toBe(true);
      expect(c00?.parede_oeste).toBe(true);
      expect(c00?.parede_norte).toBe(false); // sem parede interna no topo de (0,0)

      // Célula (1, 1) no canto superior direito
      const c11 = buscarCelula(celulas, { linha: 1, coluna: 1 });
      expect(c11).toBeDefined();
      expect(c11?.parede_norte).toBe(true);
      expect(c11?.parede_leste).toBe(true);
      expect(c11?.parede_sul).toBe(true);
    });

    it('lança erro para texto com dimensões inválidas', () => {
      expect(() => lerLabirintoAscii('invalido')).toThrow('Labirinto ASCII com tamanho inválido');
      expect(() => lerLabirintoAscii('+---+\n|')).toThrow('Labirinto ASCII com tamanho inválido');
    });
  });

  describe('parede', () => {
    it('retorna a presença da parede na orientação indicada', () => {
      const celula: MapaCelula = {
        linha: 0,
        coluna: 0,
        parede_norte: true,
        parede_sul: false,
        parede_leste: null,
        parede_oeste: true,
      };

      expect(parede(celula, 'N')).toBe(true);
      expect(parede(celula, 'S')).toBe(false);
      expect(parede(celula, 'L')).toBeNull();
      expect(parede(celula, 'O')).toBe(true);
    });
  });

  describe('buscarCelula', () => {
    it('retorna a célula correspondente ou undefined se não existir', () => {
      const celulas: MapaCelula[] = [
        { linha: 0, coluna: 0, parede_norte: false, parede_sul: true, parede_leste: false, parede_oeste: true },
      ];

      expect(buscarCelula(celulas, { linha: 0, coluna: 0 })).toEqual(celulas[0]);
      expect(buscarCelula(celulas, { linha: 1, coluna: 1 })).toBeUndefined();
    });
  });

  describe('orientacaoDoPasso', () => {
    it('determina a orientação correta entre células vizinhas', () => {
      expect(orientacaoDoPasso({ linha: 0, coluna: 0 }, { linha: 1, coluna: 0 })).toBe('N');
      expect(orientacaoDoPasso({ linha: 1, coluna: 0 }, { linha: 0, coluna: 0 })).toBe('S');
      expect(orientacaoDoPasso({ linha: 0, coluna: 0 }, { linha: 0, coluna: 1 })).toBe('L');
      expect(orientacaoDoPasso({ linha: 0, coluna: 1 }, { linha: 0, coluna: 0 })).toBe('O');
    });
  });

  describe('explorarAteObjetivo', () => {
    it('gera percurso da largada até o objetivo e volta de becos sem saída', () => {
      const { linhas, colunas, celulas } = lerLabirintoAscii(labirintoAsciiExemplo);
      const percurso = explorarAteObjetivo(celulas, linhas, colunas);

      expect(percurso.length).toBeGreaterThan(0);
      expect(percurso[0]).toEqual({ linha: 0, coluna: 0 });
      expect(percurso[percurso.length - 1]).toEqual({ linha: linhas - 1, coluna: colunas - 1 });
    });

    it('retrocede no percurso ao encontrar beco sem saída', () => {
      // Labirinto com um beco sem saída à direita de (0,0):
      // (0,0) pode ir para (0,1) mas (0,1) é beco sem saída cercado.
      // E de (0,0) vai para (1,0) e depois para (1,1).
      const labirintoComBeco = [
        '+---+---+',
        '|       |',
        '+   +---+',
        '|       |',
        '+---+---+',
      ].join('\n');
      const { linhas, colunas, celulas } = lerLabirintoAscii(labirintoComBeco);
      // Força a exploração a tentar 'L' (coluna 1) antes de 'N' (linha 1)
      const percurso = explorarAteObjetivo(celulas, linhas, colunas, ['L', 'N', 'S', 'O']);
      expect(percurso.length).toBeGreaterThan(3);
      // Deve ter visitado (0,1), voltado para (0,0), e então chegado a (1,1)
      expect(percurso.some((p) => p.linha === 0 && p.coluna === 1)).toBe(true);
      expect(percurso[percurso.length - 1]).toEqual({ linha: 1, coluna: 1 });
    });
  });

  describe('mapaConhecido', () => {
    it('mantém paredes apenas nas células visitadas e define null nas não exploradas', () => {
      const { celulas } = lerLabirintoAscii(labirintoAsciiExemplo);
      const visitadas = [{ linha: 0, coluna: 0 }];

      const conhecido = mapaConhecido(celulas, visitadas);
      const c00 = buscarCelula(conhecido, { linha: 0, coluna: 0 });
      const c11 = buscarCelula(conhecido, { linha: 1, coluna: 1 });

      expect(c00?.parede_sul).toBe(true);
      expect(c00?.parede_oeste).toBe(true);

      expect(c11?.parede_norte).toBeNull();
      expect(c11?.parede_sul).toBeNull();
      expect(c11?.parede_leste).toBeNull();
      expect(c11?.parede_oeste).toBeNull();
    });
  });
});
