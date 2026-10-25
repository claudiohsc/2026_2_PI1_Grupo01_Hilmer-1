/// Roteamento da Edge Function `telemetria`, sem Deno nem Supabase: recebe os dados da requisição e
/// um repositório, e devolve status e corpo. O `index.ts` só liga isto ao servidor HTTP.
///
/// Os códigos de resposta seguem o que o firmware espera (`PoliticaReenvio`): 2xx confirma o lote;
/// 400, 404 e 413 descartam o lote (não adianta repetir); 401 e 403 pausam o envio; 5xx repete.

import { tokenValido } from "../_shared/autenticacao.ts";
import { validarLote } from "./logica.ts";
import type { LoteTelemetria, ResultadoLote } from "./logica.ts";

export type CodigoErro = "corrida_inexistente" | "amostras_invalidas" | "lote_grande_demais";

/** Erro de regra de negócio vindo do banco (por exemplo, uma amostra fora da malha do labirinto). */
export class ErroDeNegocio extends Error {
  codigo: CodigoErro;
  constructor(codigo: CodigoErro) {
    super(codigo);
    this.codigo = codigo;
  }
}

/** Acesso ao banco. A implementação real chama `registrar_telemetria` com a chave de serviço. */
export interface Repositorio {
  registrarTelemetria(lote: LoteTelemetria): Promise<ResultadoLote>;
}

export interface Requisicao {
  metodo: string;
  caminho: string;
  tokenRecebido: string | null;
  /** Corpo já lido como JSON; `undefined` se estiver vazio e `CORPO_INVALIDO` se não for JSON. */
  corpo: unknown;
}

export interface Resposta {
  status: number;
  corpo: unknown;
}

export interface Dependencias {
  tokenEsperado: string | undefined;
  repositorio: Repositorio;
}

/** Marca o corpo que não pôde ser lido como JSON. */
export const CORPO_INVALIDO = Symbol("corpo_invalido");

function erro(status: number, codigo: string, mensagem: string): Resposta {
  return { status, corpo: { erro: codigo, mensagem } };
}

const STATUS_POR_ERRO: Record<CodigoErro, { status: number; mensagem: string }> = {
  corrida_inexistente: { status: 404, mensagem: "corrida não encontrada" },
  amostras_invalidas: { status: 400, mensagem: "alguma amostra do lote é inválida; nenhuma foi gravada" },
  lote_grande_demais: { status: 413, mensagem: "o lote tem amostras demais" },
};

/** `true` se o caminho termina em `telemetria` (por exemplo, `/functions/v1/telemetria`). */
function ehRotaDaTelemetria(caminho: string): boolean {
  const partes = caminho.split("/").filter((p) => p.length > 0);
  return partes.length > 0 && partes[partes.length - 1] === "telemetria";
}

export async function tratarRequisicao(req: Requisicao, deps: Dependencias): Promise<Resposta> {
  if (!tokenValido(req.tokenRecebido, deps.tokenEsperado)) {
    return erro(401, "nao_autorizado", "token de dispositivo ausente ou inválido");
  }
  if (!ehRotaDaTelemetria(req.caminho)) {
    return erro(404, "rota_inexistente", "rota não encontrada");
  }
  if (req.metodo !== "POST") {
    return erro(405, "metodo_nao_permitido", "use POST em /telemetria");
  }
  if (req.corpo === CORPO_INVALIDO) {
    return erro(400, "corpo_invalido", "o corpo não é um JSON válido");
  }
  const validacao = validarLote(req.corpo);
  if (!validacao.ok) {
    return erro(400, "corpo_invalido", validacao.mensagem);
  }

  try {
    const resultado = await deps.repositorio.registrarTelemetria(validacao.valor);
    return { status: 200, corpo: resultado };
  } catch (e) {
    if (e instanceof ErroDeNegocio) {
      const { status, mensagem } = STATUS_POR_ERRO[e.codigo];
      return erro(status, e.codigo, mensagem);
    }
    return erro(500, "erro_interno", "erro ao acessar o banco de dados");
  }
}
