# Status de Eletrônica

**Gerente:** Aline de Carvalho Rodrigues. **Integrantes:** Bruno Bernardes Duarte, Eduardo Gaspar, Eliabe Alves, Giovanna da Costa e Jose Felipe Duarte Guedes de Oliveira.

**Última atualização:** 19/10/2026 (Sprint 4). **Atualizado por:** Cláudio Henrique (levantamento inicial a partir do GitHub e dos documentos; a partir daqui, a gerente de eletrônica).

**O que a equipe entrega:** o sistema eletrônico do robô (esquemático, circuito, PCB, integração dos componentes) e os testes e a validação do hardware. A entrega no GitHub é [Sistemas Eletrônicos (#79)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/79). Detalhes em [Hardware](../04-projeto-conceitual/hardware-diagramas.md) e [Justificativa dos componentes](../04-projeto-conceitual/justificativa-componentes.md).

## Onde está

| **Pacote de trabalho** | **Situação no GitHub** |
|------------------------|------------------------|
| [Seleção de Componentes (#80)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/80) | Fechado em 03/10 com todas as atividades |
| [Esquemático Elétrico (#87)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/87) | Aberto; esquemático em revisão no PR #311 |
| [Projeto e Montagem do Circuito (#93)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/93) | Aberto; nenhuma atividade filha fechada |
| [Integração dos Componentes (#98)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/98) | Aberto; nenhuma atividade filha fechada |
| [Testes de Hardware (#104)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/104) e [Validação do Hardware (#112)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/112) | Abertos; nenhuma atividade filha fechada |

## Entregue

- Projeto conceitual de hardware ([#161](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/161), fechado em 30/09) e seleção de componentes (microcontrolador, sensores, driver, motores e encoders), com a [justificativa dos componentes](../04-projeto-conceitual/justificativa-componentes.md) e os [diagramas de hardware](../04-projeto-conceitual/hardware-diagramas.md) publicados.
- Documento "Parâmetros de Hardware" de 17/10/2026, enviado à equipe de software e citado nas *issues* [#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341) e [#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344). Ele não está no repositório; a equipe de software usa os valores citados nessas *issues* (motor TT 1:48, encoder óptico de 400 pulsos por volta da roda, buck de 6 V para os motores, LiDAR que só transmite após comando de partida).

## Em revisão

- [PR #311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311): diagrama de blocos, esquemático elétrico (fonte KiCad e PDF) e relatório técnico de hardware (versão de 10/10). Há pedido de alterações de Paulo Henrique (energia). Enquanto não for mesclado, a página [4.3 Hardware](../04-projeto-conceitual/hardware.md) segue com o texto-modelo da disciplina.

## Em aberto técnico

Itens registrados no próprio relatório do PR #311:

- Conflito de GPIO: o encoder direito (GPIO4 e GPIO5) divide pinos com as entradas N1 e N2 do driver. O relatório diz que nenhum pino substituto foi escolhido.
- GPIO9 do encoder esquerdo participa da seleção de boot e exige validação.
- Níveis lógicos da UART do LiDAR e dos encoders (3,3 V ou 5 V) e uso da UART0 também para console e gravação.
- Correntes de partida e de bloqueio dos motores contra os limites do driver, e dissipação térmica.
- O relatório informa que os testes de integração listados nele **não foram executados**.

Na tabela de [Testes de Hardware](../07-testes/hardware.md), só a linha 3.5 (encoders) tem registro, feita pela equipe de software; as demais linhas estão vazias. Na tabela da Sprint 1 do [cronograma](../05-cronograma.md), a entrega do cronograma de hardware consta como não concluída.

## Precisa ou aguarda de outras equipes

| **De** | **O quê** | **Dependência** |
|--------|-----------|:---------------:|
| Energia | Alinhar o esquemático ao buck de 6 V e aos valores do divisor da bateria (R2 de 39 kΩ no relatório de hardware; 33 kΩ no dimensionamento da energia) | D5 |
| Estrutura | Definição dos suportes de PCB e sensores para fixar no chassi | D8 |
| Software | Pinagem usada no firmware (`Pinos.h`) para conferir com o esquemático | D2 |

## O que outras equipes aguardam da eletrônica

| **Quem** | **O quê** | **Dependência** |
|----------|-----------|:---------------:|
| Software | Protocolo do LiDAR e pinagem final | D1, D2 |
| Energia | PCB física montada para soldar os barramentos | D6 |
| Estrutura | Dimensões de PCB e sensores | D8 |

Veja a tabela completa em [Status das equipes](README.md#dependencias-entre-equipes).

## Fontes

[Entrega Sistemas Eletrônicos (#79)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/79) · [PR #311](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/311) · [Testes de hardware](../07-testes/hardware.md) · [Cronograma](../05-cronograma.md)
