import { useCallback, useEffect, useState } from 'react';
import { CORRIDAS_EXEMPLO } from '../dados/historicoExemplo';
import { supabase } from '../lib/supabase';
import type { Corrida } from '../tipos';

export interface RetornoUseCorridas {
  corridas: Corrida[];
  carregando: boolean;
  erro: string | null;
  recarregar: () => Promise<void>;
}

/**
 * Hook para buscar o histórico de corridas no Supabase (HU-10 / Issue #235).
 *
 * Realiza uma consulta GET na tabela `corridas` com ordenação da mais recente
 * para a mais antiga (`data_hora_inicio` decrescente).
 *
 * Caso o Supabase não esteja configurado, utiliza os dados de exemplo como fallback.
 */
export function useCorridas(): RetornoUseCorridas {
  const [corridas, setCorridas] = useState<Corrida[]>(supabase ? [] : CORRIDAS_EXEMPLO);
  const [carregando, setCarregando] = useState<boolean>(Boolean(supabase));
  const [erro, setErro] = useState<string | null>(null);

  const recarregar = useCallback(async () => {
    if (!supabase) {
      setCorridas(CORRIDAS_EXEMPLO);
      setCarregando(false);
      setErro(null);
      return;
    }

    setCarregando(true);
    setErro(null);

    const { data, error } = await supabase
      .from('corridas')
      .select('*')
      .order('data_hora_inicio', { ascending: false });

    if (error) {
      console.warn('Supabase: não foi possível ler a tabela corridas.', error);
      setErro('Não foi possível carregar as corridas do Supabase.');
    } else {
      setCorridas((data as Corrida[]) ?? []);
    }

    setCarregando(false);
  }, []);

  useEffect(() => {
    let ativo = true;

    if (!supabase) return;

    setCarregando(true);
    setErro(null);

    supabase
      .from('corridas')
      .select('*')
      .order('data_hora_inicio', { ascending: false })
      .then(({ data, error }) => {
        if (!ativo) return;

        if (error) {
          console.warn('Supabase: não foi possível ler a tabela corridas.', error);
          setErro('Não foi possível carregar as corridas do Supabase.');
        } else {
          setCorridas((data as Corrida[]) ?? []);
        }

        setCarregando(false);
      });

    return () => {
      ativo = false;
    };
  }, []);

  return { corridas, carregando, erro, recarregar };
}

