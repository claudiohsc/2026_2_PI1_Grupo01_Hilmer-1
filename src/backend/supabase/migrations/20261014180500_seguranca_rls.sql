-- Segurança e Tempo Real: Políticas RLS e publicação Supabase Realtime (Issue #228 / HU-09)

-- ------------------------------------------------------------------------------
-- 1. HABILITAÇÃO DE ROW LEVEL SECURITY (RLS)
-- Garante bloqueio total por padrão em todas as tabelas do sistema.
-- ------------------------------------------------------------------------------
ALTER TABLE labirintos ENABLE ROW LEVEL SECURITY;
ALTER TABLE corridas ENABLE ROW LEVEL SECURITY;
ALTER TABLE telemetria ENABLE ROW LEVEL SECURITY;
ALTER TABLE mapa_celulas ENABLE ROW LEVEL SECURITY;

-- ------------------------------------------------------------------------------
-- 2. POLÍTICAS DA TABELA: labirintos
-- Regra: Leitura pública para o frontend carregar os grids 4x4, 8x4 e 12x4.
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura pública de labirintos"
    ON labirintos FOR SELECT
    TO anon, authenticated
    USING (true);

-- ------------------------------------------------------------------------------
-- 3. POLÍTICAS DA TABELA: corridas
-- Regra: Leitura pública do histórico de corridas pelo dashboard web.
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura pública de corridas"
    ON corridas FOR SELECT
    TO anon, authenticated
    USING (true);

-- ------------------------------------------------------------------------------
-- 4. POLÍTICAS DA TABELA: telemetria
-- Regra: Leitura pública para o dashboard acompanhar a corrida em tempo real.
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura de telemetria pelo frontend"
    ON telemetria FOR SELECT
    TO anon, authenticated
    USING (true);

-- ------------------------------------------------------------------------------
-- 5. POLÍTICAS DA TABELA: mapa_celulas
-- Regra: Leitura pública das células descobertas para renderização da grade no dashboard.
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura do mapa de células"
    ON mapa_celulas FOR SELECT
    TO anon, authenticated
    USING (true);

-- ------------------------------------------------------------------------------
-- 6. PUBLICAÇÃO SUPABASE REALTIME
-- ------------------------------------------------------------------------------
ALTER PUBLICATION supabase_realtime ADD TABLE telemetria;
ALTER PUBLICATION supabase_realtime ADD TABLE mapa_celulas;
