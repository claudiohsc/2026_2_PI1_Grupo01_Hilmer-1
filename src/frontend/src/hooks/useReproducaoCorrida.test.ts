import { act, renderHook } from '@testing-library/react';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { useReproducaoCorrida, VELOCIDADE_REPRODUCAO } from './useReproducaoCorrida';
import type { Telemetria } from '../tipos';

describe('useReproducaoCorrida', () => {
  const telemetriaMock: Telemetria[] = [
    {
      id: 1,
      corrida_id: 1,
      numero_sequencia: 1,
      timestamp: '2026-10-23T10:00:00Z',
      posicao_linha: 0,
      posicao_coluna: 0,
      orientacao: 'N',
      velocidade: 0.1,
      tensao_bateria: 8.0,
      corrente_bateria: 0.2,
      tempo_decorrido: 0.0,
      estado_corrida: 'explorando',
      evento: null,
    },
    {
      id: 2,
      corrida_id: 1,
      numero_sequencia: 2,
      timestamp: '2026-10-23T10:00:01Z',
      posicao_linha: 1,
      posicao_coluna: 0,
      orientacao: 'N',
      velocidade: 0.2,
      tensao_bateria: 7.9,
      corrente_bateria: 0.25,
      tempo_decorrido: 1.0,
      estado_corrida: 'explorando',
      evento: null,
    },
    {
      id: 3,
      corrida_id: 1,
      numero_sequencia: 3,
      timestamp: '2026-10-23T10:00:02Z',
      posicao_linha: 1,
      posicao_coluna: 1,
      orientacao: 'L',
      velocidade: 0.25,
      tensao_bateria: 7.8,
      corrente_bateria: 0.3,
      tempo_decorrido: 2.0,
      estado_corrida: 'concluido',
      evento: 'objetivo_alcancado',
    },
  ];

  beforeEach(() => {
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it('inicia no estado aguardando com tempo zero e sem leituras expostas', () => {
    const { result } = renderHook(() => useReproducaoCorrida(telemetriaMock));

    expect(result.current.estado).toBe('aguardando');
    expect(result.current.tempo).toBe(0);
    expect(result.current.leituras).toEqual([]);
  });

  it('inicia a reprodução ao chamar iniciar()', () => {
    const { result } = renderHook(() => useReproducaoCorrida(telemetriaMock));

    act(() => {
      result.current.iniciar();
    });

    expect(result.current.estado).toBe('em_execucao');
  });

  it('avança o tempo acelerado e entrega as leituras conforme o tempo decorrido', () => {
    const { result } = renderHook(() => useReproducaoCorrida(telemetriaMock));

    act(() => {
      result.current.iniciar();
    });

    // 1.0s simulado = (1.0 / VELOCIDADE_REPRODUCAO) * 1000 = 250ms reais
    act(() => {
      vi.advanceTimersByTime((1.0 / VELOCIDADE_REPRODUCAO) * 1000);
    });

    expect(result.current.tempo).toBeCloseTo(1.0, 1);
    expect(result.current.leituras).toHaveLength(2);
    expect(result.current.leituras.map((l) => l.numero_sequencia)).toEqual([1, 2]);
  });

  it('conclui a corrida ao atingir o evento objetivo_alcancado', () => {
    const { result } = renderHook(() => useReproducaoCorrida(telemetriaMock));

    act(() => {
      result.current.iniciar();
    });

    // 600ms reais a 4x = 2.4s, ultrapassando os 2.0s da chegada
    act(() => {
      vi.advanceTimersByTime(600);
    });

    expect(result.current.estado).toBe('concluido');
    expect(result.current.tempo).toBe(2.0);
    expect(result.current.leituras).toHaveLength(3);
  });

  it('transita para nao_concluido ao atingir o limite de 600s sem objetivo alcancado', () => {
    const telemetriaInfinita: Telemetria[] = [
      {
        ...telemetriaMock[0],
        evento: null,
      },
    ];

    const { result } = renderHook(() => useReproducaoCorrida(telemetriaInfinita));

    act(() => {
      result.current.iniciar();
    });

    // 600s / 4 = 150.000 ms reais
    act(() => {
      vi.advanceTimersByTime(151000);
    });

    expect(result.current.estado).toBe('nao_concluido');
    expect(result.current.tempo).toBe(600);
  });

  it('reiniciar() restaura o estado inicial para aguardando com tempo zero', () => {
    const { result } = renderHook(() => useReproducaoCorrida(telemetriaMock));

    act(() => {
      result.current.iniciar();
    });

    act(() => {
      vi.advanceTimersByTime(250);
    });

    expect(result.current.tempo).toBeGreaterThan(0);

    act(() => {
      result.current.reiniciar();
    });

    expect(result.current.estado).toBe('aguardando');
    expect(result.current.tempo).toBe(0);
    expect(result.current.leituras).toEqual([]);
  });
});
