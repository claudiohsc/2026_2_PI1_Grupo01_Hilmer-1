import { createClient } from '@supabase/supabase-js';

const url = import.meta.env.VITE_SUPABASE_URL;
const chaveAnon = import.meta.env.VITE_SUPABASE_ANON_KEY;

/**
 * Cliente do Supabase, criado a partir de `.env.local` (veja `.env.example`).
 *
 * Usa só a chave pública (`anon`): pelas políticas de RLS (#228) ela apenas lê
 * `labirintos`, `corridas`, `telemetria` e `mapa_celulas`. Gravações do robô passam
 * pelas Edge Functions (#233), nunca pelo navegador.
 *
 * Fica `null` quando as variáveis não estão definidas. Nesse caso o dashboard continua
 * funcionando com os dados de exemplo e avisa no rodapé.
 */
export const supabase = url && chaveAnon ? createClient(url, chaveAnon) : null;
