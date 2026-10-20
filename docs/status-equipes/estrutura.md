# Status de Estrutura

**Gerente:** Ana Carolina Nunes. **Integrantes:** Ágata Dias Vieira, Joao Artur de Andrades e Letícia Lima Dos Santos.

**Última atualização:** 19/10/2026 (Sprint 4). **Atualizado por:** Cláudio Henrique (levantamento inicial a partir do GitHub e dos documentos; a partir daqui, a gerente de estrutura).

**O que a equipe entrega:** o chassi e o sistema de tração, os suportes de integração com a eletrônica e a energia, e o labirinto físico de testes. A entrega no GitHub é [Sistema de Estrutura (#121)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/121). Detalhes em [Estruturas](../04-projeto-conceitual/estruturas.md).

## Onde está

| **Frente** | **Situação** |
|------------|--------------|
| Sistema de Tração ([#61](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/61)) | CAD e seleção de materiais concluídos no cronograma; desenho técnico e dimensionamento em andamento; fabricação prevista até 20/10 |
| Estrutura Mecânica ([#53](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/53)) | CAD concluído; desenho técnico, análise e seleção de materiais em andamento; prototipagem e fabricação ainda sem início registrado |
| Labirinto de Teste ([#155](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/155)) | CAD do labirinto em revisão (PR #337); construção física prevista de 21/10 a 23/10 |
| Integração ([#68](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/68)) | Projeto de suportes e CAD concluídos; análise em andamento; montagem, validação e testes previstos para novembro (ED2) |

Os percentuais acima vêm do [cronograma](../05-cronograma.md) e podem estar defasados; a data de cada atualização não consta nele.

## Entregue

- Projeto conceitual de estruturas ([#162](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/162), fechado em 30/09), em [Estruturas](../04-projeto-conceitual/estruturas.md), com os CADs do chassi na pasta do projeto conceitual.
- Modelagem CAD do chassi, do sistema de tração e do projeto de suportes (100% no cronograma).

## Em revisão

- [PR #337](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/337), "Adiciona CADs do labirinto de testes": arquivos STL da base e das paredes. O modelo é de 216 cm por 72 cm, com encaixes para as paredes, de modo que também simula os labirintos menores. Sem revisão registrada.

## Em aberto

- **Tarefas no GitHub:** há 45 tarefas abertas ([#246](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/246) a [#290](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/290)) e nenhuma fechada. Cobrem rodas e suportes dos motores, torque, materiais, chassi, labirinto de testes, suportes de PCB, sensores e bateria, montagem e testes. As tarefas #260 e #261 têm o mesmo título ("Desenho técnico do chassi base").
- **Fabricação:** chassi e peças previstas até 20/10 no cronograma.
- **Testes de tração e motores:** previstos de 21/10 a 23/10 (ED1).
- **Testes de estrutura:** a tabela de [Testes de Estrutura](../07-testes/estrutura.md) está vazia na versão publicada, e é entrega da ED1 (26/10).

## Precisa ou aguarda de outras equipes

| **De** | **O quê** | **Dependência** |
|--------|-----------|:---------------:|
| Eletrônica | Dimensões de PCB e sensores, e modelo definitivo dos motores, para modelar os suportes e calcular o torque | D8 |
| Energia | Dimensões da bateria para o suporte | D8 |

## O que outras equipes aguardam da estrutura

| **Quem** | **O quê** | **Dependência** |
|----------|-----------|:---------------:|
| Software | Diâmetro da roda e bitola definitivos; labirinto físico para o teste do LiDAR real e a corrida completa | D3, D4 |
| Energia | Labirinto físico para os ensaios de autonomia em pista | D4 |

Veja a tabela completa em [Status das equipes](README.md#dependencias-entre-equipes).

## Fontes

[Entrega Sistema de Estrutura (#121)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/121) · [Estruturas](../04-projeto-conceitual/estruturas.md) · [Testes de estrutura](../07-testes/estrutura.md) · [Cronograma](../05-cronograma.md)
