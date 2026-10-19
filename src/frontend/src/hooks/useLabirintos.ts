import { useEffect, useState } from 'react';
import { LABIRINTOS_EXEMPLO } from '../dados/labirintosExemplo';
import { supabase } from '../lib/supabase';
import type { Labirinto } from '../tipos';

/** Situação da conexão com o Supabase, mostrada no rodapé. */
export type EstadoConexao = 'sem_configuracao' | 'verificando' | 'conectado' | 'erro';

/**
 * Lista de labirintos. Começa com os dados de exemplo e, se o Supabase estiver configurado,
 * troca pela tabela `labirintos` (leitura pública pelo RLS). É a única leitura real desta
 * etapa e serve para confirmar que o cliente supabase-js está funcionando.
 */
export function useLabirintos(): { labirintos: Labirinto[]; conexao: EstadoConexao } {
  const [labirintos, setLabirintos] = useState<Labirinto[]>(LABIRINTOS_EXEMPLO);
  const [conexao, setConexao] = useState<EstadoConexao>(supabase ? 'verificando' : 'sem_configuracao');

  useEffect(() => {
    if (!supabase) return;
    let ativo = true;

    supabase
      .from('labirintos')
      .select('id, tipo, linhas, colunas')
      .order('id')
      .then(({ data, error }) => {
        if (!ativo) return;
        if (error || !data || data.length === 0) {
          console.warn('Supabase: não foi possível ler a tabela labirintos.', error);
          setConexao('erro');
          return;
        }
        setLabirintos(data as Labirinto[]);
        setConexao('conectado');
      });

    return () => {
      ativo = false;
    };
  }, []);

  return { labirintos, conexao };
}
