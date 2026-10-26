# Status de Eletrônica

**Gerente:** Aline de Carvalho Rodrigues. **Integrantes:** Bruno Bernardes Duarte, Eduardo Gaspar, Eliabe Alves, Giovanna da Costa e Jose Felipe Duarte Guedes de Oliveira.

**Última atualização:** 23/10/2026 (Sprint 5). **Atualizado por:** Ana Carolina Fialho (gerente geral), para registrar a Rev 2.0 e os PRs integrados; a partir daqui, a gerente de eletrônica.

**O que a equipe entrega:** o sistema eletrônico do robô (esquemático, circuito, PCB, integração dos componentes) e os testes e a validação do hardware. A entrega no GitHub é [Sistemas Eletrônicos (#79)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/79). Detalhes em [Hardware](../04-projeto-conceitual/hardware.md), [Diagramas de hardware](../04-projeto-conceitual/hardware-diagramas.md) e [Justificativa dos componentes](../04-projeto-conceitual/justificativa-componentes.md).

## Onde está

| **Pacote de trabalho** | **Situação no GitHub** |
|------------------------|------------------------|
| [Seleção de Componentes (#80)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/80) | Fechado em 03/10 com todas as atividades |
| [Esquemático Elétrico (#87)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/87) | Aberto; o esquemático **Rev 2.0** (21/10) foi integrado ([#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311) e [#383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383)) |
| [Projeto e Montagem do Circuito (#93)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/93) | Aberto; nenhuma atividade filha fechada |
| [Integração dos Componentes (#98)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/98) | Aberto; nenhuma atividade filha fechada |
| [Testes de Hardware (#104)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/104) e [Validação do Hardware (#112)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/112) | Abertos; nenhuma atividade filha fechada |

## Entregue

- Projeto conceitual de hardware ([#161](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/161)) e seleção de componentes, com a [justificativa dos componentes](../04-projeto-conceitual/justificativa-componentes.md).
- **Rev 2.0 do hardware (21/10/2026):** ESP32-C3 Super Mini, 3 sensores VL53L0X e IMU MPU6050 no mesmo I²C, driver DRV8833, motores N20, fusível PPTC MF-R185, chave geral e 3 reguladores MP1584EN (3,3 V, 5 V e 6 V). Os encoders dos motores N20 já estão disponíveis, nos GPIO4 e 5 (esquerdo) e GPIO6 e 7 (direito); falta integrá-los ao firmware.
- Diagramas, esquemático (fonte KiCad e PDF) e relatório técnico ([#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311), integrado em 21/10) e a página [4.3 Hardware](../04-projeto-conceitual/hardware.md) com o projeto conceitual na Rev 2.0 ([#383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383), integrado em 22/10 e enviado ao professor com atraso).
- Documento "Parâmetros de Hardware" de 17/10/2026, citado nas *issues* [#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341) e [#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344). Ele não está no repositório, e parte dos valores (motor TT, encoder óptico, LiDAR) deixou de valer com a Rev 2.0.

## Em revisão

Não consta PR da eletrônica aberto em 23/10.

## Em aberto técnico

Itens registrados nas notas do esquemático Rev 2.0 e nas revisões dos PRs:

- **ADC da bateria:** o divisor (R1 100 kΩ, R2 33 kΩ) termina em `BAT_ADC_TEST`, sem GPIO; o GPIO2, que tem ADC, ficou com o XSHUT do sensor direito.
- **Pinos sensíveis no boot:** GPIO2, GPIO8 e GPIO9 são pinos de *strapping*; GPIO20/21 (UART0) acionam o motor direito e podem dar tranco no boot. O próprio esquemático pede validação.
- **Motor N20:** redução, corrente de travamento e fornecedor pendentes.
- **Bateria e BMS:** a química (LiPo ou Li-ion) e o BMS no esquemático, apontados na revisão do [#311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311), seguem pendentes.
- Na página [4.3 Hardware](../04-projeto-conceitual/hardware.md), a imagem `hardware-diagrama-blocos.jpg` que continua no repositório é a versão antiga (com LiDAR); a página usa o diagrama Rev 2.0 em Mermaid.

Na tabela de [Testes de Hardware](../07-testes/hardware.md), as linhas ainda descrevem o projeto antigo (3.2 LiDAR por UART, 3.5 encoders); só a 3.5 tem registro, feito pela equipe de software. A tabela é entrega da ED1 (26/10).

## Precisa ou aguarda de outras equipes

| **De** | **O quê** | **Dependência** |
|--------|-----------|:---------------:|
| Energia | Confirmar a química da bateria e o BMS para o esquemático (o divisor já usa R2 de 33 kΩ, como a energia) | D5 |
| Estrutura | Definição dos suportes de PCB, dos 3 VL53L0X e da MPU6050 para fixar no chassi | D8 |
| Software | Pinagem do firmware (`Pinos.h`) conferida com o esquemático Rev 2.0 | D2 (resolvida) |

## O que outras equipes aguardam da eletrônica

| **Quem** | **O quê** | **Dependência** |
|----------|-----------|:---------------:|
| Software | Hardware Rev 2.0 montado para a bancada, parâmetros do motor N20 e pino do ADC da bateria | D1, D7 |
| Energia | PCB física montada para soldar os barramentos | D6 |
| Estrutura | Dimensões de PCB e sensores | D8 |

Veja a tabela completa em [Status das equipes](README.md#dependencias-entre-equipes).

## Fontes

[Entrega Sistemas Eletrônicos (#79)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/79) · [PR #311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311) · [PR #383](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/383) · [Testes de hardware](../07-testes/hardware.md) · [Cronograma](../05-cronograma.md)

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 19/10/2026 | Levantamento inicial a partir do GitHub e dos documentos do projeto | Cláudio Henrique | Paulo Henrique |
| 1.1 | 23/10/2026 | Registro da Rev 2.0 do hardware, dos PRs #311 e #383 integrados e das pendências técnicas do esquemático | Ana Carolina Fialho | — |
