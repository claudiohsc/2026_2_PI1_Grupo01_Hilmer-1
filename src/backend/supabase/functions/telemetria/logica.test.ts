import test from "node:test";
import assert from "node:assert/strict";
import { MAXIMO_AMOSTRAS_POR_LOTE, validarLote } from "./logica.ts";

const amostra = (n: number) => ({ numero_sequencia: n, posicao_linha: 0, posicao_coluna: 0, orientacao: "N" });

test("lote válido devolve a corrida e as amostras como vieram", () => {
  const amostras = [amostra(1), amostra(2)];
  assert.deepEqual(validarLote({ corrida_id: 7, amostras, campo_extra: "ignorado" }), {
    ok: true,
    valor: { corrida_id: 7, amostras },
  });
});

test("lote aceita de 1 a 100 amostras", () => {
  assert.equal(validarLote({ corrida_id: 1, amostras: [amostra(1)] }).ok, true);
  const cheio = Array.from({ length: MAXIMO_AMOSTRAS_POR_LOTE }, (_, i) => amostra(i + 1));
  assert.equal(validarLote({ corrida_id: 1, amostras: cheio }).ok, true);
});

test("lote com mais de 100 amostras é recusado", () => {
  const demais = Array.from({ length: MAXIMO_AMOSTRAS_POR_LOTE + 1 }, (_, i) => amostra(i + 1));
  const r = validarLote({ corrida_id: 1, amostras: demais });
  assert.equal(r.ok, false);
});

test("lote recusa corpo que não é objeto", () => {
  for (const corpo of [undefined, null, "lote", 7, [], [{ corrida_id: 1 }]]) {
    assert.equal(validarLote(corpo).ok, false);
  }
});

test("lote exige corrida_id inteiro positivo dentro do limite do banco", () => {
  assert.equal(validarLote({ amostras: [amostra(1)] }).ok, false);
  assert.equal(validarLote({ corrida_id: null, amostras: [amostra(1)] }).ok, false);
  for (const id of [0, -1, 1.5, "7", NaN, Infinity, true, 2147483648, {}]) {
    assert.equal(validarLote({ corrida_id: id, amostras: [amostra(1)] }).ok, false, String(id));
  }
  assert.equal(validarLote({ corrida_id: 2147483647, amostras: [amostra(1)] }).ok, true);
});

test("lote exige uma lista não vazia de amostras que sejam objetos", () => {
  for (const amostras of [undefined, null, [], "x", 3, {}, [1], [null], [[]], [amostra(1), "x"]]) {
    assert.equal(validarLote({ corrida_id: 1, amostras }).ok, false, JSON.stringify(amostras));
  }
});
