# _Frontend_ - Dashboard Web do Micromouse

Esta pasta armazena o dashboard web do micromouse (HU-14): a tela de **telemetria ao vivo** e a tela de **histórico de consultas**, conforme o [protótipo navegável no Figma](https://www.figma.com/proto/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121&p=f&scaling=min-zoom&content-scaling=fixed&page-id=0%3A1&starting-point-node-id=7%3A121).

Tecnologias: **React 18** + **Vite** + **TypeScript**, **Recharts** para gráficos, **supabase-js** para ler o banco e CSS simples (sem framework de estilos).

> [!NOTE]
> Nesta etapa (#230) as telas usam **dados de exemplo** no formato das tabelas do Supabase. A única leitura real é a lista de `labirintos`, que confirma a conexão. Os dados reais entram nas HUs 09, 10 e 15 (histórico) e 08, 16 e 06 (telemetria ao vivo).

---

## Como rodar

Requer **Node.js 18 ou mais recente** (com `npm`).

```bash
cd src/frontend
npm install
cp .env.example .env.local   # depois preencha com os dados do Supabase
npm run dev                  # abre em http://localhost:5173
```

| Comando | O que faz |
|---|---|
| `npm run dev` | Servidor de desenvolvimento com recarga automática, em `http://localhost:5173`. |
| `npm run build` | Confere os tipos (`tsc --noEmit`) e gera a versão final em `dist/`. |
| `npm run preview` | Serve a pasta `dist/` para conferir o build. |

Sem o `.env.local` o dashboard funciona do mesmo jeito, só com os dados de exemplo; o rodapé avisa "Supabase não configurado".

---

## Variáveis de ambiente

Copie `.env.example` para `.env.local` e preencha (valores em *Project Settings > API* no Supabase):

* `VITE_SUPABASE_URL`: URL base do projeto.
* `VITE_SUPABASE_ANON_KEY`: chave pública `anon`.

> [!WARNING]
> **O `.env.local` nunca vai para o Git** (está no `.gitignore`). Tudo o que começa com `VITE_` é embutido no código que roda no navegador, então aqui entra **só a chave `anon`**, que pelas políticas de RLS apenas lê. A chave `service_role` e o token do dispositivo nunca devem aparecer no frontend.

Com tudo certo, o rodapé mostra "Supabase conectado: labirintos lidos do banco".

---

## Estrutura de pastas

```text
src/frontend/
├── .env.example               # Modelo das variáveis de ambiente
├── index.html                 # Página base do Vite
├── package.json               # Dependências e scripts
├── tsconfig.json              # Configuração do TypeScript
├── vite.config.ts             # Configuração do Vite
├── public/
│   └── favicon.svg
└── src/
    ├── main.tsx               # Ponto de entrada
    ├── App.tsx                # Cabeçalho, abas e rodapé; guarda aba, labirinto e tentativa
    ├── tipos.ts               # Tipos das tabelas do Supabase (mesmos nomes de colunas)
    ├── index.css              # Cores e estilos base
    ├── componentes/           # Cabecalho, GradeLabirinto, CartaoMetrica, BadgeDesafio,
    │                          # GraficoVelocidade, FiltroLabirinto, TabelaHistorico (+ .css)
    ├── paginas/               # TelaAoVivo (Frames 01–03) e TelaHistorico (Frame 04)
    ├── hooks/                 # useLabirintos (leitura no Supabase) e useReproducaoCorrida
    ├── lib/                   # supabase.ts (cliente), formatacao.ts, labirinto.ts
    └── dados/                 # Dados de exemplo: labirintos, corrida e histórico
```

---

## Telas

### Telemetria ao Vivo (Frames 01, 02 e 03)

* **Mapa** (`GradeLabirinto`): malha escura, largada verde no canto inferior esquerdo, objetivo vermelho no canto oposto, paredes já mapeadas, trajeto laranja e o robô com a orientação. A célula (0, 0) é a largada; Norte aumenta a linha e Leste aumenta a coluna, como no firmware.
* **Cards**: bateria (% estimado pela tensão, tensão e corrente), velocidade média e pico, cronômetro com o limite de 10:00 e a tentativa (1 de 3).
* **Badge** no cabeçalho: `DESAFIO CUMPRIDO: NÃO`, `NÃO (EM ANDAMENTO)` ou `SIM`.
* **Botão**: `INICIAR CORRIDA` → `CORRIDA EM ANDAMENTO` → `RESETAR / NOVA TENTATIVA`.
* **Gráfico de exemplo** (Recharts): velocidade × tempo da tentativa.

Ao clicar em `INICIAR CORRIDA`, a corrida de exemplo é reproduzida 4× mais rápido que o tempo real, passando pelos três estados. Os labirintos de exemplo são os mesmos de `src/firmware/test/test_labirinto_simulado/LabirintosExemplo.h`, e o trajeto é gerado respeitando as paredes. Nesta etapa os botões **não** acionam o robô nem gravam no banco; isso entra com a HU-15 e a Edge Function de corridas (#233).

### Histórico de Consultas (Frame 04 e filtros M1–M3)

Tabela com Data/Hora, Labirinto, Tentativa (com a nota: 1ª = 10, 2ª = 7,5, 3ª = 5), Tempo final, Velocidade média, Bateria e Status, da corrida mais recente para a mais antiga, com o filtro `Todos os Labirintos` / `Labirinto 1 (4×4)` / `Labirinto 2 (8×4)` / `Labirinto 3 (12×4)`.

---

## Pontos em aberto

* **Unidade de `corridas.consumo_bateria`**: a migração diz "mAh ou % consumida". O histórico assume **% consumida** e mostra a carga restante (`100 − consumo`). Precisa ser fechado na HU-09.
* **Percentual da bateria**: estimado de forma linear entre 6,6 V (corte) e 8,4 V (carga plena) da LiPo 2S. A curva real não é linear; o cálculo final é da HU-08.
