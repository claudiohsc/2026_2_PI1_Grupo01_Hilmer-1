import { describe, expect, it } from 'vitest';
import {
  descreverTentativa,
  formatarDataHora,
  formatarMinutos,
  formatarTempo,
  formatarVelocidade,
  nomeLabirinto,
  nomeLabirintoHistorico,
  notaDaTentativa,
  ordinal,
  percentualBateria,
  LIMITE_TEMPO_S,
  MAXIMO_TENTATIVAS,
  TENSAO_CHEIA_V,
  TENSAO_CORTE_V,
} from './formatacao';

describe('formatacao', () => {
  it('constantes possuem os valores esperados pelas regras de negócio', () => {
    expect(LIMITE_TEMPO_S).toBe(600);
    expect(MAXIMO_TENTATIVAS).toBe(3);
    expect(TENSAO_CHEIA_V).toBe(8.4);
    expect(TENSAO_CORTE_V).toBe(6.6);
  });

  describe('nomeLabirinto', () => {
    it('formata o rótulo do seletor do cabeçalho para cada dimensão', () => {
      expect(nomeLabirinto('4x4')).toBe('Labirinto 1 · 4x4');
      expect(nomeLabirinto('8x4')).toBe('Labirinto 2 · 8x4');
      expect(nomeLabirinto('12x4')).toBe('Labirinto 3 · 12x4');
    });
  });

  describe('nomeLabirintoHistorico', () => {
    it('formata o rótulo do filtro e da tabela do histórico com o símbolo ×', () => {
      expect(nomeLabirintoHistorico('4x4')).toBe('Labirinto 1 (4×4)');
      expect(nomeLabirintoHistorico('8x4')).toBe('Labirinto 2 (8×4)');
      expect(nomeLabirintoHistorico('12x4')).toBe('Labirinto 3 (12×4)');
    });
  });

  describe('formatarTempo', () => {
    it('formata segundos com milissegundos no padrão mm:ss.mmm', () => {
      expect(formatarTempo(0)).toBe('00:00.000');
      expect(formatarTempo(68.45)).toBe('01:08.450');
      expect(formatarTempo(600)).toBe('10:00.000');
      expect(formatarTempo(5.008)).toBe('00:05.008');
    });

    it('trata valores negativos retornando zero', () => {
      expect(formatarTempo(-10)).toBe('00:00.000');
    });
  });

  describe('formatarMinutos', () => {
    it('formata segundos inteiros em m:ss', () => {
      expect(formatarMinutos(0)).toBe('0:00');
      expect(formatarMinutos(65)).toBe('1:05');
      expect(formatarMinutos(600)).toBe('10:00');
    });
  });

  describe('formatarVelocidade', () => {
    it('formata velocidade em metros por segundo com 2 casas decimais', () => {
      expect(formatarVelocidade(0)).toBe('0.00 m/s');
      expect(formatarVelocidade(0.18)).toBe('0.18 m/s');
      expect(formatarVelocidade(1.2345)).toBe('1.23 m/s');
    });
  });

  describe('percentualBateria', () => {
    it('calcula a faixa linear de bateria 2S entre 6.6V (0%) e 8.4V (100%)', () => {
      expect(percentualBateria(8.4)).toBe(100);
      expect(percentualBateria(6.6)).toBe(0);
      expect(percentualBateria(7.5)).toBe(50);
    });

    it('limita valores fora da faixa entre 0% e 100%', () => {
      expect(percentualBateria(9.0)).toBe(100);
      expect(percentualBateria(5.0)).toBe(0);
    });
  });

  describe('notaDaTentativa', () => {
    it('atribui notas decrescentes conforme o número da tentativa bem-sucedida', () => {
      expect(notaDaTentativa(1)).toBe(10);
      expect(notaDaTentativa(2)).toBe(7.5);
      expect(notaDaTentativa(3)).toBe(5);
      expect(notaDaTentativa(4)).toBe(0);
    });
  });

  describe('ordinal', () => {
    it('formata números ordinais femininos', () => {
      expect(ordinal(1)).toBe('1ª');
      expect(ordinal(2)).toBe('2ª');
      expect(ordinal(3)).toBe('3ª');
    });
  });

  describe('formatarDataHora', () => {
    it('converte string ISO 8601 em formato brasileiro DD/MM/AAAA HH:mm', () => {
      const dataIso = new Date(2026, 8, 27, 14, 32).toISOString();
      const resultado = formatarDataHora(dataIso);
      expect(resultado).toMatch(/\d{2}\/\d{2}\/\d{4} \d{2}:\d{2}/);
      expect(resultado).not.toContain(',');
    });
  });

  describe('descreverTentativa', () => {
    it('descreve tentativa concluída com a respectiva nota', () => {
      expect(descreverTentativa(1, 'concluida')).toBe('1ª Tentativa (Nota 10)');
      expect(descreverTentativa(2, 'concluida')).toBe('2ª Tentativa (Nota 7.5)');
      expect(descreverTentativa(3, 'concluida')).toBe('3ª Tentativa (Nota 5)');
    });

    it('descreve tentativa não concluída', () => {
      expect(descreverTentativa(2, 'nao_concluida')).toBe('2ª Tentativa (Não concluída)');
    });

    it('descreve tentativa em andamento', () => {
      expect(descreverTentativa(3, 'em_andamento')).toBe('3ª Tentativa (Em andamento)');
    });
  });
});

