import type { Corrida, Labirinto, Telemetria } from '../tipos';

export type EventoPostgres = 'INSERT' | 'UPDATE' | 'DELETE' | '*';

export interface PayloadPostgresChanges<T = Record<string, unknown>> {
  schema: string;
  table: string;
  commit_timestamp: string;
  eventType: 'INSERT' | 'UPDATE' | 'DELETE';
  new: T;
  old: Partial<T>;
  errors: null | string[];
}

export type CallbackPostgresChanges<T = Record<string, unknown>> = (
  payload: PayloadPostgresChanges<T>,
) => void;

export interface FiltroCanal {
  event: EventoPostgres;
  schema?: string;
  table: string;
  filter?: string;
}

export interface CanalRealtimeSimulado {
  nome: string;
  estado: 'SUBSCRIBED' | 'CLOSED' | 'TIMED_OUT';
  on(
    tipo: 'postgres_changes',
    filtro: FiltroCanal,
    callback: CallbackPostgresChanges<any>,
  ): CanalRealtimeSimulado;
  subscribe(callback?: (status: string) => void): CanalRealtimeSimulado;
  unsubscribe(): Promise<'ok'>;
}

export interface ConsultaBuilder<T> extends PromiseLike<{ data: T[] | null; error: { message: string } | null }> {
  select(colunas?: string): ConsultaBuilder<T>;
  order(coluna: keyof T | string, opcoes?: { ascending?: boolean }): ConsultaBuilder<T>;
  eq(coluna: keyof T | string, valor: unknown): ConsultaBuilder<T>;
  neq(coluna: keyof T | string, valor: unknown): ConsultaBuilder<T>;
  limit(quantidade: number): ConsultaBuilder<T>;
  then<TResult1 = { data: T[] | null; error: { message: string } | null }, TResult2 = never>(
    onfulfilled?: ((value: { data: T[] | null; error: { message: string } | null }) => TResult1 | PromiseLike<TResult1>) | null,
    onrejected?: ((reason: any) => TResult2 | PromiseLike<TResult2>) | null,
  ): Promise<TResult1 | TResult2>;
}

export interface DadosIniciaisSimulados {
  corridas?: Corrida[];
  labirintos?: Labirinto[];
  telemetria?: Telemetria[];
  [tabela: string]: any[] | undefined;
}

export interface ClienteSupabaseSimulado {
  from<T = any>(tabela: string): {
    select(colunas?: string): ConsultaBuilder<T>;
    insert(linhas: T | T[]): Promise<{ data: T[] | null; error: { message: string } | null }>;
  };
  channel(nome: string): CanalRealtimeSimulado;
  removeChannel(canal: CanalRealtimeSimulado): Promise<'ok'>;

  // Utilitários de simulação para os testes
  definirTabela<T = any>(tabela: string, dados: T[]): void;
  definirErro(tabela: string, erro: string | Error | null): void;
  obterDados<T = any>(tabela: string): T[];
  emitirRealtime<T = any>(
    tabela: string,
    evento: 'INSERT' | 'UPDATE' | 'DELETE',
    novo: T,
    antigo?: Partial<T>,
  ): void;
  obterCanais(): CanalRealtimeSimulado[];
  limpar(): void;
}

/**
 * Cria uma instância simulada em memória do cliente Supabase para testes unitários e de integração.
 *
 * Suporta consultas encadeadas (`.from().select().order().eq()`), inserções e canais Realtime
 * (`.channel().on('postgres_changes', ...).subscribe()`).
 */
