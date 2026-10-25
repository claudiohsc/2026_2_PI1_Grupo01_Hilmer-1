/// Regras da Edge Function `telemetria`: validação do corpo do lote e tipos compartilhados.
/// Sem dependência de Deno nem do Supabase, para rodar também nos testes com Node.

import { ehIdValido, ehObjeto } from "../_shared/validacao.ts";
import type { Validacao } from "../_shared/validacao.ts";

/** Maior lote aceito, igual ao limite de `registrar_telemetria` no banco. */
export const MAXIMO_AMOSTRAS_POR_LOTE = 100;

/** Corpo de `POST /telemetria`: a corrida e as amostras, com os nomes das colunas de `telemetria`. */
export interface LoteTelemetria {
  corrida_id: number;
  amostras: unknown[];
}

/** Resposta de `registrar_telemetria`: quantas amostras entraram, quantas já existiam e se a corrida foi reconsolidada. */
export interface ResultadoLote {
  gravadas: number;
  existentes: number;
  reconsolidada: boolean;
}

/**
 * Corpo de `POST /telemetria`: `{ "corrida_id": 7, "amostras": [ { "numero_sequencia": 1, ... } ] }`.
 *
 * Aqui só se confere a forma: corrida válida e lista de 1 a `MAXIMO_AMOSTRAS_POR_LOTE` objetos. Os
 * valores de cada amostra (limites da malha, orientação, tempo e tensão) são conferidos no banco, que
 * recusa o lote inteiro se uma amostra for inválida.
 */
export function validarLote(corpo: unknown): Validacao<LoteTelemetria> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  if (corpo.corrida_id === undefined || corpo.corrida_id === null) {
    return { ok: false, mensagem: "o campo 'corrida_id' é obrigatório" };
  }
  if (!ehIdValido(corpo.corrida_id)) {
    return { ok: false, mensagem: "o campo 'corrida_id' deve ser um inteiro positivo" };
  }
  const amostras = corpo.amostras;
  if (!Array.isArray(amostras) || amostras.length === 0) {
    return { ok: false, mensagem: "o campo 'amostras' deve ser uma lista com pelo menos uma amostra" };
  }
  if (amostras.length > MAXIMO_AMOSTRAS_POR_LOTE) {
    return { ok: false, mensagem: `o lote tem mais de ${MAXIMO_AMOSTRAS_POR_LOTE} amostras` };
  }
  if (!amostras.every(ehObjeto)) {
    return { ok: false, mensagem: "cada amostra deve ser um objeto JSON" };
  }
  return { ok: true, valor: { corrida_id: corpo.corrida_id, amostras } };
}
