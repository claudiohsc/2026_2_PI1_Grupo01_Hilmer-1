# Status das equipes

Página de consulta rápida para saber **onde cada equipe está no projeto**, o que ela **entregou**, o que está **em andamento** e o que **precisa ou aguarda de outras equipes**. É uma visão geral: o detalhe técnico está nos documentos de cada frente e nas *issues* e *Pull Requests* do GitHub.

**Última atualização desta página:** 23/10/2026 (Sprint 5). Cada equipe tem a sua própria página, atualizada pelo respectivo gerente.

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
| Energia | Bateria 2S (LiPo nos documentos; a química ainda precisa ser confirmada pela energia), reguladores de tensão (*buck*), proteções e carregamento | Energia |
| Eletrônica | ESP32-C3 Super Mini, 3 sensores de distância VL53L0X e IMU MPU6050 no mesmo barramento I²C, driver de motores DRV8833 e dois motores N20 com encoders (Rev 2.0) | Eletrônica |
| Firmware | Código do ESP32-C3 em C++ (PlatformIO): percepção das paredes, mapa, navegação e controle dos motores | Software |
| Backend | Supabase: banco PostgreSQL e Edge Function que registra as corridas | Software |
| Frontend | Dashboard web em React, Vite e TypeScript, que lê os dados do Supabase | Software |

O fluxo previsto é: o robô percorre o labirinto, monta o mapa e envia a telemetria ao Supabase, e o dashboard exibe a corrida ao vivo e o histórico. O envio da telemetria pelo robô ainda está em desenvolvimento (Épico 03).

