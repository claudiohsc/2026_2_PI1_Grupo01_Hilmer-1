import { renderHook, waitFor } from '@testing-library/react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { useCorridas } from './useCorridas';
import { CORRIDAS_EXEMPLO } from '../dados/historicoExemplo';
import { criarClienteSupabaseSimulado, type ClienteSupabaseSimulado } from '../test/supabaseSimulado';
import { registrarConsultas } from '../test/registroConsultas';
import type { Corrida } from '../tipos';

/*
 * Integração do hook com o Supabase simulado em memória. As corridas abaixo são inventadas,
 * só com o formato real da tabela `corridas`.
 */
const clienteSimulado = criarClienteSupabaseSimulado();
let clienteAtual: ClienteSupabaseSimulado | null = clienteSimulado;

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteAtual;
  },
}));

function corrida(id: number, labirintoId: number, inicio: string): Corrida {
  return {
    id,
    labirinto_id: labirintoId,
    numero_tentativa: 1,
    data_hora_inicio: inicio,
    data_hora_fim: null,
    tempo_final: 60,
    velocidade_media: 0.3,
    velocidade_pico: 0.5,
    consumo_bateria: 10,
    status: 'concluida',
  };
}

const CORRIDAS = [
  corrida(1, 1, '2026-10-20T10:00:00Z'),
  corrida(2, 2, '2026-10-21T10:00:00Z'),
  corrida(3, 1, '2026-10-22T10:00:00Z'),
  corrida(4, 3, '2026-10-23T10:00:00Z'),
];

const ids = (lista: Corrida[]) => lista.map((c) => c.id);

describe('useCorridas com filtro por labirinto (integração com Supabase simulado)', () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    clienteSimulado.limpar();
    clienteSimulado.definirTabela('corridas', CORRIDAS);
    clienteAtual = clienteSimulado;
  });

  it('sem filtro, consulta o banco sem labirinto_id e traz todas, da mais recente para a mais antiga', async () => {
    const consultas = registrarConsultas(clienteSimulado);
    const { result } = renderHook(() => useCorridas(null));

    await waitFor(() => expect(result.current.carregando).toBe(false));
    expect(consultas).toEqual([{ tabela: 'corridas', filtros: {} }]);
    expect(ids(result.current.corridas)).toEqual([4, 3, 2, 1]);
  });

  it.each([
    [1, [3, 1]],
    [2, [2]],
    [3, [4]],
  ])('com o labirinto %i, o filtro vai na consulta ao banco e só voltam as corridas dele', async (labirintoId, esperadas) => {
    const consultas = registrarConsultas(clienteSimulado);
    const { result } = renderHook(() => useCorridas(labirintoId));

    await waitFor(() => expect(result.current.carregando).toBe(false));
    expect(consultas).toEqual([{ tabela: 'corridas', filtros: { labirinto_id: labirintoId } }]);
    expect(ids(result.current.corridas)).toEqual(esperadas);
  });

  it('trocar o labirinto refaz a consulta com o filtro novo', async () => {
    const consultas = registrarConsultas(clienteSimulado);
    const { result, rerender } = renderHook(({ id }) => useCorridas(id), {
      initialProps: { id: null as number | null },
    });
    await waitFor(() => expect(result.current.corridas).toHaveLength(4));

    rerender({ id: 2 });
    await waitFor(() => expect(ids(result.current.corridas)).toEqual([2]));
    rerender({ id: null });
    await waitFor(() => expect(result.current.corridas).toHaveLength(4));

    expect(consultas.map((c) => c.filtros)).toEqual([{}, { labirinto_id: 2 }, {}]);
  });

  it('uma resposta atrasada do filtro anterior não sobrescreve a do filtro atual', async () => {
    // A consulta do labirinto 1 demora; a do labirinto 3, escolhida logo depois, responde na hora.
    registrarConsultas(clienteSimulado, (c) => (c.filtros.labirinto_id === 1 ? 50 : 0));
    const { result, rerender } = renderHook(({ id }) => useCorridas(id), { initialProps: { id: 1 } });

    rerender({ id: 3 });
    await waitFor(() => expect(ids(result.current.corridas)).toEqual([4]));
    await new Promise((resolver) => setTimeout(resolver, 80)); // a resposta do labirinto 1 chega agora

    expect(ids(result.current.corridas)).toEqual([4]);
    expect(result.current.carregando).toBe(false);
  });

  it('com erro no banco, avisa e não troca a lista', async () => {
    clienteSimulado.definirErro('corridas', 'falha simulada');
    vi.spyOn(console, 'warn').mockImplementation(() => {});
    const { result } = renderHook(() => useCorridas(1));

    await waitFor(() => expect(result.current.erro).toBe('Não foi possível carregar as corridas do Supabase.'));
    expect(result.current.corridas).toEqual([]);
  });

  it('sem Supabase configurado, filtra as corridas de exemplo', async () => {
    clienteAtual = null;
    const { result, rerender } = renderHook(({ id }) => useCorridas(id), {
      initialProps: { id: 3 as number | null },
    });

    expect(result.current.carregando).toBe(false);
    expect(result.current.corridas.every((c) => c.labirinto_id === 3)).toBe(true);
    expect(result.current.corridas).toHaveLength(CORRIDAS_EXEMPLO.filter((c) => c.labirinto_id === 3).length);

    rerender({ id: null });
    await waitFor(() => expect(result.current.corridas).toEqual(CORRIDAS_EXEMPLO));
  });
});
