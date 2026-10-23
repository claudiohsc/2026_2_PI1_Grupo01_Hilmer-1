# Status de Energia

**Gerente:** Paulo Henrique Melo de Souza. **Integrantes:** Daniel Rodrigues Nascimento e Elias Faria de Oliveira.

**Última atualização:** 23/10/2026 (Sprint 5). **Atualizado por:** Ana Carolina Fialho (gerente geral), só para registrar os PRs integrados até 23/10; a partir daqui, o gerente de energia.

**O que a equipe entrega:** o sistema de energia do robô (bateria LiPo 2S, distribuição e regulação, carregamento e proteção) e os testes de energia. A entrega no GitHub é [Sistema de Energia (#117)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/117). Detalhes em [Energia](../04-projeto-conceitual/energia.md) e [Proteções de entrada](../04-projeto-conceitual/protecoes-entrada.md).

## Onde está

A frente está na fase de dimensionamento, documentação e primeiros ensaios. Em 21/10 foi registrada a inspeção e carga da bateria em bancada ([#376](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/376)); os ensaios de subtensão e BMS ([#340](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/340)) são simulados. As demais tarefas de bancada, que dependem de componentes montados, estão abertas.

## Entregue

- Requisitos e perfil de consumo ([#195](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/195)), arquitetura preliminar e orçamento de componentes ([#196](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/196)).
- Dimensionamento da capacidade e da taxa C da LiPo 2S ([#198](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/198)), com autonomia estimada na página de [Energia](../04-projeto-conceitual/energia.md).
- Componentes dos reguladores MP1584EN e LM2596 ([#199](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/199)), incluindo o buck dedicado de 6 V para os motores (versão 1.4 da página).
- Proteção contra inversão de polaridade e sobrecorrente ([#200](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/200), PR #338 mesclado).
- Ensaios simulados de corte por subtensão e de BMS ([#340](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/340), [#207](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/207)) e bancada de ensaio com inspeção e carga da bateria ([#376](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/376), [#201](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/201)).
- EAP e cronograma de energia no GitHub Projects ([#197](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/197)).

## Em revisão (documentação aberta, sem revisão registrada)

| **PR** | **Assunto** | **Issue** |
|:------:|-------------|:---------:|
| [#352](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/352) | Curva de descarga da LiPo 2S e tabela de estado de carga | [#205](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/205) |
| [#353](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/353) | Divisor resistivo e filtro anti-aliasing para o ADC | [#203](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/203) |
| [#346](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/346) | Procedimento de recarga balanceada (POP) | [#210](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/210) |
| [#399](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/399) | Atualização do projeto conceitual de energia | não informada |
| [#312](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/312) | Dimensionamento dos reguladores step-down (base `main`) | não informada |

Os PRs #352 e #353 estão com base em `documentacao`, mas listam também arquivos de `src/firmware` e `src/backend`, o que indica branch com histórico misturado; vale conferir antes de mesclar.

## Em aberto (bancada e relatórios)

- **Bancada e montagem:** bancada de ensaio e baterias de teste ([#201](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/201)), bucks em protoboard ([#202](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/202)), eficiência e ripple ([#206](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/206)), calibração do divisor no ADC ([#208](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/208)), barramentos na PCB ([#212](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/212)) e energização inicial ([#213](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/213)).
- **Ensaios:** autonomia em bancada ([#209](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/209)), queda de tensão sob aceleração ([#214](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/214)), capacitores de *bulk* ([#216](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/216)), autonomia em pista ([#217](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/217)), corte por subtensão na pista ([#218](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/218)) e troca rápida de bateria ([#219](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/219)).
- **Relatórios:** testes de energia para a ED1 ([#211](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/211)), testes integrados para a ED2 ([#220](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/220)), balanço energético ([#222](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/222)) e packs para a apresentação ([#221](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/221)).
- Sem responsável no GitHub: #202, #213, #216, #218, #219 e #221.
- A tabela de [Testes de Energia](../07-testes/energia.md) tem as linhas 2.1 a 2.3 (subtensão e BMS, PPTC e bateria em bancada); as demais seguem vazias. É entrega da ED1 (26/10).

## Precisa ou aguarda de outras equipes

| **De** | **O quê** | **Dependência** |
|--------|-----------|:---------------:|
| Eletrônica | PCB física para soldar os barramentos e energizar a placa | D6 |
| Eletrônica | Esquemático alinhado ao buck de 6 V e ao divisor (R2 de 33 kΩ aqui; 39 kΩ no relatório de hardware de 10/10) | D5 |
| Estrutura | Labirinto de testes para os ensaios de autonomia em pista | D4 |
| Software | Tarefa de leitura da bateria no firmware e no dashboard ([#215](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/215) é de energia; não há *issue* correspondente em software) | D7 |

## O que outras equipes aguardam da energia

| **Quem** | **O quê** | **Dependência** |
|----------|-----------|:---------------:|
| Software | Curva de descarga e tabela de estado de carga (PR #352) para a telemetria de bateria | D7 |
| Estrutura | Dimensões da bateria e do suporte | D8 |

Veja a tabela completa em [Status das equipes](README.md#dependencias-entre-equipes).

## Fontes

[Entrega Sistema de Energia (#117)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/117) · [Energia](../04-projeto-conceitual/energia.md) · [Testes de energia](../07-testes/energia.md) · [Cronograma](../05-cronograma.md)

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 19/10/2026 | Levantamento inicial a partir do GitHub e dos documentos do projeto | Cláudio Henrique | Paulo Henrique |
| 1.1 | 23/10/2026 | Registro dos PRs #340 e #376 integrados e do primeiro ensaio em bancada | Ana Carolina Fialho | — |
