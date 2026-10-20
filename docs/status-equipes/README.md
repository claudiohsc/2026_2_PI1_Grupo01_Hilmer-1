# Status das equipes

Página de consulta rápida para saber **onde cada equipe está no projeto**, o que ela **entregou**, o que está **em andamento** e o que **precisa ou aguarda de outras equipes**. É uma visão geral: o detalhe técnico está nos documentos de cada frente e nas *issues* e *Pull Requests* do GitHub.

**Última atualização desta página:** 19/10/2026 (Sprint 4). Cada equipe tem a sua própria página, atualizada pelo respectivo gerente.

| **Equipe** | **Gerente** | **Página** |
|------------|-------------|:----------:|
| Software | Cláudio Henrique | [Status de Software](software.md) |
| Eletrônica (hardware) | Aline de Carvalho Rodrigues | [Status de Eletrônica](eletronica.md) |
| Energia | Paulo Henrique Melo de Souza | [Status de Energia](energia.md) |
| Estrutura | Ana Carolina Nunes | [Status de Estrutura](estrutura.md) |

A gerência geral do projeto é de Ana Carolina Fialho. A lista de integrantes do início do projeto está no [TAP](../01-tap.md); a gerência de software e a gerência geral mudaram depois dele.

!!! note "Como ler"
    Tudo o que está escrito aqui vem de uma fonte citada: *issues* e PRs do GitHub, páginas desta documentação ou o cronograma. Quando uma informação **não consta** nas fontes, a página diz isso em vez de supor. Se algo parecer desatualizado, confira a data de "Última atualização" da página da equipe e os links antes de usar a informação.

## Contexto geral do projeto

**Projeto:** Desenvolvimento de um Micromouse Autônomo com Sistema de Telemetria, da disciplina Projeto Integrador 1 (PI1) da FCTE/UnB, semestre 2026/2, Grupo 01, orientado pelo Prof. Dr. Hilmer Rodrigues Neri. Início em 07/09/2026 e término em 04/12/2026 ([TAP](../01-tap.md)).

**Objetivo:** construir um robô pequeno (o *micromouse*) que explora sozinho um labirinto desconhecido e chega à célula objetivo, no canto oposto ao início, sem controle remoto, sem intervenção humana e sem danificar a pista. Além do robô, o projeto tem uma interface web que mostra a telemetria em tempo real e um banco de dados que guarda as corridas para consulta posterior.

**Labirintos do desafio:**

| **Labirinto** | **Células** | **Dimensão** |
|:-------------:|:-----------:|:------------:|
| 1º | 4 × 4 | 72 × 72 cm |
| 2º | 8 × 4 | 144 × 72 cm |
| 3º | 12 × 4 | 216 × 72 cm |

**Restrições principais** ([Requisitos](../02-requisitos.md)): o robô não pode passar de 16,5 cm de comprimento ou largura; cada labirinto deve ser concluído em até 10 minutos; a bateria deve bastar para os três labirintos em sequência; a telemetria deve aparecer no sistema web em até 2 segundos; voo, salto, escalada e propulsão por combustão são proibidos.

**Como o sistema é formado:**

| **Parte** | **O que há nela** | **Equipe** |
|-----------|-------------------|:----------:|
| Estrutura | Chassi, sistema de tração, suportes e labirinto físico de testes | Estrutura |
| Energia | Bateria LiPo 2S, reguladores de tensão (*buck*), proteções e carregamento | Energia |
| Eletrônica | ESP32-C3, LiDAR por UART, dois encoders, driver de motores DRV8833 e dois motores DC | Eletrônica |
| Firmware | Código do ESP32-C3 em C++ (PlatformIO): percepção das paredes, mapa, navegação e controle dos motores | Software |
| Backend | Supabase: banco PostgreSQL e Edge Function que registra as corridas | Software |
| Frontend | Dashboard web em React, Vite e TypeScript, que lê os dados do Supabase | Software |

O fluxo previsto é: o robô percorre o labirinto, monta o mapa e envia a telemetria ao Supabase, e o dashboard exibe a corrida ao vivo e o histórico. O envio da telemetria pelo robô ainda está em desenvolvimento (Épico 03).

**Convenções do software:** a posição no labirinto é (linha, coluna), com a partida em (0, 0); Norte aumenta a linha e Leste aumenta a coluna. A dimensão do labirinto é escrita como "colunas × linhas" (por exemplo, 8x4 tem 8 colunas e 4 linhas). Uma **corrida** é uma tentativa de resolver o labirinto, registrada no banco com identificador único. O robô não usa IMU: a posição vem da odometria dos encoders.

