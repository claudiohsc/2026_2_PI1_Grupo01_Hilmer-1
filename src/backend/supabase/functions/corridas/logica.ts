/// Regras da Edge Function `corridas`: validação dos corpos e tipos compartilhados.
/// Sem dependência de Deno nem do Supabase, para rodar também nos testes com Node.

import { analisarId, ehObjeto } from "../_shared/validacao.ts";
import type { Validacao } from "../_shared/validacao.ts";

export { analisarId };
export type { Validacao };

export const TIPOS_LABIRINTO = ["4x4", "8x4", "12x4"] as const;
export type TipoLabirinto = (typeof TIPOS_LABIRINTO)[number];

export const STATUS_FINAIS = ["concluida", "nao_concluida"] as const;
export type StatusFinal = (typeof STATUS_FINAIS)[number];

/** Linha da tabela `corridas`, como devolvida pelo banco. */
export interface Corrida {
  id: number;
  labirinto_id: number;
  numero_tentativa: number;
  data_hora_inicio: string;
  data_hora_fim: string | null;
  tempo_final: number | null;
  velocidade_media: number | null;
  velocidade_pico: number | null;
  consumo_bateria: number | null;
  status: string;
}

export interface DadosEncerramento {
  /** Sem status, o banco decide pela telemetria: `concluida` se houve `objetivo_alcancado`, senão `nao_concluida`. */
  status?: StatusFinal;
  tempo_final?: number;
  velocidade_media?: number;
  velocidade_pico?: number;
  consumo_bateria?: number;
}

/** Corpo de `POST /corridas/{id}/mapa`: as células do mapa final, como o `MapaJson` do firmware. */
export interface DadosMapa {
  celulas: unknown[];
}

/** Corpo de `POST /corridas`: `{ "labirinto": "4x4" | "8x4" | "12x4" }`. O labirinto é obrigatório. */
export function validarCriacao(corpo: unknown): Validacao<{ tipo: TipoLabirinto }> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  const tipo = corpo.labirinto;
  if (tipo === undefined || tipo === null) {
    return { ok: false, mensagem: "o campo 'labirinto' é obrigatório" };
  }
  if (!(TIPOS_LABIRINTO as readonly unknown[]).includes(tipo)) {
    return { ok: false, mensagem: `labirinto inválido; use um de: ${TIPOS_LABIRINTO.join(", ")}` };
  }
  return { ok: true, valor: { tipo: tipo as TipoLabirinto } };
}

const CAMPOS_NUMERICOS = ["tempo_final", "velocidade_media", "velocidade_pico", "consumo_bateria"] as const;

/**
 * Corpo de `POST /corridas/{id}/encerrar`. Tudo é opcional: o `status` (`concluida` ou
 * `nao_concluida`) e as métricas numéricas, finitas e não negativas. O que o robô não informar, o
 * banco calcula pela telemetria já gravada. Sem corpo, vale `{}`.
 */
export function validarEncerramento(corpo: unknown): Validacao<DadosEncerramento> {
  const dadosEnviados = corpo === undefined ? {} : corpo;
  if (!ehObjeto(dadosEnviados)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  const dados: DadosEncerramento = {};
  const status = dadosEnviados.status;
  if (status !== undefined && status !== null) {
    if (!(STATUS_FINAIS as readonly unknown[]).includes(status)) {
      return { ok: false, mensagem: `status inválido; use um de: ${STATUS_FINAIS.join(", ")}` };
    }
    dados.status = status as StatusFinal;
  }
  for (const campo of CAMPOS_NUMERICOS) {
    const valor = dadosEnviados[campo];
    if (valor === undefined || valor === null) {
      continue;
    }
    if (typeof valor !== "number" || !Number.isFinite(valor) || valor < 0) {
      return { ok: false, mensagem: `o campo '${campo}' deve ser um número finito e não negativo` };
    }
    dados[campo] = valor;
  }
  return { ok: true, valor: dados };
}

/**
 * Corpo de `POST /corridas/{id}/mapa`: um objeto com a lista `celulas` (o `MapaJson` do firmware,
 * que também traz `dimensao`, `linhas` e `colunas`, ignorados aqui). A conferência de cada célula
 * (limites, paredes) fica no banco, em `registrar_mapa`, que recusa o mapa inteiro se uma for inválida.
 */
export function validarMapa(corpo: unknown): Validacao<DadosMapa> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  const celulas = corpo.celulas;
  if (!Array.isArray(celulas) || celulas.length === 0) {
    return { ok: false, mensagem: "o campo 'celulas' deve ser uma lista com pelo menos uma célula" };
  }
  return { ok: true, valor: { celulas } };
}
