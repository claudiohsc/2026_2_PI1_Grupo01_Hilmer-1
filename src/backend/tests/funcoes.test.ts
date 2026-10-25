// Integração das Edge Functions `corridas` e `telemetria` com as funções SQL, num PostgreSQL de verdade
// (PGlite): a requisição HTTP simulada passa pelo roteador e pelo repositório, que chama o mesmo SQL
// que o Supabase executa. Cobre o contrato com o firmware (#236/#238): os corpos de
// `tests/fixtures`, os códigos de resposta que o `PoliticaReenvio` do robô espera e a corrida inteira
// (criar, enviar em lotes, reenviar, gravar o mapa e encerrar sem informar nada).

import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import type { PGlite } from "@electric-sql/pglite";
import { como, criarBanco } from "./ajudantes.ts";
import { ErroDeNegocio as ErroCorridas, tratarRequisicao as tratarCorridas } from "../supabase/functions/corridas/roteador.ts";
import type { CodigoErro as CodigoCorridas, Repositorio as RepositorioCorridas } from "../supabase/functions/corridas/roteador.ts";
import type { Corrida } from "../supabase/functions/corridas/logica.ts";
import { ErroDeNegocio as ErroTelemetria, tratarRequisicao as tratarTelemetria } from "../supabase/functions/telemetria/roteador.ts";
import type { CodigoErro as CodigoTelemetria, Repositorio as RepositorioTelemetria } from "../supabase/functions/telemetria/roteador.ts";
import type { ResultadoLote } from "../supabase/functions/telemetria/logica.ts";

const TOKEN = "token-do-robo";

function fixture(nome: string): any {
  return JSON.parse(readFileSync(fileURLToPath(new URL(`./fixtures/${nome}`, import.meta.url)), "utf8"));
}

/** Os erros do banco chegam com o código na mensagem, como no `index.ts` das funções. */
function codigoNaMensagem<T extends string>(e: unknown, codigos: readonly T[]): T | undefined {
  const mensagem = e instanceof Error ? e.message : String(e);
  return codigos.find((c) => mensagem.includes(c));
}

/** Repositórios que fazem o que os `index.ts` fazem, mas sobre o PGlite e como `service_role`. */
function repositorios(db: PGlite) {
  const codigosCorridas: CodigoCorridas[] = [
    "labirinto_inexistente",
    "corrida_inexistente",
    "corrida_ja_encerrada",
    "status_invalido",
    "celulas_invalidas",
    "mapa_grande_demais",
  ];
  const codigosTelemetria: CodigoTelemetria[] = ["corrida_inexistente", "amostras_invalidas", "lote_grande_demais"];

  const corridas: RepositorioCorridas = {
    async criarCorrida(tipo) {
      try {
        const r = await como(db, "service_role", () => db.query<{ c: Corrida }>("SELECT to_jsonb(c) AS c FROM criar_corrida($1) c", [tipo]));
        return r.rows[0].c;
      } catch (e) {
        const codigo = codigoNaMensagem(e, codigosCorridas);
        throw codigo ? new ErroCorridas(codigo) : e;
      }
    },
    async encerrarCorrida(id, dados) {
      try {
        const r = await como(db, "service_role", () =>
          db.query<{ c: Corrida }>("SELECT to_jsonb(c) AS c FROM encerrar_corrida($1, $2, $3, $4, $5, $6) c", [
            id,
            dados.status ?? null,
            dados.tempo_final ?? null,
            dados.velocidade_media ?? null,
            dados.velocidade_pico ?? null,
            dados.consumo_bateria ?? null,
          ]),
        );
        return r.rows[0].c;
      } catch (e) {
        const codigo = codigoNaMensagem(e, codigosCorridas);
        throw codigo ? new ErroCorridas(codigo) : e;
      }
    },
    async registrarMapa(id, celulas) {
      try {
        const r = await como(db, "service_role", () =>
          db.query<{ r: { novas: number; atualizadas: number } }>("SELECT registrar_mapa($1, $2::jsonb) AS r", [id, JSON.stringify(celulas)]),
        );
        return r.rows[0].r;
      } catch (e) {
        const codigo = codigoNaMensagem(e, codigosCorridas);
        throw codigo ? new ErroCorridas(codigo) : e;
      }
    },
  };

  const telemetria: RepositorioTelemetria = {
    async registrarTelemetria(lote) {
      try {
        const r = await como(db, "service_role", () =>
          db.query<{ r: ResultadoLote }>("SELECT registrar_telemetria($1, $2::jsonb) AS r", [lote.corrida_id, JSON.stringify(lote.amostras)]),
        );
        return r.rows[0].r;
      } catch (e) {
        const codigo = codigoNaMensagem(e, codigosTelemetria);
        throw codigo ? new ErroTelemetria(codigo) : e;
      }
    },
  };

  return { corridas, telemetria };
}

