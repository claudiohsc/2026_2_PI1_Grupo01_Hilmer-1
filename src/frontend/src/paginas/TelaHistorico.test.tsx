import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { TelaHistorico } from './TelaHistorico';
import { criarClienteSupabaseSimulado } from '../test/supabaseSimulado';
import type { Corrida, Labirinto } from '../tipos';

const clienteSimulado = criarClienteSupabaseSimulado();

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteSimulado;
  },
}));

describe('TelaHistorico (Integração com Supabase Simulado)', () => {
  const labirintos: Labirinto[] = [
    { id: 1, tipo: '4x4', linhas: 4, colunas: 4 },
    { id: 2, tipo: '8x4', linhas: 8, colunas: 4 },
    { id: 3, tipo: '12x4', linhas: 12, colunas: 4 },
  ];

  const corridasSimuladas: Corrida[] = [
    {
      id: 10,
      labirinto_id: 1,
      numero_tentativa: 1,
      data_hora_inicio: '2026-10-22T14:00:00Z',
      data_hora_fim: '2026-10-22T14:00:45Z',
      tempo_final: 45.0,
      velocidade_media: 0.35,
      velocidade_pico: 0.6,
      consumo_bateria: 12,
      status: 'concluida',
    },
    {
      id: 11,
      labirinto_id: 2,
      numero_tentativa: 2,
      data_hora_inicio: '2026-10-22T15:30:00Z',
      data_hora_fim: '2026-10-22T15:31:30Z',
      tempo_final: 90.0,
      velocidade_media: 0.28,
      velocidade_pico: 0.55,
      consumo_bateria: 25,
      status: 'nao_concluida',
    },
  ];

  beforeEach(() => {
    clienteSimulado.limpar();
    clienteSimulado.definirTabela('corridas', corridasSimuladas);
  });

  it('renderiza as corridas retornadas pelo cliente Supabase simulado', async () => {
    render(<TelaHistorico labirintos={labirintos} />);

    // Inicialmente exibe o estado de carregamento
    expect(screen.getByText('Carregando histórico do banco de dados…')).toBeInTheDocument();

    // Após resolução da consulta ao Supabase simulado
    await waitFor(() => {
      expect(screen.queryByText('Carregando histórico do banco de dados…')).not.toBeInTheDocument();
    });

    // Confere a contagem e os dados renderizados das corridas
    expect(screen.getByText('2 corridas')).toBeInTheDocument();

    // Corrida 10: Labirinto 1 (4×4), 1ª Tentativa (Nota 10)
    expect(screen.getByRole('cell', { name: 'Labirinto 1 (4×4)' })).toBeInTheDocument();
    expect(screen.getByText('1ª Tentativa (Nota 10)')).toBeInTheDocument();
    expect(screen.getByText('00:45.000')).toBeInTheDocument();
    expect(screen.getByText('0.35 m/s')).toBeInTheDocument();
    expect(screen.getByText('88%')).toBeInTheDocument(); // 100 - 12%

    // Corrida 11: Labirinto 2 (8×4), 2ª Tentativa (Não concluída)
    expect(screen.getByRole('cell', { name: 'Labirinto 2 (8×4)' })).toBeInTheDocument();
    expect(screen.getByText('2ª Tentativa (Não concluída)')).toBeInTheDocument();
    expect(screen.getByText('01:30.000')).toBeInTheDocument();
    expect(screen.getByText('0.28 m/s')).toBeInTheDocument();
    expect(screen.getByText('75%')).toBeInTheDocument(); // 100 - 25%
  });

  it('permite filtrar as corridas pelo tipo de labirinto selecionado', async () => {
    const user = userEvent.setup();
    render(<TelaHistorico labirintos={labirintos} />);

    await waitFor(() => {
      expect(screen.getByText('2 corridas')).toBeInTheDocument();
    });

    // Seleciona o filtro "Labirinto 1 (4×4)"
    const botaoFiltroL1 = screen.getByRole('button', { name: /Labirinto 1/i });
    await user.click(botaoFiltroL1);

    expect(screen.getByText('1 corrida')).toBeInTheDocument();
    expect(screen.getByText('1ª Tentativa (Nota 10)')).toBeInTheDocument();
    expect(screen.queryByText('2ª Tentativa (Não concluída)')).not.toBeInTheDocument();

    // Retorna para "Todos os Labirintos"
    const botaoTodos = screen.getByRole('button', { name: /Todos os Labirintos/i });
    await user.click(botaoTodos);

    expect(screen.getByText('2 corridas')).toBeInTheDocument();
    expect(screen.getByText('2ª Tentativa (Não concluída)')).toBeInTheDocument();
  });

  it('atualiza os dados na tabela ao clicar no botão Atualizar', async () => {
    const user = userEvent.setup();
    render(<TelaHistorico labirintos={labirintos} />);

    await waitFor(() => {
      expect(screen.getByText('2 corridas')).toBeInTheDocument();
    });

    // Insere uma nova corrida no Supabase simulado
    const novaCorrida: Corrida = {
      id: 12,
      labirinto_id: 3,
      numero_tentativa: 1,
      data_hora_inicio: '2026-10-22T16:00:00Z',
      data_hora_fim: '2026-10-22T16:01:10Z',
      tempo_final: 70.0,
      velocidade_media: 0.4,
      velocidade_pico: 0.7,
      consumo_bateria: 18,
      status: 'concluida',
    };
    clienteSimulado.definirTabela('corridas', [...corridasSimuladas, novaCorrida]);

    const botaoAtualizar = screen.getByRole('button', { name: 'Atualizar' });
    await user.click(botaoAtualizar);

    await waitFor(() => {
      expect(screen.getByText('3 corridas')).toBeInTheDocument();
    });

    expect(screen.getByRole('cell', { name: 'Labirinto 3 (12×4)' })).toBeInTheDocument();
  });

  it('exibe mensagem de erro quando o cliente Supabase falha na consulta', async () => {
    clienteSimulado.definirErro('corridas', 'Erro de conexão com o banco');

    render(<TelaHistorico labirintos={labirintos} />);

    await waitFor(() => {
      expect(screen.getByRole('alert')).toBeInTheDocument();
    });

    expect(screen.getByText('Não foi possível carregar as corridas do Supabase.')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Tentar novamente' })).toBeInTheDocument();
  });
});