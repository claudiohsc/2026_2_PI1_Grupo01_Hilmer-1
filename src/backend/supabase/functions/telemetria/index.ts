// Edge Function `telemetria`: grava os lotes de amostras que o robô envia durante a corrida.
//
// O robô chama:
//   POST /functions/v1/telemetria   { "corrida_id": 7, "amostras": [ { "numero_sequencia": 1, ... } ] }
// com o cabeçalho `x-device-token`, e recebe `{ "gravadas": n, "existentes": m, "reconsolidada": bool }`.
// A regra de negócio (idempotência por `numero_sequencia`, lote atômico de até 100 amostras) fica na
// função SQL `registrar_telemetria`; a chave `anon` não tem permissão de escrita direta nas tabelas.
//
// Segredos: DEVICE_TOKEN (o mesmo da função `corridas`, definido com `supabase secrets set`).
// SUPABASE_URL e SUPABASE_SERVICE_ROLE_KEY já são fornecidos pelo Supabase às Edge Functions.

import { createClient } from "npm:@supabase/supabase-js@2.45.4";
import { CORPO_INVALIDO, ErroDeNegocio, tratarRequisicao } from "./roteador.ts";
import type { CodigoErro, Repositorio } from "./roteador.ts";
import type { LoteTelemetria, ResultadoLote } from "./logica.ts";

const CODIGOS: CodigoErro[] = ["corrida_inexistente", "amostras_invalidas", "lote_grande_demais"];

/** Converte a mensagem de erro do banco em erro de negócio, quando for um dos conhecidos. */
function converterErro(mensagem: string): Error {
  const codigo = CODIGOS.find((c) => mensagem.includes(c));
  return codigo ? new ErroDeNegocio(codigo) : new Error(mensagem);
}

/** Cliente com a chave de serviço, criado na primeira chamada que precisa do banco. */
let cliente: ReturnType<typeof createClient> | undefined;
function obterCliente() {
  cliente ??= createClient(Deno.env.get("SUPABASE_URL")!, Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!, {
    auth: { persistSession: false },
  });
  return cliente;
}

const repositorio: Repositorio = {
  async registrarTelemetria(lote: LoteTelemetria): Promise<ResultadoLote> {
    const { data, error } = await obterCliente().rpc("registrar_telemetria", {
      p_corrida_id: lote.corrida_id,
      p_amostras: lote.amostras,
    });
    if (error) throw converterErro(error.message);
    return data as ResultadoLote;
  },
};

Deno.serve(async (req: Request) => {
  let corpo: unknown = undefined;
  const texto = await req.text();
  if (texto.length > 0) {
    try {
      corpo = JSON.parse(texto);
    } catch {
      corpo = CORPO_INVALIDO;
    }
  }

  const resposta = await tratarRequisicao(
    {
      metodo: req.method,
      caminho: new URL(req.url).pathname,
      tokenRecebido: req.headers.get("x-device-token"),
      corpo,
    },
    { tokenEsperado: Deno.env.get("DEVICE_TOKEN") ?? undefined, repositorio },
  );

  return new Response(JSON.stringify(resposta.corpo), {
    status: resposta.status,
    headers: { "Content-Type": "application/json" },
  });
});