/** O robô chamando as funções: monta a requisição, passa pelo roteador certo e devolve status e corpo. */
function robo(db: PGlite) {
  const { corridas, telemetria } = repositorios(db);
  const base = { tokenEsperado: TOKEN };
  const requisicao = (caminho: string, corpo: unknown) => ({ metodo: "POST", caminho, tokenRecebido: TOKEN, corpo });
  return {
    criar: (labirinto: string) => tratarCorridas(requisicao("/functions/v1/corridas", { labirinto }), { ...base, repositorio: corridas }),
    enviar: (corpo: unknown) => tratarTelemetria(requisicao("/functions/v1/telemetria", corpo), { ...base, repositorio: telemetria }),
    mapa: (id: number, corpo: unknown) =>
      tratarCorridas(requisicao(`/functions/v1/corridas/${id}/mapa`, corpo), { ...base, repositorio: corridas }),
    encerrar: (id: number, corpo?: unknown) =>
      tratarCorridas(requisicao(`/functions/v1/corridas/${id}/encerrar`, corpo), { ...base, repositorio: corridas }),
  };
}

async function contar(db: PGlite, tabela: string, id: number): Promise<number> {
  const r = await db.query<{ n: number }>(`SELECT COUNT(*)::int AS n FROM ${tabela} WHERE corrida_id = $1`, [id]);
  return r.rows[0].n;
}

test("o lote do contrato com o firmware passa pela função e é gravado; o reenvio responde 200 sem duplicar", async () => {
  const db = await criarBanco();
  const r = robo(db);
  const criada = await r.criar("4x4");
  assert.equal(criada.status, 201);
  const id = (criada.corpo as Corrida).id;
  const lote = { ...fixture("lote-telemetria.json"), corrida_id: id };

  const primeiro = await r.enviar(lote);
  assert.equal(primeiro.status, 200);
  assert.deepEqual(primeiro.corpo, { gravadas: lote.amostras.length, existentes: 0, reconsolidada: false });

  const reenvio = await r.enviar(lote);
  assert.equal(reenvio.status, 200);
  assert.deepEqual(reenvio.corpo, { gravadas: 0, existentes: lote.amostras.length, reconsolidada: false });
  assert.equal(await contar(db, "telemetria", id), lote.amostras.length);
});

test("corrida completa pelas funções: cria, envia em lotes, reenvia, grava o mapa e encerra sem informar nada", async () => {
  const db = await criarBanco();
  const r = robo(db);
  const id = ((await r.criar("4x4")).corpo as Corrida).id;
  const amostras = fixture("lote-telemetria.json").amostras as unknown[];

  assert.equal((await r.enviar({ corrida_id: id, amostras: amostras.slice(0, 3) })).status, 200);
  assert.equal((await r.enviar({ corrida_id: id, amostras: amostras.slice(0, 3) })).status, 200); // resposta perdida, reenvio
  assert.equal((await r.enviar({ corrida_id: id, amostras: amostras.slice(3) })).status, 200);

  const mapa = await r.mapa(id, fixture("mapa-final-4x4.json"));
  assert.equal(mapa.status, 200);
  assert.deepEqual(mapa.corpo, { novas: 16, atualizadas: 0 });

  const encerrada = await r.encerrar(id); // sem corpo: o banco decide pela telemetria
  assert.equal(encerrada.status, 200);
  const corrida = encerrada.corpo as Corrida;
  assert.equal(corrida.status, "concluida"); // houve o evento objetivo_alcancado
  assert.equal(corrida.tempo_final, 4);
  assert.equal(corrida.velocidade_pico, 0.3);

  assert.equal(await contar(db, "telemetria", id), 5);
  assert.equal(await contar(db, "mapa_celulas", id), 16);
  const historico = await como(db, "anon", () => db.query<{ status: string }>("SELECT status FROM corridas WHERE id = $1", [id]));
  assert.equal(historico.rows[0].status, "concluida");
});

