-- Realtime da tabela `corridas` (Issue #231 / HU-14).
--
-- O dashboard ao vivo assina `telemetria` (leituras) e `corridas` (largada, encerramento e
-- `tempo_final`). Só `telemetria` e `mapa_celulas` estavam na publicação do Realtime, então uma
-- corrida encerrada como `nao_concluida` antes dos 10:00 nunca chegava à tela. A guarda evita erro
-- se a tabela já estiver na publicação (por exemplo, adicionada pelo painel do Supabase).

DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1 FROM pg_publication_tables
    WHERE pubname = 'supabase_realtime' AND schemaname = 'public' AND tablename = 'corridas'
  ) THEN
    ALTER PUBLICATION supabase_realtime ADD TABLE public.corridas;
  END IF;
END
$$;