**Convenções do software:** a posição no labirinto é (linha, coluna), com a partida em (0, 0); Norte aumenta a linha e Leste aumenta a coluna. A dimensão do labirinto é escrita como "colunas × linhas" (por exemplo, 8x4 tem 8 colunas e 4 linhas). Uma **corrida** é uma tentativa de resolver o labirinto, registrada no banco com identificador único. Na Rev 2.0 do hardware (21/10), o robô usa a IMU MPU6050 para medir o giro ([#378](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/378)); os encoders dos motores N20 já estão disponíveis (GPIO4 a 7), mas, enquanto não são integrados ao firmware, o deslocamento é estimado pelo comando dos motores e corrigido pelos sensores ToF ([#380](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/380)).

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

Sprint atual: **Sprint 5 (22/10 a 28/10)**, com feriado em 28/10. A ED1 cai na segunda-feira desta sprint.

## Visão geral por equipe

| **Equipe** | **Onde está** | **Próxima entrega** | **Principal dependência em aberto** |
|------------|---------------|---------------------|-------------------------------------|
| Software | Mapa completo e navegação (resolução, odometria, centralização, objetivo e colisões) prontos; backend, frontend e histórico com dados reais prontos. Adaptação à Rev 2.0 em andamento: pinagem pronta ([#377](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/377)); driver dos VL53L0X, limiares para ToF, IMU e deslocamento sem encoder abertos ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344), [#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379), [#378](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/378), [#380](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/380)). | Testes isolados da Rev 2.0 ([#381](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/381), até 25/10) | Hardware Rev 2.0 montado para a bancada; pino do ADC da bateria |
| Eletrônica | Esquemático Rev 2.0, relatório técnico ([#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311)) e projeto conceitual de hardware ([#383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383), entregue com atraso por e-mail) integrados na `documentacao`. Nenhuma tarefa de montagem ou teste fechada no GitHub; a tabela de testes de hardware ainda descreve o projeto antigo (LiDAR, encoders). | Montagem do circuito e testes de bancada para a ED1 (7.3) | Definir o pino do ADC da bateria e confirmar os parâmetros do motor N20 |
| Energia | Dimensionamento documentado; ensaios simulados de subtensão e BMS ([#340](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/340)) e bancada de inspeção e carga da bateria ([#376](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/376)) integrados. PRs [#346](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/346), [#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352), [#353](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/353) e [#399](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/399) aguardando revisão. | Relatório de testes de energia para a ED1 ([#211](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/211)) | PCB física para soldar os barramentos ([#212](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/212)) |
| Estrutura | Projeto conceitual e CAD do chassi concluídos. CAD do labirinto (STL, 216 × 72 cm) integrado ([#337](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/337)); documentação e desenhos do labirinto em revisão ([#393](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/393)). Fabricação ainda não registrada como iniciada. | Tabela de testes de estrutura para a ED1 (7.1) e labirinto físico | Dimensões de PCB, sensores, bateria e motores |

## Dependências entre equipes

Cada linha diz quem precisa de quê e de quem. Quando uma dependência for resolvida, o gerente da equipe que **aguardava** atualiza a linha.

| **ID** | **Quem precisa** | **De quem** | **O quê** | **Referências** |
|:------:|------------------|-------------|-----------|-----------------|
| D1 | Software | Eletrônica | ~~Protocolo do LiDAR~~: deixou de valer com a Rev 2.0 (LiDAR trocado por 3 VL53L0X). Passa a ser: **hardware Rev 2.0 montado** para a bancada (3 VL53L0X, MPU6050, driver e motores N20) e os parâmetros do N20 (redução, corrente de travamento, tensão) | [#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175), [#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341), [#381](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/381) |
| D2 | Software e Eletrônica | Eletrônica | Pinagem: **resolvida na Rev 2.0**. `Pinos.h` ([#377](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/377), PR [#390](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/390)) e o esquemático Rev 2.0 ([#383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383)) usam I²C em GPIO9/8, XSHUT em GPIO0/1/2, motores em GPIO10/3/20/21 e encoders em GPIO4 a 7. Falta o pino do ADC da bateria (veja D7) | [#383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383), [#377](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/377) |
| D3 | Software | Estrutura | Diâmetro da roda e distância entre as rodas (bitola) definitivos. O firmware usa valores provisórios na odometria | [#187](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/187) |
| D4 | Software e Energia | Estrutura | Labirinto de testes físico montado, para a bancada dos sensores, a corrida completa e os ensaios de autonomia em pista. O CAD de 216 × 72 cm foi integrado ([#337](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/337)), mas a documentação em revisão ([#393](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/393)) descreve uma base de 720 × 720 mm; o tamanho precisa ser confirmado | [#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175), [#185](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/185), [#217](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/217) |
| D5 | Eletrônica e Energia | Eletrônica e Energia | Alinhar o esquemático com o dimensionamento de energia. Na Rev 2.0, o divisor da bateria usa R2 de 33 kΩ (igual à energia) e há buck de 6 V para os motores; falta confirmar a química da bateria (LiPo ou Li-ion) e o BMS no esquemático | [#383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383), [#353](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/353) |
| D6 | Energia | Eletrônica | PCB física montada para soldar os barramentos de alimentação e energizar a placa | [#212](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/212), [#213](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/213) |
| D7 | Software | Energia e Eletrônica | Telemetria de bateria no dashboard: curva de descarga e tabela de estado de carga ([#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352)) e leitura do ADC. Na Rev 2.0, o divisor termina em `BAT_ADC_TEST` sem GPIO, porque o GPIO2 ficou com o XSHUT direito; não há *issue* de software para a leitura da bateria | [#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352), [#215](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/215) |
| D8 | Estrutura | Eletrônica e Energia | Dimensões da PCB, dos sensores, da bateria e dos motores para modelar os suportes | [#276](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/276), [#277](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/277), [#278](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/278) |
| D9 | Software | Estrutura | Posição dos 3 VL53L0X no chassi (distância de cada sensor à borda e altura, que precisa ficar abaixo dos 50 mm das paredes do labirinto). É a base dos limiares de parede ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)) | [#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379), [#393](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/393) |

## Pull Requests abertos sem revisão ou com alterações pedidas

Situação em 23/10/2026, conforme o GitHub.

| **PR** | **Equipe** | **Assunto** | **Situação** |
|:------:|:----------:|-------------|--------------|
| [#393](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/393) | Estrutura | Documentação e desenhos do labirinto de testes | Sem revisão |
| [#391](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/391) | Software | Driver dos 3 VL53L0X ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344)) | Alterações pedidas por Cláudio Henrique |
| [#317](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/317) | Energia / Orçamento | Orçamento e plano de custos | Alterações pedidas por Cláudio Henrique |
| [#312](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/312) | Energia | Reguladores step-down (base `main`) | Sem revisão |
| [#346](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/346), [#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352), [#353](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/353), [#399](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/399) | Energia | POP de recarga, curva de descarga, divisor resistivo e atualização do conceitual de energia | Sem revisão |

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

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 19/10/2026 | Criação da seção e primeiro levantamento a partir do GitHub, do cronograma e dos documentos do projeto | Cláudio Henrique | Paulo Henrique |
| 1.1 | 23/10/2026 | Atualização para a Rev 2.0 do hardware (VL53L0X, MPU6050, N20, Super Mini), dependências D1 a D9 e situação dos PRs e das tarefas em 23/10 | Ana Carolina Fialho | — |
