import { vi } from 'vitest';
import type { ClienteSupabaseSimulado } from './supabaseSimulado';

/** Uma consulta feita ao cliente simulado: a tabela e os filtros `eq` pedidos ao banco. */
export interface ConsultaRegistrada {
  tabela: string;
  filtros: Record<string, unknown>;
}

/**
 * Registra as consultas `select` feitas ao Supabase simulado, com os filtros `eq` de cada uma,
 * para os testes conferirem o que foi **pedido ao banco**, e não só o que a tela mostrou.
 *
 * @param atrasoMs Opcional: atraso, em ms, da resposta de cada consulta (para simular respostas
 * fora de ordem). Recebe a consulta e devolve o atraso; 0 responde na hora.
 */
export function registrarConsultas(
  cliente: ClienteSupabaseSimulado,
  atrasoMs: (consulta: ConsultaRegistrada) => number = () => 0,
): ConsultaRegistrada[] {
  const consultas: ConsultaRegistrada[] = [];
  const fromOriginal = cliente.from.bind(cliente);

  vi.spyOn(cliente, 'from').mockImplementation((tabela: string) => {
    const tabelaSimulada = fromOriginal(tabela);
    return {
      ...tabelaSimulada,
      select(colunas?: string) {
        const builder = tabelaSimulada.select(colunas);
        const consulta: ConsultaRegistrada = { tabela, filtros: {} };
        consultas.push(consulta);

        const eqOriginal = builder.eq.bind(builder);
        builder.eq = (coluna, valor) => {
          consulta.filtros[String(coluna)] = valor;
          return eqOriginal(coluna, valor);
        };
        const thenOriginal = builder.then.bind(builder);
        builder.then = ((aoCumprir, aoRejeitar) =>
          thenOriginal(async (resposta) => {
            const atraso = atrasoMs(consulta);
            if (atraso > 0) await new Promise((resolver) => setTimeout(resolver, atraso));
            return aoCumprir ? aoCumprir(resposta) : resposta;
          }, aoRejeitar)) as typeof builder.then;
        return builder;
      },
    };
  });

  return consultas;
}
