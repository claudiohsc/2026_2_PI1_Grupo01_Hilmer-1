/// <reference types="vite/client" />

/** Variáveis de ambiente lidas de `.env.local` (veja `.env.example`). */
interface ImportMetaEnv {
  readonly VITE_SUPABASE_URL?: string;
  readonly VITE_SUPABASE_ANON_KEY?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
