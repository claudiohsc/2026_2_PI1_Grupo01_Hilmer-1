# _Backend_ - Persistência de Dados e API (Supabase)

Esta pasta armazena os esquemas de banco de dados, migrações SQL, políticas de segurança e configurações do backend do projeto Micromouse.

A arquitetura do backend utiliza o **Supabase (PostgreSQL gerenciado)**, aproveitando os recursos nativos de:
- **PostgREST**: Exposição automática de APIs REST a partir das tabelas.
- **Supabase Realtime**: Notificações e propagação de eventos via WebSockets para atualização do dashboard em tempo real.
- **Row Level Security (RLS)**: Controle granular de acesso para isolar a escrita do firmware e a leitura do frontend.

---

## Estrutura de Pastas

```text
src/backend/
├── .env.example                  # Modelo de variáveis de ambiente do Supabase
├── README.md                     # Este documento de instruções
└── supabase/
    └── migrations/
        ├── 01_esquema_inicial.sql # DDL das 4 tabelas, restrições e seed de labirintos
        └── 02_seguranca_rls.sql   # Políticas RLS e ativação do Realtime
```

---

## Variáveis de Ambiente

Para conectar o frontend ou o firmware ao Supabase, copie o arquivo de exemplo:

```bash
cp .env.example .env
```

Preencha as seguintes variáveis com as credenciais do seu projeto:

* `SUPABASE_URL`: URL base do projeto (encontrada em *Project Settings > API* no dashboard do Supabase).
* `SUPABASE_ANON_KEY`: Chave pública `anon` com acesso controlado por RLS.

> [!WARNING]
> **Nunca versione arquivos `.env` com chaves reais.** O `.env` já deve constar no `.gitignore`.

---

## Como Aplicar as Migrações no Supabase

As migrações devem ser executadas **na ordem numérica indicada**:

### 1. Criar o Projeto no Supabase
1. Acesse [supabase.com](https://supabase.com) e faça login.
2. Crie um novo projeto no plano gratuito (*Free Tier*).
3. Em *Project Settings > Members*, convide os membros da equipe de software.

### 2. Executar os Scripts SQL
No menu lateral esquerdo do dashboard do Supabase:
1. Abra o **SQL Editor**.
2. Crie uma nova query e cole o conteúdo de [`01_esquema_inicial.sql`](supabase/migrations/01_esquema_inicial.sql).
3. Clique em **Run** para criar as tabelas (`labirintos`, `corridas`, `telemetria`, `mapa_celulas`) e inserir o seed dos 3 labirintos padrão (4x4, 8x4 e 12x4).
4. Em seguida, cole o conteúdo de [`02_seguranca_rls.sql`](supabase/migrations/02_seguranca_rls.sql).
5. Clique em **Run** para habilitar o RLS, criar as políticas de acesso e ativar o Realtime na tabela `telemetria`.

---

## Testando o Upsert Idempotente de Telemetria

Para validar a restrição de unicidade `UNIQUE (corrida_id, numero_sequencia)` e o comportamento de *upsert* com `ON CONFLICT DO NOTHING`, execute no SQL Editor:

```sql
-- 1. Cria uma corrida de teste no labirinto 1 (4x4)
INSERT INTO corridas (id, labirinto_id, numero_tentativa, status)
VALUES (999, 1, 1, 'em_andamento')
ON CONFLICT (id) DO NOTHING;

-- 2. Primeira inserção do pacote de telemetria número 1
INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao)
VALUES (999, 1, 0, 0, 'N')
ON CONFLICT (corrida_id, numero_sequencia) DO NOTHING;

-- 3. Segunda inserção IDÊNTICA (simula retransmissão por instabilidade de Wi-Fi)
INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao)
VALUES (999, 1, 0, 0, 'N')
ON CONFLICT (corrida_id, numero_sequencia) DO NOTHING;

-- 4. Verificação: deve existir apenas 1 registro para a sequência 1
SELECT count(*) FROM telemetria WHERE corrida_id = 999 AND numero_sequencia = 1;

-- 5. Limpeza do teste
DELETE FROM corridas WHERE id = 999;
```

---

## Rastreabilidade
- **Issue**: #228 - Configurar o Supabase e criar o esquema do banco (DER)
- **História de Usuário Pai**: HU-09 - Persistência de Dados e Corridas
- **Requisitos Associados**: RF-8, RF-9, RF-14, RF-15, RNF-5