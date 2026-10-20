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

!!! note "Para quem lê (pessoas e IAs)"
    Tudo o que está escrito aqui vem de uma fonte citada: *issues* e PRs do GitHub, páginas desta documentação ou o cronograma. Quando uma informação **não consta** nas fontes, a página diz isso em vez de supor. Se algo parecer desatualizado, confira a data de "Última atualização" da página da equipe e os links antes de usar a informação.

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
