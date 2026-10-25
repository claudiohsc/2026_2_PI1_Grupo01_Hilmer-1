import test from "node:test";
import assert from "node:assert/strict";
import { analisarId, validarCriacao, validarEncerramento, validarMapa } from "./logica.ts";

test("criação aceita os três tipos de labirinto", () => {
  for (const tipo of ["4x4", "8x4", "12x4"]) {
    const r = validarCriacao({ labirinto: tipo });
    assert.deepEqual(r, { ok: true, valor: { tipo } });
  }
});

test("criação exige o labirinto", () => {
  for (const corpo of [{}, { labirinto: null }, { outro: 1 }]) {
    const r = validarCriacao(corpo);
    assert.equal(r.ok, false);
  }
});

test("criação recusa labirinto fora dos três tipos", () => {
  for (const tipo of ["3x3", "4X4", "", 4, true, ["4x4"], "4x4 "]) {
    assert.equal(validarCriacao({ labirinto: tipo }).ok, false, String(tipo));
  }
});

test("criação recusa corpo que não é objeto", () => {
  for (const corpo of [undefined, null, "4x4", 7, [], [{ labirinto: "4x4" }]]) {
    assert.equal(validarCriacao(corpo).ok, false);
  }
});

test("encerramento aceita os dois status finais, sem métricas", () => {
  assert.deepEqual(validarEncerramento({ status: "concluida" }), { ok: true, valor: { status: "concluida" } });
  assert.deepEqual(validarEncerramento({ status: "nao_concluida" }), { ok: true, valor: { status: "nao_concluida" } });
});

test("encerramento recusa status em andamento ou desconhecido", () => {
  for (const corpo of [{ status: "em_andamento" }, { status: "ok" }, { status: 1 }, { status: true }, { status: "" }]) {
    assert.equal(validarEncerramento(corpo).ok, false, JSON.stringify(corpo));
  }
});

test("encerramento sem status é aceito: o banco decide pela telemetria", () => {
  assert.deepEqual(validarEncerramento({}), { ok: true, valor: {} });
  assert.deepEqual(validarEncerramento(undefined), { ok: true, valor: {} });
  assert.deepEqual(validarEncerramento({ status: null }), { ok: true, valor: {} });
  assert.deepEqual(validarEncerramento({ tempo_final: 12.5 }), { ok: true, valor: { tempo_final: 12.5 } });
});

test("encerramento repassa as métricas numéricas informadas", () => {
  const r = validarEncerramento({
    status: "concluida",
    tempo_final: 42.5,
    velocidade_media: 0.31,
    velocidade_pico: 0.8,
    consumo_bateria: 0,
    campo_extra: "ignorado",
  });
  assert.deepEqual(r, {
    ok: true,
    valor: { status: "concluida", tempo_final: 42.5, velocidade_media: 0.31, velocidade_pico: 0.8, consumo_bateria: 0 },
  });
});

test("encerramento recusa métrica negativa, não numérica ou não finita", () => {
  for (const valor of [-1, "10", NaN, Infinity, true, {}]) {
    assert.equal(validarEncerramento({ status: "concluida", tempo_final: valor }).ok, false, String(valor));
  }
});

test("encerramento ignora métrica nula", () => {
  const r = validarEncerramento({ status: "concluida", tempo_final: null });
  assert.deepEqual(r, { ok: true, valor: { status: "concluida" } });
});

test("encerramento recusa corpo que não é objeto", () => {
  for (const corpo of [null, "concluida", 7, []]) {
    assert.equal(validarEncerramento(corpo).ok, false);
  }
});

test("identificador da corrida: inteiro positivo dentro do limite do banco", () => {
  assert.equal(analisarId("1"), 1);
  assert.equal(analisarId("2147483647"), 2147483647);
  for (const texto of ["0", "-1", "1.5", "abc", "", "2147483648", "1e3", " 1", "01a"]) {
    assert.equal(analisarId(texto), null, texto);
  }
});

test("mapa aceita um objeto com a lista de células e ignora os outros campos do MapaJson", () => {
  const celulas = [{ linha: 0, coluna: 0, parede_norte: false, parede_sul: true, parede_leste: false, parede_oeste: true }];
  assert.deepEqual(validarMapa({ dimensao: "4x4", linhas: 4, colunas: 4, celulas }), { ok: true, valor: { celulas } });
});

test("mapa recusa corpo que não é objeto ou sem lista de células", () => {
  for (const corpo of [undefined, null, "mapa", 7, [], { celulas: [] }, { celulas: "x" }, { celulas: {} }, {}]) {
    assert.equal(validarMapa(corpo).ok, false, JSON.stringify(corpo));
  }
});
