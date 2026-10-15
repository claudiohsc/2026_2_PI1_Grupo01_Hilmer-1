-- Migração Inicial: Criação das tabelas centrais e seed de labirintos (Issue #228 / HU-09)
-- Tabelas: labirintos, corridas, telemetria, mapa_celulas

-- ------------------------------------------------------------------------------
-- 1. TABELA: labirintos
-- DESCRIÇÃO: Tabela de catálogo com as dimensões dos labirintos suportados.
-- PADRÕES DA COMPETIÇÃO: Formatos previstos no projeto (4x4, 8x4 e 12x4).
-- ------------------------------------------------------------------------------
CREATE TABLE labirintos (
    id SERIAL PRIMARY KEY,
    tipo VARCHAR(10) NOT NULL CHECK (tipo IN ('4x4', '8x4', '12x4')),
    linhas INT NOT NULL CHECK (linhas > 0),
    colunas INT NOT NULL CHECK (colunas > 0)
);

COMMENT ON TABLE labirintos IS 'Dimensões dos labirintos suportados no projeto';
COMMENT ON COLUMN labirintos.id IS 'Identificador único do labirinto (chave primária)';
COMMENT ON COLUMN labirintos.tipo IS 'Rótulo do labirinto (4x4, 8x4 ou 12x4)';
COMMENT ON COLUMN labirintos.linhas IS 'Quantidade total de linhas na matriz do labirinto';
COMMENT ON COLUMN labirintos.colunas IS 'Quantidade total de colunas na matriz do labirinto';

-- ------------------------------------------------------------------------------
-- 2. TABELA: corridas
-- DESCRIÇÃO: Registra cada tentativa de resolução do labirinto pelo micromouse.
-- CICLO DE VIDA:
--   - Criada pelo Frontend ou Backend ao iniciar uma tentativa ('em_andamento').
--   - Finalizada com métricas consolidadas ('concluida' ou 'nao_concluida').
-- ------------------------------------------------------------------------------
CREATE TABLE corridas (
    id SERIAL PRIMARY KEY,
    labirinto_id INT NOT NULL REFERENCES labirintos(id) ON DELETE RESTRICT,
    numero_tentativa INT NOT NULL DEFAULT 1,
    data_hora_inicio TIMESTAMPTZ NOT NULL DEFAULT now(),
    data_hora_fim TIMESTAMPTZ,
    tempo_final FLOAT,             -- Tempo total em segundos (s)
    velocidade_media FLOAT,        -- Velocidade média em metros por segundo (m/s)
    velocidade_pico FLOAT,         -- Velocidade máxima atingida em m/s
    consumo_bateria FLOAT,         -- Estimativa de consumo em mAh ou % consumida
    status VARCHAR(20) NOT NULL DEFAULT 'em_andamento' 
        CHECK (status IN ('em_andamento', 'concluida', 'nao_concluida'))
);

COMMENT ON TABLE corridas IS 'Histórico e métricas agregadas de cada tentativa de corrida';
COMMENT ON COLUMN corridas.id IS 'Identificador único da corrida (chave primária)';
COMMENT ON COLUMN corridas.labirinto_id IS 'FK para o labirinto utilizado; protegido contra deleção acidental';
COMMENT ON COLUMN corridas.numero_tentativa IS 'Número da tentativa de resolução do labirinto';
COMMENT ON COLUMN corridas.data_hora_inicio IS 'Instante de início da corrida registrado em UTC com timezone';
COMMENT ON COLUMN corridas.data_hora_fim IS 'Instante de término da corrida registrado em UTC com timezone (preenchido no encerramento)';
COMMENT ON COLUMN corridas.tempo_final IS 'Tempo total decorrido da corrida em segundos (s)';
COMMENT ON COLUMN corridas.velocidade_media IS 'Velocidade média calculada da corrida em m/s';
COMMENT ON COLUMN corridas.velocidade_pico IS 'Maior velocidade instantânea atingida pelo robô na corrida em m/s';
COMMENT ON COLUMN corridas.consumo_bateria IS 'Consumo estimado de bateria durante a corrida';
COMMENT ON COLUMN corridas.status IS 'Estado atual da corrida: em_andamento, concluida ou nao_concluida';

-- ------------------------------------------------------------------------------
-- 3. TABELA: telemetria
-- DESCRIÇÃO: Leituras periódicas geradas pelo firmware (ESP32-C3) via Wi-Fi.
-- DECISÕES DE PROJETO:
--   - Odometria diferencial (encoders) gera a orientação: N, S, L, O (sem IMU).
--   - CONSTRAINT uq_telemetria_corrida_sequencia: impede duplicações em retransmissões
--     de rede (buffer Wi-Fi / RF16) e suporta upsert com ON CONFLICT DO NOTHING.
-- ------------------------------------------------------------------------------
CREATE TABLE telemetria (
    id SERIAL PRIMARY KEY,
    corrida_id INT NOT NULL REFERENCES corridas(id) ON DELETE CASCADE,
    numero_sequencia INT NOT NULL,  -- Sequencial monotônico incremental por corrida
    timestamp TIMESTAMPTZ NOT NULL DEFAULT now(),
    posicao_linha INT NOT NULL,     -- Coordenada de linha atual na matriz (0 a N-1)
    posicao_coluna INT NOT NULL,    -- Coordenada de coluna atual na matriz (0 a M-1)
    orientacao VARCHAR(2) NOT NULL CHECK (orientacao IN ('N', 'S', 'L', 'O')),
    velocidade FLOAT NOT NULL DEFAULT 0.0,       -- Velocidade linear instantânea (m/s)
    tensao_bateria FLOAT NOT NULL DEFAULT 0.0,   -- Tensão instantânea dos packs (V)
    corrente_bateria FLOAT NOT NULL DEFAULT 0.0, -- Corrente consumida instantânea (A)
    tempo_decorrido FLOAT NOT NULL DEFAULT 0.0,  -- Tempo desde o início da corrida (s)
    estado_corrida VARCHAR(50),                  -- Ex.: 'explorando', 'retornando', 'parado'
    evento VARCHAR(30) CHECK (evento IS NULL OR evento IN ('colisao', 'objetivo_alcancado')),

    -- Restrição de unicidade para evitar duplicatas e viabilizar ingestão idempotente
    CONSTRAINT uq_telemetria_corrida_sequencia UNIQUE (corrida_id, numero_sequencia)
);

COMMENT ON TABLE telemetria IS 'Leituras em tempo real enviadas pelo robô durante a corrida';
COMMENT ON COLUMN telemetria.id IS 'Identificador único do registro de telemetria (chave primária)';
COMMENT ON COLUMN telemetria.corrida_id IS 'FK para a corrida à qual esta leitura pertence';
COMMENT ON COLUMN telemetria.numero_sequencia IS 'Contador sequencial monotônico para ordenação e controle de perdas na rede';
COMMENT ON COLUMN telemetria.timestamp IS 'Data e hora exata da leitura do sensor registrada em UTC com timezone';
COMMENT ON COLUMN telemetria.posicao_linha IS 'Linha atual estimada do robô na matriz do labirinto';
COMMENT ON COLUMN telemetria.posicao_coluna IS 'Coluna atual estimada do robô na matriz do labirinto';
COMMENT ON COLUMN telemetria.orientacao IS 'Orientação do robô calculada por odometria (Norte, Sul, Leste, Oeste)';
COMMENT ON COLUMN telemetria.velocidade IS 'Velocidade linear instantânea em m/s';
COMMENT ON COLUMN telemetria.tensao_bateria IS 'Tensão instantânea medida da bateria em Volts (V)';
COMMENT ON COLUMN telemetria.corrente_bateria IS 'Corrente instantânea drenada dos circuitos em Amperes (A)';
COMMENT ON COLUMN telemetria.tempo_decorrido IS 'Tempo total decorrido desde o início da corrida em segundos (s)';
COMMENT ON COLUMN telemetria.estado_corrida IS 'Estado da máquina de estados do firmware (ex.: explorando, parado)';
COMMENT ON COLUMN telemetria.evento IS 'Eventos pontuais críticos: colisao ou objetivo_alcancado (ou nulo)';

-- ------------------------------------------------------------------------------
-- 4. TABELA: mapa_celulas
-- DESCRIÇÃO: Mapa descoberto pelo algoritmo do robô célula a célula.
-- RELAÇÃO: Cada corrida possui seu conjunto de células mapeadas (N:1).
-- ------------------------------------------------------------------------------
CREATE TABLE mapa_celulas (
    id SERIAL PRIMARY KEY,
    corrida_id INT NOT NULL REFERENCES corridas(id) ON DELETE CASCADE,
    linha INT NOT NULL,
    coluna INT NOT NULL,
    parede_norte BOOLEAN,
    parede_sul BOOLEAN,
    parede_leste BOOLEAN,
    parede_oeste BOOLEAN,

    -- Cada célula de uma corrida específica só precisa de um registro de paredes
    CONSTRAINT uq_mapa_corrida_posicao UNIQUE (corrida_id, linha, coluna)
);

COMMENT ON TABLE mapa_celulas IS 'Paredes mapeadas pelo robô em cada célula durante a corrida';
COMMENT ON COLUMN mapa_celulas.id IS 'Identificador único do registro de célula mapeada (chave primária)';
COMMENT ON COLUMN mapa_celulas.corrida_id IS 'FK para a corrida correspondente';
COMMENT ON COLUMN mapa_celulas.linha IS 'Índice da linha da célula mapeada';
COMMENT ON COLUMN mapa_celulas.coluna IS 'Índice da coluna da célula mapeada';
COMMENT ON COLUMN mapa_celulas.parede_norte IS 'Presença de parede ao Norte (true = parede, false = livre, null = não explorada)';
COMMENT ON COLUMN mapa_celulas.parede_sul IS 'Presença de parede ao Sul (true = parede, false = livre, null = não explorada)';
COMMENT ON COLUMN mapa_celulas.parede_leste IS 'Presença de parede a Leste (true = parede, false = livre, null = não explorada)';
COMMENT ON COLUMN mapa_celulas.parede_oeste IS 'Presença de parede a Oeste (true = parede, false = livre, null = não explorada)';

-- ------------------------------------------------------------------------------
-- 5. SEED INICIAL DE LABIRINTOS
-- DESCRIÇÃO: Carga inicial dos labirintos conforme critério de aceite da Issue #228.
-- ------------------------------------------------------------------------------
INSERT INTO labirintos (tipo, linhas, colunas) VALUES 
    ('4x4', 4, 4),
    ('8x4', 8, 4),
    ('12x4', 12, 4);