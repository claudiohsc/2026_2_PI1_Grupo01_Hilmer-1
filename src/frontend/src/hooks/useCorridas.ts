import { useCallback, useEffect, useRef, useState } from 'react';
import { CORRIDAS_EXEMPLO } from '../dados/historicoExemplo';
import { supabase } from '../lib/supabase';
import type { Corrida } from '../tipos';

export interface RetornoUseCorridas {
  corridas: Corrida[];
  carregando: boolean;
  erro: string | null;
  recarregar: () => Promise<void>;
}

/** Corridas de exemplo do labirinto pedido (todas com `null`). */
function exemplosDoLabirinto(labirintoId: number | null): Corrida[] {
  return labirintoId === null ? CORRIDAS_EXEMPLO : CORRIDAS_EXEMPLO.filter((c) => c.labirinto_id === labirintoId);
}

/**
 * Hook para buscar o histórico de corridas no Supabase (HU-10 / Issue #235; filtro da HU-15 / #234).
 *
 * Realiza uma consulta GET na tabela `corridas` com ordenação da mais recente
 * para a mais antiga (`data_hora_inicio` decrescente). Com `labirintoId`, a consulta já
 * vai filtrada ao banco (`labirinto_id = id`); com `null`, traz todas as corridas. Trocar o
 * labirinto refaz a consulta.
 *
 * Caso o Supabase não esteja configurado, utiliza os dados de exemplo como fallback.
 */
export function useCorridas(labirintoId: number | null = null): RetornoUseCorridas {
  const [corridas, setCorridas] = useState<Corrida[]>(supabase ? [] : exemplosDoLabirinto(labirintoId));
  const [carregando, setCarregando] = useState<boolean>(Boolean(supabase));
  const [erro, setErro] = useState<string | null>(null);

  // Evita atualizar o estado depois que a tela foi desmontada (resposta que chega tarde).
  const montado = useRef(true);
  // Só a resposta da consulta mais recente vale: trocando o filtro depressa, uma resposta
  // atrasada do filtro anterior não pode sobrescrever a do filtro atual.
  const ultimaConsulta = useRef(0);

  const recarregar = useCallback(async () => {
    if (!supabase) {
      setCorridas(exemplosDoLabirinto(labirintoId));
      setCarregando(false);
      setErro(null);
      return;
    }

    const consulta = ++ultimaConsulta.current;
    setCarregando(true);
    setErro(null);

    let pedido = supabase.from('corridas').select('*');
    if (labirintoId !== null) {
      pedido = pedido.eq('labirinto_id', labirintoId);
    }
    const { data, error } = await pedido.order('data_hora_inicio', { ascending: false });

    if (!montado.current || consulta !== ultimaConsulta.current) return;

    if (error) {
      console.warn('Supabase: não foi possível ler a tabela corridas.', error);
      setErro('Não foi possível carregar as corridas do Supabase.');
    } else {
      setCorridas((data as Corrida[]) ?? []);
    }

    setCarregando(false);
  }, [labirintoId]);

  // A primeira carga, e cada troca de labirinto, usam a mesma consulta do botão "Atualizar".
  useEffect(() => {
    montado.current = true;
    void recarregar();
    return () => {
      montado.current = false;
    };
  }, [recarregar]);

  return { corridas, carregando, erro, recarregar };
}