**Termos usados nas páginas:**

| **Termo** | **Significado** |
|-----------|-----------------|
| ED1, ED2, APT | Entrega definitiva 1, entrega definitiva 2 e apresentação final |
| Sprint | Ciclo semanal de quinta a quarta-feira ([calendário](../05-cronograma.md#calendario-de-sprints)) |
| Épico, HU | Grupo de trabalho do software e história de usuário do [backlog](../04-projeto-conceitual/software.md) |
| RF, RNF | Requisito funcional e requisito não funcional |
| #123 | Número de uma *issue* ou PR em [github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer) |

**Onde fica cada coisa no repositório:**

| **Pasta** | **Conteúdo** |
|-----------|--------------|
| `docs/` | Esta documentação (MkDocs); alterações nela vão por PR para a branch `documentacao` |
| `src/firmware/` | Firmware do ESP32-C3 e seus testes |
| `src/backend/` | Supabase: migrações, funções SQL, Edge Function e testes |
| `src/frontend/` | Dashboard web |
| `hw/` e `mec/` | Arquivos de hardware (esquemáticos) e de mecânica |
| `scripts/` | Scripts de apoio, como o da curva de descarga da bateria |
| `.github/workflows/` | Verificações automáticas: CI do firmware e publicação da documentação |

Código vai por PR para a branch `main`; documentação vai por PR para a `documentacao`. Documentos de referência: [TAP](../01-tap.md), [Requisitos](../02-requisitos.md), [EAP](../03-eap.md), [Cronograma](../05-cronograma.md) e [Orçamento](../06-orcamento.md).

## Marcos do projeto

Datas do [cronograma](../05-cronograma.md):

| **Marco** | **Data** | **O que é avaliado** |
|-----------|:--------:|----------------------|
| ED1 - Entrega definitiva 1 | 26/10/2026 | Testes de estrutura, energia, hardware e software |
| ED2 - Entrega definitiva 2 | 23/11/2026 | Testes de integração |
| APT - Apresentação final | 02/12/2026 | Produto completo em funcionamento |

Sprint atual: **Sprint 4 (15/10 a 21/10)**. A Sprint 5 vai de 22/10 a 28/10, com feriado em 28/10.

## Visão geral por equipe

| **Equipe** | **Onde está** | **Próxima entrega** | **Principal dependência em aberto** |
|------------|---------------|---------------------|-------------------------------------|
| Software | Épico 01 (percepção e mapeamento) fechado, exceto o teste de bancada com o LiDAR real. Épico 02 (navegação) em andamento: algoritmo, odometria e centralização prontos. Base do Épico 03 (banco, Edge Function e frontend) em andamento. | Driver dos motores (#341), interface do LiDAR (#344) e execução dos movimentos (#184) | Protocolo do LiDAR e pinagem final, vindos da eletrônica; diâmetro da roda e bitola, vindos da estrutura |
| Eletrônica | Projeto conceitual e seleção de componentes concluídos. Esquemático e relatório técnico em revisão (PR #311). Nenhuma tarefa de montagem ou teste fechada no GitHub. | Aprovação do PR #311 e início da montagem do circuito | Resolver os conflitos de GPIO registrados no relatório do PR #311 |
| Energia | Dimensionamento da bateria, dos reguladores e das proteções documentado. Quatro PRs de documentação aguardando revisão. Resultados registrados até agora são de simulação. | Relatório técnico de testes de energia para a ED1 (#211) | PCB física para soldar os barramentos (#212) |
| Estrutura | Projeto conceitual e CAD do chassi concluídos. CAD do labirinto de testes em revisão (PR #337). Fabricação ainda não registrada como iniciada. | Fabricação do chassi e do labirinto de testes (cronograma até 23/10) | Dimensões de PCB, bateria e motores vindas de eletrônica e energia |

## Dependências entre equipes

Cada linha diz quem precisa de quê e de quem. Quando uma dependência for resolvida, o gerente da equipe que **aguardava** atualiza a linha.

| **ID** | **Quem precisa** | **De quem** | **O quê** | **Referências** |
|:------:|------------------|-------------|-----------|-----------------|
| D1 | Software | Eletrônica | Confirmar o protocolo do LiDAR: modelo exato (ST-L50B2 ou STL-50B2), bytes de partida e se o formato do pacote é o do LD06 | [#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344), [#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175) |
| D2 | Software e Eletrônica | Eletrônica | Fechar a pinagem. O firmware (`Pinos.h`) usa encoders em GPIO0, 1, 3 e 10 e o driver em GPIO4 a 7. O relatório do PR #311 usa encoder direito em GPIO4 e 5 e esquerdo em GPIO9 e 10, e registra conflito com o driver | [#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311), [#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341) |
| D3 | Software | Estrutura | Diâmetro da roda e distância entre as rodas (bitola) definitivos. O firmware usa valores provisórios na odometria | [#187](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/187) |
| D4 | Software e Energia | Estrutura | Labirinto de testes físico montado, para o teste do LiDAR real, a corrida completa e os ensaios de autonomia em pista | [#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175), [#185](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/185), [#217](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/217) |
| D5 | Eletrônica e Energia | Eletrônica e Energia | Alinhar o esquemático com o dimensionamento de energia: buck de 6 V para os motores (energia v1.4) e valores do divisor da bateria (R2 de 33 kΩ na energia, 39 kΩ no relatório de hardware de 10/10) | [#353](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/353), [#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311) |
| D6 | Energia | Eletrônica | PCB física montada para soldar os barramentos de alimentação e energizar a placa | [#212](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/212), [#213](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/213) |
| D7 | Software | Energia | Telemetria de bateria no dashboard: curva de descarga e tabela de estado de carga (PR #352) e leitura do ADC. Não há *issue* de software aberta para a leitura da bateria no firmware | [#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352), [#215](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/215) |
| D8 | Estrutura | Eletrônica e Energia | Dimensões da PCB, dos sensores, da bateria e dos motores para modelar os suportes | [#276](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/276), [#277](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/277), [#278](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/278) |

## Pull Requests abertos sem revisão ou com alterações pedidas

Situação em 19/10/2026, conforme o GitHub.

| **PR** | **Equipe** | **Assunto** | **Situação** |
|:------:|:----------:|-------------|--------------|
| [#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311) | Eletrônica | Diagramas, esquemático KiCad e relatório técnico | Alterações pedidas por Paulo Henrique |
| [#317](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/317) | Energia / Orçamento | Orçamento e plano de custos | Alterações pedidas por Cláudio Henrique |
| [#312](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/312) | Energia | Reguladores step-down | Sem revisão |
| [#337](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/337) | Estrutura | CADs do labirinto de testes | Sem revisão |
| [#340](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/340), [#346](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/346), [#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352), [#353](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/353) | Energia | Ensaios de proteção, POP de recarga, curva de descarga e divisor resistivo | Sem revisão |

As páginas de cada equipe listam também os PRs de software em revisão.

## Como manter esta página atualizada

1. **Quem atualiza:** o gerente de cada equipe edita **somente a página da sua equipe**. O gerente geral mantém esta página inicial (marcos, visão geral, dependências e PRs).
2. **Quando:** no primeiro dia de cada sprint (planning) e sempre que uma entrega, um bloqueio ou uma dependência mudar. Atualize a data de "Última atualização" da sua página.
3. **Como:** abra um PR para a branch `documentacao`, com título `docs(status): atualiza o status de <equipe>`, e peça revisão do gerente geral.
4. **O que escrever:**
    - Use só o que está em *issues*, PRs, no [cronograma](../05-cronograma.md) ou nos documentos do projeto, e cite o link.
    - Se uma informação não existe nas fontes, escreva **"não consta"**. Não estime nem complete por conta própria.
    - Mantenha a página curta: nível de visão geral, sem detalhe de implementação.
    - Percentuais e status do cronograma ficam no [cronograma](../05-cronograma.md); aqui vale o que está no GitHub.
5. **Dependências:** ao abrir uma dependência nova, acrescente uma linha na tabela acima (com o próximo ID livre). Ao resolvê-la, ajuste o texto da linha em vez de apagá-la, até o fim da sprint.

## Histórico de versão

| **Versão** | **Data** | **Descrição** | **Autor** | **Revisor** |
|:----------:|:--------:|---------------|-----------|-------------|
| 1.0 | 19/10/2026 | Criação da seção e primeiro levantamento a partir do GitHub, do cronograma e dos documentos do projeto | Cláudio Henrique | Revisão no PR de criação |