export function criarClienteSupabaseSimulado(
  dadosIniciais: DadosIniciaisSimulados = {},
): ClienteSupabaseSimulado {
  const tabelas: Map<string, any[]> = new Map();
  const erros: Map<string, { message: string } | null> = new Map();
  const assinantesRealtime: Array<{
    canal: CanalRealtimeSimulado;
    filtro: FiltroCanal;
    callback: CallbackPostgresChanges<any>;
  }> = [];
  const canais: CanalRealtimeSimulado[] = [];

  for (const [tabela, linhas] of Object.entries(dadosIniciais)) {
    if (linhas) {
      tabelas.set(tabela, [...linhas]);
    }
  }

  function aplicarFiltrosEOrdem<T>(
    linhas: T[],
    filtros: Array<(item: T) => boolean>,
    ordenacao?: { coluna: keyof T | string; ascending: boolean },
    limite?: number,
  ): T[] {
    let resultado = linhas.filter((item) => filtros.every((fn) => fn(item)));

    if (ordenacao) {
      const { coluna, ascending } = ordenacao;
      resultado = [...resultado].sort((a: any, b: any) => {
        const valA = a[coluna];
        const valB = b[coluna];
        if (valA === valB) return 0;
        if (valA === null || valA === undefined) return 1;
        if (valB === null || valB === undefined) return -1;
        if (valA < valB) return ascending ? -1 : 1;
        return ascending ? 1 : -1;
      });
    }

    if (limite !== undefined && limite >= 0) {
      resultado = resultado.slice(0, limite);
    }

    return resultado;
  }

  function criarBuilder<T>(tabela: string, _colunas?: string): ConsultaBuilder<T> {
    const filtros: Array<(item: T) => boolean> = [];
    let ordenacao: { coluna: keyof T | string; ascending: boolean } | undefined;
    let limite: number | undefined;

    const builder: ConsultaBuilder<T> = {
      select(_colunasNovas?: string) {
        return builder;
      },
      order(coluna: keyof T | string, opcoes?: { ascending?: boolean }) {
        ordenacao = { coluna, ascending: opcoes?.ascending ?? true };
        return builder;
      },
      eq(coluna: keyof T | string, valor: unknown) {
        filtros.push((item: any) => item[coluna] === valor);
        return builder;
      },
      neq(coluna: keyof T | string, valor: unknown) {
        filtros.push((item: any) => item[coluna] !== valor);
        return builder;
      },
      limit(quantidade: number) {
        limite = quantidade;
        return builder;
      },
      then<TResult1 = { data: T[] | null; error: { message: string } | null }, TResult2 = never>(
        onfulfilled?: ((value: { data: T[] | null; error: { message: string } | null }) => TResult1 | PromiseLike<TResult1>) | null,
        onrejected?: ((reason: any) => TResult2 | PromiseLike<TResult2>) | null,
      ): Promise<TResult1 | TResult2> {
        const promise = new Promise<{ data: T[] | null; error: { message: string } | null }>((resolve) => {
          const erroConfigurado = erros.get(tabela);
          if (erroConfigurado) {
            resolve({ data: null, error: erroConfigurado });
            return;
          }

          const linhas = (tabelas.get(tabela) as T[]) ?? [];
          const resultado = aplicarFiltrosEOrdem(linhas, filtros, ordenacao, limite);
          resolve({ data: resultado, error: null });
        });

        return promise.then(onfulfilled, onrejected);
      },
    };

    return builder;
  }

  const cliente: ClienteSupabaseSimulado = {
    from<T = any>(tabela: string) {
      return {
        select(colunas?: string) {
          return criarBuilder<T>(tabela, colunas);
        },
        async insert(linhas: T | T[]) {
          const erroConfigurado = erros.get(tabela);
          if (erroConfigurado) {
            return { data: null, error: erroConfigurado };
          }

          const arrayLinhas = Array.isArray(linhas) ? linhas : [linhas];
          const existentes = (tabelas.get(tabela) as T[]) ?? [];
          tabelas.set(tabela, [...existentes, ...arrayLinhas]);

          for (const linha of arrayLinhas) {
            cliente.emitirRealtime(tabela, 'INSERT', linha);
          }

          return { data: arrayLinhas, error: null };
        },
      };
    },

    channel(nome: string): CanalRealtimeSimulado {
      const canal: CanalRealtimeSimulado = {
        nome,
        estado: 'CLOSED',
        on(tipo, filtro, callback) {
          if (tipo === 'postgres_changes') {
            assinantesRealtime.push({ canal, filtro, callback });
          }
          return canal;
        },
        subscribe(callback?: (status: string) => void) {
          canal.estado = 'SUBSCRIBED';
          if (callback) {
            // Notifica de forma assíncrona como o Supabase Realtime real
            queueMicrotask(() => callback('SUBSCRIBED'));
          }
          return canal;
        },
        async unsubscribe() {
          canal.estado = 'CLOSED';
          const indicesParaRemover = assinantesRealtime
            .map((item, index) => (item.canal === canal ? index : -1))
            .filter((idx) => idx !== -1)
            .reverse();

          for (const idx of indicesParaRemover) {
            assinantesRealtime.splice(idx, 1);
          }

          return 'ok';
        },
      };

      canais.push(canal);
      return canal;
    },

    async removeChannel(canal: CanalRealtimeSimulado) {
      await canal.unsubscribe();
      const index = canais.indexOf(canal);
      if (index !== -1) {
        canais.splice(index, 1);
      }
      return 'ok';
    },

    definirTabela<T = any>(tabela: string, dados: T[]) {
      tabelas.set(tabela, [...dados]);
    },

    definirErro(tabela: string, erro: string | Error | null) {
      if (erro === null) {
        erros.delete(tabela);
      } else if (typeof erro === 'string') {
        erros.set(tabela, { message: erro });
      } else {
        erros.set(tabela, { message: erro.message });
      }
    },

    obterDados<T = any>(tabela: string): T[] {
      return (tabelas.get(tabela) as T[]) ?? [];
    },

    emitirRealtime<T = any>(
      tabela: string,
      evento: 'INSERT' | 'UPDATE' | 'DELETE',
      novo: T,
      antigo: Partial<T> = {},
    ) {
      const payload: PayloadPostgresChanges<T> = {
        schema: 'public',
        table: tabela,
        commit_timestamp: new Date().toISOString(),
        eventType: evento,
        new: novo,
        old: antigo,
        errors: null,
      };

      for (const assinante of assinantesRealtime) {
        if (assinante.canal.estado !== 'SUBSCRIBED') continue;
        if (assinante.filtro.table !== tabela && assinante.filtro.table !== '*') continue;
        if (assinante.filtro.event !== '*' && assinante.filtro.event !== evento) continue;

        try {
          assinante.callback(payload);
        } catch (err) {
          console.error('Erro no callback de realtime simulado:', err);
        }
      }
    },

    obterCanais() {
      return [...canais];
    },

    limpar() {
      tabelas.clear();
      erros.clear();
      assinantesRealtime.length = 0;
      canais.length = 0;
    },
  };

  return cliente;
}
