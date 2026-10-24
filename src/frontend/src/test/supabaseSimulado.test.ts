import { describe, expect, it, vi } from 'vitest';
import { criarClienteSupabaseSimulado } from './supabaseSimulado';
import type { Corrida } from '../tipos';

describe('ClienteSupabaseSimulado', () => {
  const corridaBase: Corrida = {
    id: 1,
    labirinto_id: 1,
    numero_tentativa: 1,
    data_hora_inicio: '2026-10-20T10:00:00Z',
    data_hora_fim: '2026-10-20T10:05:00Z',
    tempo_final: 50.2,
    velocidade_media: 0.25,
    velocidade_pico: 0.45,
    consumo_bateria: 15,
    status: 'concluida',
  };

  it('permite consultar e ordenar registros de corridas', async () => {
    const corridas: Corrida[] = [
      { ...corridaBase, id: 1, data_hora_inicio: '2026-10-20T10:00:00Z' },
      { ...corridaBase, id: 2, data_hora_inicio: '2026-10-20T11:00:00Z' },
      { ...corridaBase, id: 3, data_hora_inicio: '2026-10-20T09:00:00Z' },
    ];

    const supabase = criarClienteSupabaseSimulado({ corridas });

    const { data, error } = await supabase
      .from('corridas')
      .select('*')
      .order('data_hora_inicio', { ascending: false });

    expect(error).toBeNull();
    expect(data).toHaveLength(3);
    expect(data?.[0].id).toBe(2);
    expect(data?.[1].id).toBe(1);
    expect(data?.[2].id).toBe(3);
  });

  it('permite filtrar com eq, neq e aplicar limite', async () => {
    const corridas: Corrida[] = [
      { ...corridaBase, id: 1, labirinto_id: 1, status: 'concluida' },
      { ...corridaBase, id: 2, labirinto_id: 2, status: 'nao_concluida' },
      { ...corridaBase, id: 3, labirinto_id: 1, status: 'em_andamento' },
    ];

    const supabase = criarClienteSupabaseSimulado({ corridas });

    const resultado1 = await supabase
      .from('corridas')
      .select('*')
      .eq('labirinto_id', 1);

    expect(resultado1.data).toHaveLength(2);

    const resultado2 = await supabase
      .from('corridas')
      .select('*')
      .neq('status', 'concluida')
      .limit(1);

    expect(resultado2.data).toHaveLength(1);
    expect(resultado2.data?.[0].id).toBe(2);
  });

  it('suporta definir dados da tabela dinamicamente e simular erros', async () => {
    const supabase = criarClienteSupabaseSimulado();
    supabase.definirTabela('corridas', [corridaBase]);

    const res1 = await supabase.from('corridas').select('*');
    expect(res1.data).toEqual([corridaBase]);

    supabase.definirErro('corridas', 'Falha simulada no banco');
    const resErro = await supabase.from('corridas').select('*');
    expect(resErro.data).toBeNull();
    expect(resErro.error?.message).toBe('Falha simulada no banco');

    supabase.definirErro('corridas', null);
    const resRecuperada = await supabase.from('corridas').select('*');
    expect(resRecuperada.data).toHaveLength(1);
  });

  it('suporta canais Realtime e notifica ouvintes quando eventos são emitidos', async () => {
    const supabase = criarClienteSupabaseSimulado();
    const canal = supabase.channel('telemetria-ao-vivo');
    const callback = vi.fn();
    const statusCallback = vi.fn();

    canal.on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'telemetria' }, callback);
    canal.subscribe(statusCallback);

    await vi.waitFor(() => {
      expect(statusCallback).toHaveBeenCalledWith('SUBSCRIBED');
    });

    const leitura = { id: 101, corrida_id: 1, velocidade: 0.3 };
    supabase.emitirRealtime('telemetria', 'INSERT', leitura);

    expect(callback).toHaveBeenCalledTimes(1);
    expect(callback).toHaveBeenCalledWith(
      expect.objectContaining({
        eventType: 'INSERT',
        table: 'telemetria',
        new: leitura,
      }),
    );

    await canal.unsubscribe();
    supabase.emitirRealtime('telemetria', 'INSERT', { id: 102 });
    expect(callback).toHaveBeenCalledTimes(1);
  });

  it('permite inserção de novos registros e dispara evento de realtime', async () => {
    const supabase = criarClienteSupabaseSimulado();
    const canal = supabase.channel('corridas-updates');
    const callback = vi.fn();
    canal.on('postgres_changes', { event: '*', table: 'corridas' }, callback);
    canal.subscribe();

    const nova = { ...corridaBase, id: 99 };
    const { data, error } = await supabase.from('corridas').insert(nova);

    expect(error).toBeNull();
    expect(data).toEqual([nova]);
    expect(supabase.obterDados('corridas')).toHaveLength(1);
    expect(callback).toHaveBeenCalledWith(
      expect.objectContaining({
        eventType: 'INSERT',
        new: nova,
      }),
    );
  });

  it('remove canais com removeChannel', async () => {
    const supabase = criarClienteSupabaseSimulado();
    const canal = supabase.channel('canal-remover');
    expect(supabase.obterCanais()).toHaveLength(1);

    await supabase.removeChannel(canal);
    expect(supabase.obterCanais()).toHaveLength(0);
  });
});