test("reenviar o mapa atualiza as células sem duplicar", async () => {
  const db = await criarBanco();
  const r = robo(db);
  const id = ((await r.criar("4x4")).corpo as Corrida).id;
  await r.mapa(id, fixture("mapa-final-4x4.json"));

  const segundo = await r.mapa(id, fixture("mapa-final-4x4.json"));

  assert.equal(segundo.status, 200);
  assert.deepEqual(segundo.corpo, { novas: 0, atualizadas: 16 });
  assert.equal(await contar(db, "mapa_celulas", id), 16);
});

test("encerrar de novo, sem status, devolve a corrida como está (idempotente)", async () => {
  const db = await criarBanco();
  const r = robo(db);
  const id = ((await r.criar("4x4")).corpo as Corrida).id;
  await r.enviar({ corrida_id: id, amostras: fixture("lote-telemetria.json").amostras });
  const primeira = (await r.encerrar(id)).corpo as Corrida;

  const segunda = await r.encerrar(id);

  assert.equal(segunda.status, 200);
  assert.deepEqual(segunda.corpo, primeira);
});

test("os erros do banco chegam ao robô com os códigos que ele classifica (404 e 400 descartam o lote)", async () => {
  const db = await criarBanco();
  const r = robo(db);
  const id = ((await r.criar("4x4")).corpo as Corrida).id;
  const amostra = { numero_sequencia: 1, posicao_linha: 0, posicao_coluna: 0, orientacao: "N" };

  const inexistente = await r.enviar({ corrida_id: 9999, amostras: [amostra] });
  assert.equal(inexistente.status, 404);
  assert.equal((inexistente.corpo as { erro: string }).erro, "corrida_inexistente");

  const foraDaMalha = await r.enviar({ corrida_id: id, amostras: [{ ...amostra, posicao_linha: 7 }] });
  assert.equal(foraDaMalha.status, 400);
  assert.equal((foraDaMalha.corpo as { erro: string }).erro, "amostras_invalidas");
  assert.equal(await contar(db, "telemetria", id), 0); // lote atômico: nada foi gravado

  const mapaInexistente = await r.mapa(9999, fixture("mapa-final-4x4.json"));
  assert.equal(mapaInexistente.status, 404);

  const celulaInvalida = await r.mapa(id, { celulas: [{ linha: 9, coluna: 0, parede_norte: false, parede_sul: false, parede_leste: false, parede_oeste: false }] });
  assert.equal(celulaInvalida.status, 400);
  assert.equal((celulaInvalida.corpo as { erro: string }).erro, "celulas_invalidas");
});

test("mapa com mais células do que o labirinto da corrida responde 413", async () => {
  const db = await criarBanco();
  const r = robo(db);
  const id = ((await r.criar("4x4")).corpo as Corrida).id;
  const celulas = Array.from({ length: 17 }, (_, i) => ({
    linha: Math.floor(i / 4) % 4,
    coluna: i % 4,
    parede_norte: false,
    parede_sul: false,
    parede_leste: false,
    parede_oeste: false,
  }));

  const resposta = await r.mapa(id, { celulas });

  assert.equal(resposta.status, 413);
  assert.equal((resposta.corpo as { erro: string }).erro, "mapa_grande_demais");
});

test("a telemetria só é aceita com o token do dispositivo", async () => {
  const db = await criarBanco();
  const { telemetria } = repositorios(db);
  const resposta = await tratarTelemetria(
    { metodo: "POST", caminho: "/functions/v1/telemetria", tokenRecebido: "errado", corpo: fixture("lote-telemetria.json") },
    { tokenEsperado: TOKEN, repositorio: telemetria },
  );
  assert.equal(resposta.status, 401);
});
