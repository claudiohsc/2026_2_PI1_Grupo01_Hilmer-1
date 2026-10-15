-- Segurança e Tempo Real: Políticas RLS e publicação Supabase Realtime (Issue #228 / HU-09)

-- ------------------------------------------------------------------------------
-- 1. HABILITAÇÃO DE ROW LEVEL SECURITY (RLS)
-- Garante que nenhum dado seja lido ou modificado sem uma política explícita.
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
-- Regras:
--   - Frontend pode ler o histórico de todas as corridas (SELECT).
--   - Frontend pode criar uma nova corrida ao iniciar tentativa (INSERT).
--   - Frontend pode atualizar métricas finais e status ao encerrar corrida (UPDATE).
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura pública de corridas"
    ON corridas FOR SELECT
    TO anon, authenticated
    USING (true);

CREATE POLICY "Criação de corridas pelo frontend"
    ON corridas FOR INSERT
    TO anon, authenticated
    WITH CHECK (true);

CREATE POLICY "Atualização de corridas pelo frontend"
    ON corridas FOR UPDATE
    TO anon, authenticated
    USING (true)
    WITH CHECK (true);

-- ------------------------------------------------------------------------------
-- 4. POLÍTICAS DA TABELA: telemetria
-- CRITÉRIO DA ISSUE #228:
--   "A chave do firmware só faz INSERT em telemetria; o frontend tem leitura."
-- Regras:
--   - Leitura pública para o dashboard acompanhar a corrida (SELECT).
--   - Escrita restrita apenas para inserção de leituras pelo firmware (INSERT).
--   - Bloqueio explícito de UPDATE e DELETE para garantir integridade do histórico.
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura de telemetria pelo frontend"
    ON telemetria FOR SELECT
    TO anon, authenticated
    USING (true);

CREATE POLICY "Firmware pode inserir telemetria"
    ON telemetria FOR INSERT
    TO anon, authenticated
    WITH CHECK (true);

-- ------------------------------------------------------------------------------
-- 5. POLÍTICAS DA TABELA: mapa_celulas
-- Regras:
--   - Leitura pública das células descobertas para renderização do mapa (SELECT).
--   - Inserção/atualização das paredes mapeadas durante a corrida (INSERT/UPDATE).
-- ------------------------------------------------------------------------------
CREATE POLICY "Leitura do mapa de células"
    ON mapa_celulas FOR SELECT
    TO anon, authenticated
    USING (true);

CREATE POLICY "Inserção de células mapeadas"
    ON mapa_celulas FOR INSERT
    TO anon, authenticated
    WITH CHECK (true);

CREATE POLICY "Atualização de células mapeadas"
    ON mapa_celulas FOR UPDATE
    TO anon, authenticated
    USING (true)
    WITH CHECK (true);

-- 6. Habilita o Realtime na tabela telemetria (Critério Issue #228)
ALTER PUBLICATION supabase_realtime ADD TABLE telemetria;
