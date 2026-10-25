/// Validações comuns às Edge Functions. Sem dependência de Deno nem do Supabase, para rodar também
/// nos testes com Node.

export type Validacao<T> = { ok: true; valor: T } | { ok: false; mensagem: string };

/** `true` para um objeto JSON (não nulo e não lista). */
export function ehObjeto(corpo: unknown): corpo is Record<string, unknown> {
  return typeof corpo === "object" && corpo !== null && !Array.isArray(corpo);
}

/** Maior inteiro da coluna `id` (SERIAL) do banco. */
export const ID_MAXIMO = 2147483647;

/** `true` para um inteiro positivo que cabe na coluna `id` do banco. */
export function ehIdValido(valor: unknown): valor is number {
  return typeof valor === "number" && Number.isInteger(valor) && valor > 0 && valor <= ID_MAXIMO;
}

/** Identificador na URL: só dígitos, maior que zero e dentro do inteiro de 32 bits do banco. */
export function analisarId(texto: string): number | null {
  if (!/^[0-9]+$/.test(texto)) {
    return null;
  }
  const id = Number(texto);
  return ehIdValido(id) ? id : null;
}
