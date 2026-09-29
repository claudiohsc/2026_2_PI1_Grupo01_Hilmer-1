    # Projeto Conceitual de Software

- [Explicações adicionais](https://drive.google.com/file/d/1WWIz6609c7Y7zAQRBWHSbX0t2vEJ2z1A/view?usp=sharing)
- [Noções de UML](https://drive.google.com/file/d/1l1yt2ittHuRKIXVXYT76P1HriR07pK-t/view?usp=sharing)
- [Requisitos](https://engsoftmoderna.info/cap3.html)

> **Itens fundamentais:**
> - **Diagrama Atividades UML:** descrever e explicar o fluxo do comportamento funcional do produto proposto, evidenciando, de forma clara e estruturada, como as atividades são executadas, em que ordem e sob quais condições. Esse diagrama permite compreender o funcionamento dinâmico do sistema, destacando:
>   - os principais atores (usuários ou sistemas externos) envolvidos no processo;
>   - as atividades de negócio realizadas por cada ator ou pelo próprio sistema;
>   - os insumos (entradas) necessários para a execução das atividades;
>   - os resultados (saídas) gerados ao longo do fluxo;
>   - os pontos de decisão, paralelismo e sincronização das atividades;
>   - Entre as notações mais relevantes, destacam-se o estado inicial e final, atividades, nós de decisão e junção, barras de bifurcação, Raias (*swimlanes*) e Fluxos de controle.
> - **_Backlog_ do Produto**
>   - Detalhar os requisitos funcionais (RF) com a técnica de documentação e especificação de história de usuário (HU);
>   - Protótipos de interface gráfica do *software* em alta fidelidade.
>     - **Todas HUs devem conter sua descrição (Eu-Como-Para), critérios de aceitação e protótipos de interface, documentadas no github.**
>   - Exporte as informações do Backlog do Produto no GitHub Projects [em formato CSV](https://docs.github.com/en/issues/planning-and-tracking-with-projects/managing-your-project/exporting-your-projects-data), e [renderize em Markdown](https://www.google.com/search?q=convert+CSV+file+to+Markdown+table) no formato a seguir:

<a id="backlog-do-produto"></a>

## _Backlog_ do Produto

Os requisitos funcionais levantados em [Requisitos](2%20-%20Requisitos.md) foram detalhados em Histórias de Usuário (HUs) no formato *Eu-Como-Para*. Cada RF possui uma HU correspondente, com a mesma numeração (**HU-01 ↔ RF-01, …, HU-17 ↔ RF-17**), o que garante rastreabilidade direta entre requisito e história. A prioridade segue a classificação MoSCoW (*Must have*, *Should have*, *Could have*) definida para cada RF.

### Requisitos Funcionais

<u>[RF-01](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/3): Navegação pelo labirinto</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-01](#hu-01) | Percorrer o labirinto de forma autônoma até o objetivo | Must have |

<u>[RF-02](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/5): Reconhecimento de ambiente</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-02](#hu-02) | Reconhecer paredes e piso em cada célula | Must have |

<u>[RF-03](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/6): Construção do mapa do labirinto</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-03](#hu-03) | Construir o mapa quadriculado durante o percurso | Must have |

<u>[RF-04](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/7): Armazenamento do mapa do labirinto</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-04](#hu-04) | Armazenar o mapa do labirinto percorrido | Must have |

<u>[RF-05](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/8): Localização do robô</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-05](#hu-05) | Estimar a posição do robô por odometria | Must have |

<u>[RF-06](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/9): Percurso do robô</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-06](#hu-06) | Visualizar o percurso do robô sobre o mapa | Should have |

<u>[RF-07](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/10): Detecção do objetivo</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-07](#hu-07) | Detectar e sinalizar a chegada à célula objetivo | Must have |

<u>[RF-08](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/11): Telemetria em tempo real</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-08](#hu-08) | Acompanhar a telemetria do robô em tempo real | Must have |

<u>[RF-09](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/12): Armazenamento em banco de dados</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-09](#hu-09) | Persistir a telemetria das corridas no banco de dados | Must have |

<u>[RF-10](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/13): Consulta de todos os labirintos</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-10](#hu-10) | Consultar o histórico de todas as corridas | Should have |

<u>[RF-11](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/14): Sobrepujar condições do solo</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-11](#hu-11) | Compensar irregularidades do solo | Should have |

<u>[RF-12](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/15): Tratamento de colisões</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-12](#hu-12) | Evitar colisões com as paredes | Must have |

<u>[RF-13](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/16): Calibração de Hardware</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-13](#hu-13) | Autocalibrar os sensores antes da largada | Must have |

<u>[RF-14](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/17): Dashboard de Telemetria</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-14](#hu-14) | Supervisionar a corrida pelo dashboard web | Must have |

<u>[RF-15](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/18): Gestão de Corridas</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-15](#hu-15) | Registrar e filtrar corridas por tipo de labirinto | Must have |

<u>[RF-16](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/28): Estabilização de telemetria</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-16](#hu-16) | Não perder telemetria em quedas de conexão | Must have |

<u>[RF-17](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/29): Recuperação de Impacto</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| [HU-17](#hu-17) | Recuperar-se de colisões sem perder o mapa | Should have |

### Requisitos Não-Funcionais

| ID (Link Github Projects) | Título | Prioridade | Rastreabilidade |
|:--------------------------| :-- | :--- |:----------------|
| [RNF-01](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/19) | Dimensão máxima: até 16,5 cm de comprimento e largura, sem restrição de altura | Must have | RF-01/HU-01, RF-12/HU-12 |
| [RNF-02](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/20) | Locomoção somente terrestre: proibidos voo, salto, escalada ou propulsão por combustão/foguete | Must have | RF-01/HU-01, RF-11/HU-11 |
| [RNF-03](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/21) | Tempo limite: cada labirinto concluído em até 10 minutos | Must have | RF-01/HU-01, RF-14/HU-14 |
| [RNF-04](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/22) | Autonomia de bateria suficiente para os 3 labirintos consecutivos | Should have | RF-01/HU-01, RF-08/HU-08 |
| [RNF-05](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/23) | Latência da telemetria: exibição em até 2 s após a leitura do sensor | Should have | RF-08/HU-08, RF-14/HU-14, RF-16/HU-16 |
| [RNF-06](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/24) | Resistência mecânica a esforços e colisões sem perda de funcionamento | Must have | RF-12/HU-12, RF-17/HU-17 |
| [RNF-07](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/25) | Autonomia do sistema: 100% das decisões tomadas sem intervenção humana | Must have | RF-01/HU-01, RF-07/HU-07 |
| [RNF-08](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/26) | Inicialização de hardware: autocalibração concluída em até 1 s | Must have | RF-13/HU-13 |
| [RNF-09](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/27) | Inicialização de software: sistema pronto para o desafio em até 2 s | Should have | RF-13/HU-13, RF-14/HU-14 |

### Detalhamento das Histórias de Usuário

As HUs com interface gráfica (HU-06, HU-07, HU-08, HU-10, HU-14 e HU-15) estão vinculadas aos *frames* de alta fidelidade do [Protótipo Funcional Navegável](#prototipo-navegavel). As demais são executadas no *firmware* embarcado ou no *back-end* e não possuem tela própria; nesses casos é indicado o elemento do protótipo em que seu resultado se torna visível ao usuário.

---

<a id="hu-01"></a>

#### HU-01: Percorrer o Labirinto de Forma Autônoma até o Objetivo
* **Requisitos Associados:** [RF-01](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/3), [RNF-01](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/19), [RNF-02](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/20), [RNF-03](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/21), [RNF-04](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/22), [RNF-07](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/25)
* **Descrição:**
  * **Eu, como** equipe competidora,
  * **Quero** que o micromouse percorra o labirinto de forma totalmente autônoma, da célula de partida até a célula de destino,
  * **Para que** o desafio seja cumprido dentro do tempo regulamentar.
* **Critérios de Aceitação:**
  1. Após a largada, o robô deve calcular e executar o próximo movimento exclusivamente com base no mapa e nas leituras dos sensores, sem nenhuma intervenção humana (RNF-07).
  2. O robô deve alcançar a célula de destino (canto oposto ao início) nos três labirintos (4×4, 8×4 e 12×4), cada um em até 10 minutos (RNF-03).
  3. O deslocamento deve ocorrer apenas por locomoção terrestre sobre rodas (RNF-02).
  4. Caso o tempo de 10 minutos se esgote, o robô deve encerrar a tentativa e reportar o status `Não concluído`.
* **Protótipo da HU:** Não possui tela própria (*firmware*). O deslocamento é refletido pelo traçado `Robot Path` no [Frame 02_Running](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211).

---

<a id="hu-02"></a>

#### HU-02: Reconhecer Paredes e Piso em Cada Célula
* **Requisitos Associados:** [RF-02](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/5)
* **Descrição:**
  * **Eu, como** equipe responsável pela navegação autônoma,
  * **Quero** que o micromouse identifique a presença de paredes à frente, à esquerda e à direita de cada célula,
  * **Para que** o algoritmo de navegação saiba quais passagens estão livres e o mapa seja construído corretamente.
* **Critérios de Aceitação:**
  1. Ao chegar ao centro de cada célula (18 × 18 cm), o robô deve classificar os lados frontal, esquerdo e direito como `parede` ou `livre` a partir das leituras do LiDAR ToF 360°.
  2. A classificação deve ser feita com base em um limiar de distância definido em calibração, filtrando leituras espúrias (ex.: média de múltiplas amostras).
  3. Em teste de bancada com as configurações de parede conhecidas, a taxa de acerto da classificação deve ser de 100% nas células avaliadas.
* **Protótipo da HU:** Não possui tela própria (*firmware*). As paredes identificadas são refletidas no `Maze Grid` do [Frame 02_Running](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211).

---

<a id="hu-03"></a>

#### HU-03: Construir o Mapa Quadriculado durante o Percurso
* **Requisitos Associados:** [RF-03](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/6)
* **Descrição:**
  * **Eu, como** equipe responsável pela navegação autônoma,
  * **Quero** que o micromouse monte um mapa quadriculado do labirinto à medida que o percorre,
  * **Para que** o algoritmo de navegação planeje rotas com base no terreno já conhecido e o mapa possa ser reproduzido no *dashboard*.
* **Critérios de Aceitação:**
  1. O mapa deve ser representado como uma matriz de células com as dimensões do labirinto em execução (4×4, 8×4 ou 12×4), registrando as quatro paredes de cada célula.
  2. Cada célula visitada deve ter suas paredes gravadas no mapa, mantendo a consistência entre células vizinhas (a parede leste de uma célula é a parede oeste da vizinha).
  3. As atualizações do mapa devem ser enviadas na telemetria para exibição no *dashboard*.
* **Protótipo da HU:** Não possui tela própria (*firmware*). O mapa construído é exibido no `Maze Grid` do [Frame 02_Running](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211).

---

<a id="hu-04"></a>

#### HU-04: Armazenar o Mapa do Labirinto Percorrido
* **Requisitos Associados:** [RF-04](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/7)
* **Descrição:**
  * **Eu, como** equipe responsável pela navegação autônoma,
  * **Quero** que o mapa do labirinto seja mantido em memória durante a corrida e armazenado ao final,
  * **Para que** o robô planeje rotas eficientes e o mapa percorrido fique disponível para consulta posterior.
* **Critérios de Aceitação:**
  1. O mapa deve permanecer em memória durante toda a corrida, inclusive após colisões ou perda de conexão.
  2. A estrutura de armazenamento deve comportar o maior labirinto da competição (12×4 células).
  3. Ao alcançar a célula objetivo, o mapa final do labirinto percorrido deve ser armazenado e enviado ao servidor, vinculado ao registro da corrida (HU-09).
* **Protótipo da HU:** Não possui tela própria (*firmware*). O mapa final é exibido no `Maze Grid` do [Frame 03_Completed](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=14-337).

---

<a id="hu-05"></a>

#### HU-05: Estimar a Posição do Robô por Odometria
* **Requisitos Associados:** [RF-05](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/8)
* **Descrição:**
  * **Eu, como** equipe responsável pela navegação autônoma,
  * **Quero** que o micromouse estime continuamente sua posição (célula) e orientação no labirinto,
  * **Para que** ele saiba onde está no mapa e o *dashboard* exiba sua localização real.
* **Critérios de Aceitação:**
  1. A posição deve ser estimada a partir da contagem dos *encoders* de efeito Hall e a orientação a partir do giroscópio da IMU, com a célula de partida como origem (0, 0).
  2. A cada célula percorrida, a posição discreta (linha, coluna) e a orientação (N, S, L, O) devem ser atualizadas e enviadas na telemetria.
  3. O erro acumulado deve ser corrigido usando as paredes detectadas como referência (alinhamento ao centro da célula).
  4. Ao final de uma corrida em bancada, a célula estimada pelo robô deve coincidir com a célula real em que ele se encontra.
* **Protótipo da HU:** Não possui tela própria (*firmware*). A posição estimada é exibida pelo marcador `Robot` no [Frame 02_Running](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211).

---

<a id="hu-06"></a>

#### HU-06: Visualizar o Percurso do Robô sobre o Mapa
* **Requisitos Associados:** [RF-06](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/9)
* **Descrição:**
  * **Eu, como** operador de bancada ou avaliador da competição,
  * **Quero** visualizar graficamente o labirinto e o percurso feito pelo micromouse em tempo real,
  * **Para que** eu possa analisar e validar a estratégia de exploração do robô.
* **Critérios de Aceitação:**
  1. A malha do labirinto deve apresentar fundo escuro (`#0F172A`) com a célula de partida em verde suave no canto inferior e a de chegada em vermelho no canto oposto.
  2. O traçado (`Robot Path`) deve ser atualizado ortogonalmente em tempo real conforme a odometria do robô avança.
  3. O marcador `Robot` deve indicar a célula atualmente ocupada pelo micromouse.
  4. Ao término da corrida, o traçado completo deve permanecer visível sobre o mapa.
* **Protótipo da HU:**
  * [Frame 01_Idle (Aguardando Largada)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121)
  * [Frame 02_Running (Em Execução)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211)
  * [Frame 03_Completed (Desafio Concluído)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=14-337)

---

<a id="hu-07"></a>

#### HU-07: Detectar e Sinalizar a Chegada à Célula Objetivo
* **Requisitos Associados:** [RF-07](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/10), [RNF-07](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/25)
* **Descrição:**
  * **Eu, como** operador de bancada ou avaliador da competição,
  * **Quero** que o micromouse reconheça quando alcançou a célula de destino e que isso seja sinalizado no *dashboard*,
  * **Para que** a conclusão do desafio seja registrada sem depender de verificação manual.
* **Critérios de Aceitação:**
  1. Ao chegar à célula de destino (canto oposto ao início), o robô deve parar os motores e enviar o evento `objetivo alcançado` na telemetria.
  2. Ao receber o evento, o badge de status do *dashboard* deve transitar para `DESAFIO CUMPRIDO: SIM` (verde) e o cronômetro deve ser congelado.
  3. A detecção deve ocorrer de forma autônoma, sem acionamento manual (RNF-07).
* **Protótipo da HU:**
  * [Frame 03_Completed (Desafio Concluído)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=14-337)

---

<a id="hu-08"></a>

#### HU-08: Acompanhar a Telemetria do Robô em Tempo Real
* **Requisitos Associados:** [RF-08](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/11), [RNF-04](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/22), [RNF-05](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/23)
* **Descrição:**
  * **Eu, como** operador de bancada,
  * **Quero** acompanhar em tempo real os dados de telemetria enviados pelo micromouse,
  * **Para que** eu possa monitorar as condições e o funcionamento do robô durante o desafio.
* **Critérios de Aceitação:**
  1. O ESP32-C3 deve enviar pacotes de telemetria contendo posição na malha, velocidade, tensão/corrente da bateria, tempo decorrido e estado da corrida, com carimbo de tempo e número de sequência.
  2. Os dados devem ser exibidos no *dashboard* em até 2 s após a leitura do sensor correspondente (RNF-05).
  3. Devem ser exibidos cards dedicados de telemetria contendo consumo de bateria (%, V, mA), velocidade média/pico e cronômetro da tentativa atual.
* **Protótipo da HU:**
  * [Frame 02_Running (Em Execução)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211)

---

<a id="hu-09"></a>

#### HU-09: Persistir a Telemetria das Corridas no Banco de Dados
* **Requisitos Associados:** [RF-09](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/12)
* **Descrição:**
  * **Eu, como** membro da equipe,
  * **Quero** que os dados de telemetria de cada corrida sejam armazenados no banco de dados,
  * **Para que** possam ser consultados, comparados e analisados após a corrida.
* **Critérios de Aceitação:**
  1. As amostras de telemetria recebidas durante a corrida (posição, velocidade, tensão/corrente da bateria e tempo) devem ser persistidas vinculadas ao identificador da corrida (HU-15).
  2. Ao término (objetivo alcançado, tempo esgotado ou interrupção), o registro deve ser consolidado com tempo final, velocidade média e de pico, consumo de bateria, status (`Concluído` / `Não concluído`) e o mapa final (HU-04).
  3. Os dados persistidos devem permanecer disponíveis para consulta após o encerramento do sistema.
* **Protótipo da HU:** Não possui tela própria (*back-end*). A persistência é acionada na transição para o [Frame 03_Completed](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=14-337) e os dados aparecem no [Frame 04_History_View](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-411).

---

<a id="hu-10"></a>

#### HU-10: Consultar o Histórico de Todas as Corridas
* **Requisitos Associados:** [RF-10](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/13)
* **Descrição:**
  * **Eu, como** membro da equipe ou docente avaliador,
  * **Quero** acessar o histórico de todas as corridas persistidas em banco de dados,
  * **Para que** eu possa auditar a evolução do desempenho, verificar a pontuação por tentativa e comparar os tempos finais consolidados.
* **Critérios de Aceitação:**
  1. A navegação entre a telemetria ao vivo e o histórico deve ocorrer de forma instantânea via abas no cabeçalho global.
  2. A visualização padrão (`Todos os Labirintos`) deve listar tabularmente todas as corridas salvas no banco com Data/Hora, Labirinto, Tentativa/Nota, Tempo Final, Velocidade, Bateria e Status.
* **Protótipo da HU:**
  * [Frame 04_History_View (Todos os Labirintos)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-411)

---

<a id="hu-11"></a>

#### HU-11: Compensar Irregularidades do Solo
* **Requisitos Associados:** [RF-11](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/14), [RNF-02](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/20)
* **Descrição:**
  * **Eu, como** equipe competidora,
  * **Quero** que o micromouse mantenha tração e trajetória ao passar por pequenas irregularidades no piso,
  * **Para que** o percurso não seja interrompido por desníveis ou emendas do labirinto.
* **Critérios de Aceitação:**
  1. O robô deve transpor desníveis e emendas do piso do labirinto apenas por locomoção terrestre sobre rodas (RNF-02).
  2. O controle de velocidade deve compensar a diferença de rotação entre as rodas (medida pelos *encoders*) ao passar pela irregularidade, mantendo a trajetória.
  3. A estimativa de posição (HU-05) não deve perder a célula corrente após a passagem pela irregularidade.
* **Protótipo da HU:** Não possui tela própria (*firmware*); não há representação gráfica específica no protótipo.

---

<a id="hu-12"></a>

#### HU-12: Evitar Colisões com as Paredes
* **Requisitos Associados:** [RF-12](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/15), [RNF-01](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/19), [RNF-06](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/24)
* **Descrição:**
  * **Eu, como** equipe competidora,
  * **Quero** que o micromouse mantenha distância segura das paredes durante o deslocamento,
  * **Para que** ele não danifique sua estrutura nem perca tempo e posição com impactos.
* **Critérios de Aceitação:**
  1. Durante o deslocamento em linha reta, o robô deve corrigir sua trajetória (controle dos motores) para se manter centralizado no corredor com base nas distâncias laterais medidas.
  2. O robô deve reduzir a velocidade e parar antes de atingir uma parede frontal detectada.
  3. Em teste de bancada percorrendo o labirinto completo, não deve haver contato do chassi com as paredes.
* **Protótipo da HU:** Não possui tela própria (*firmware*); não há representação gráfica específica no protótipo.

---

<a id="hu-13"></a>

#### HU-13: Autocalibrar os Sensores antes da Largada
* **Requisitos Associados:** [RF-13](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/16), [RNF-08](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/26), [RNF-09](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/27)
* **Descrição:**
  * **Eu, como** operador de bancada,
  * **Quero** que o micromouse execute automaticamente uma rotina de calibração dos sensores de distância e inerciais ao ser ligado,
  * **Para que** as leituras sejam confiáveis desde a primeira célula, sem ajustes manuais antes de cada tentativa.
* **Critérios de Aceitação:**
  1. Ao ser ligado na célula de partida, o robô deve calibrar o sensor de distância (LiDAR ToF), a IMU (*offset* do giroscópio com o robô parado) e zerar a contagem dos *encoders*.
  2. Cada execução da autocalibração deve ser concluída em até 1 s (RNF-08) e, quando a primeira execução for bem-sucedida, o sistema deve estar pronto para largada em até 2 s após ligado (RNF-09).
  3. Caso algum sensor não responda ou retorne valores fora da faixa esperada, a calibração deve ser considerada malsucedida e a rotina deve ser executada novamente; o robô não deve iniciar o deslocamento enquanto a calibração não for bem-sucedida, e cada falha deve ser reportada na telemetria.
  4. Concluída a calibração com sucesso, o estado do robô deve ser informado ao *dashboard* como `Aguardando Largada`.
* **Protótipo da HU:** Não possui tela própria (*firmware*). O resultado é visível no [Frame 01_Idle](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121), que representa o robô calibrado e aguardando largada.

---

<a id="hu-14"></a>

#### HU-14: Supervisionar a Corrida pelo Dashboard Web
* **Requisitos Associados:** [RF-14](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/17), [RNF-03](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/22), [RNF-05](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/23), [RNF-09](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/27)
* **Descrição:**
  * **Eu, como** operador de bancada ou avaliador da competição,
  * **Quero** um *dashboard* web que reúna o mapa bidimensional, o trajeto, a velocidade média e o consumo de bateria do micromouse,
  * **Para que** eu tenha controle visual completo sobre a execução autônoma do robô e possa validar o cumprimento do desafio dentro do teto regulamentar de 10 minutos.
* **Critérios de Aceitação:**
  1. A tela de telemetria ao vivo deve reunir, em uma única visualização, o mapa do labirinto com o trajeto (HU-06) e o painel lateral de cards de telemetria (HU-08).
  2. O cronômetro deve exibir o tempo da tentativa atual com marcador explícito do limite de 10:00 min (RNF-03).
  3. O *dashboard* deve refletir os estados da corrida: `Aguardando Largada`, `Em Execução` e `Desafio Concluído`.
  4. Deve haver controles para iniciar a corrida (`INICIAR CORRIDA`) e para reiniciar uma nova tentativa (`RESETAR / NOVA TENTATIVA`).
* **Protótipo da HU:**
  * [Frame 01_Idle (Aguardando Largada)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121)
  * [Frame 02_Running (Em Execução)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211)
  * [Frame 03_Completed (Desafio Concluído)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=14-337)

---

<a id="hu-15"></a>

#### HU-15: Registrar e Filtrar Corridas por Tipo de Labirinto
* **Requisitos Associados:** [RF-15](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/18)
* **Descrição:**
  * **Eu, como** membro da equipe ou docente avaliador,
  * **Quero** que cada tentativa seja registrada com um identificador único e o tipo de labirinto, e poder filtrar o histórico por esse tipo,
  * **Para que** eu possa comparar o desempenho do robô em cada configuração de labirinto.
* **Critérios de Aceitação:**
  1. Ao iniciar uma corrida, o sistema deve criar um registro com identificador único, data/hora, tipo de labirinto (`4×4`, `8×4` ou `12×4`) e número da tentativa.
  2. Nenhuma corrida pode ser gravada sem tipo de labirinto associado.
  3. Ao fim da corrida, o registro deve ser encerrado e disponibilizado no histórico de consulta.
  4. O controle segmentado de filtros deve permitir isolar os registros exclusivamente por labirinto (`4×4`, `8×4` ou `12×4`).
* **Protótipo da HU:**
  * [Frame 04_History_View_M1 (Filtro Labirinto 1)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-573)
  * [Frame 04_History_View_M2 (Filtro Labirinto 2)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-655)
  * [Frame 04_History_View_M3 (Filtro Labirinto 3)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-737)

---

<a id="hu-16"></a>

#### HU-16: Não Perder Telemetria em Quedas de Conexão
* **Requisitos Associados:** [RF-16](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/28), [RNF-05](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/23)
* **Descrição:**
  * **Eu, como** operador de bancada,
  * **Quero** que os dados de telemetria sejam mantidos em *buffer* quando houver instabilidade na conexão,
  * **Para que** nenhum dado da corrida seja perdido e o histórico fique completo.
* **Critérios de Aceitação:**
  1. Enquanto a conexão estiver indisponível, o robô deve armazenar os pacotes de telemetria em *buffer* local, sem interromper a navegação.
  2. Restabelecida a conexão, os pacotes do *buffer* devem ser reenviados em ordem cronológica.
  3. O sistema web deve receber a telemetria em *buffer* antes da persistência, ordenando os pacotes pelo número de sequência e descartando duplicados.
* **Protótipo da HU:** Não possui tela própria (*firmware*/*back-end*). O resultado é visível na atualização contínua dos cards de telemetria do [Frame 02_Running](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211).

---

<a id="hu-17"></a>

#### HU-17: Recuperar-se de Colisões sem Perder o Mapa
* **Requisitos Associados:** [RF-17](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/29), [RNF-06](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/24)
* **Descrição:**
  * **Eu, como** equipe competidora,
  * **Quero** que o micromouse detecte uma colisão e retome a exploração a partir de onde estava,
  * **Para que** um impacto não invalide a corrida nem descarte o mapeamento já realizado.
* **Critérios de Aceitação:**
  1. A colisão deve ser detectada por variação brusca de aceleração na IMU ou por travamento das rodas (*encoders* parados com motores acionados).
  2. Após a detecção, o robô deve recuar, realinhar-se ao centro da célula e retomar a navegação.
  3. O mapa construído (HU-03/HU-04) e a posição estimada (HU-05) devem ser preservados após a recuperação.
  4. O evento de colisão deve ser registrado na telemetria da corrida.
* **Protótipo da HU:** Não possui tela própria (*firmware*); não há representação gráfica específica no protótipo.

<a id="prototipo-navegavel"></a>

## Protótipo Funcional do Software, Navegável

> **Nota de Integração:** Os protótipos de interface estão vinculados diretamente às Histórias de Usuário (HUs) do [Backlog do Produto](#backlog-do-produto) acima, que contêm a descrição (*Eu-Como-Para*), os critérios de aceitação e os links para os *frames* de alta fidelidade no Figma.

### 1. Acesso ao Projeto no Figma
* **Ambiente Interativo (Flow 1):** [Executar Protótipo Navegável](https://www.figma.com/proto/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121&p=f&t=jkjBO9W8kCzgRT8u-1&scaling=min-zoom&content-scaling=fixed&page-id=0%3A1&starting-point-node-id=7%3A121)
* **Canvas de Design (Estrutura de Frames):** [Acessar Arquivo de Design](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=0-1&t=axS6kELL1SrmfIfD-1)

---

### 2. Mapeamento de Telas e Estados Operacionais

A interface do sistema web foi concebida para atender à supervisão de bancada e à persistência dos resultados, dividindo-se entre a visualização dinâmica de corrida e a área analítica de banco de dados. Como cada HU corresponde ao RF de mesmo número, a coluna de HUs também indica os RFs atendidos (ex.: HU-06 ↔ RF-06):

| Identificador | Nome da Vista / Estado | Descrição Funcional | HUs / RFs atendidos | RNFs |
| :---: | :--- | :--- | :---: | :---: |
| `01_Idle` | Aguardando Largada | Robô alinhado na célula verde de largada, sensores e odometria sincronizados, métricas nominais zeradas e acionamento manual de início. | HU-06, HU-13, HU-14 | RNF-09 |
| `02_Running` | Telemetria ao Vivo | Plotagem vetorial em tempo real da rota percorrida sobre a malha escura, atualização contínua de velocidade média, consumo de bateria e cronometragem. | HU-01, HU-02, HU-03, HU-05, HU-06, HU-08, HU-14, HU-16 | RNF-03, RNF-05 |
| `03_Completed` | Conclusão do Desafio | Detecção de chegada ao objetivo diametralmente oposto, travamento do cronômetro, sinalização verde de `DESAFIO CUMPRIDO: SIM` e acionamento de persistência. | HU-04, HU-06, HU-07, HU-09, HU-14 | RNF-03 |
| `04_History_View` | Consulta Consolidada | Tabela do banco de dados exibindo o histórico de todas as corridas realizadas com identificadores únicos, tempos, velocidades e status de aprovação. | HU-09, HU-10 | — |
| `04_History_View_M1..M3` | Filtros por Labirinto | Visão tabular segmentada isolando individualmente as execuções dos labirintos 4×4, 8×4 ou 12×4. | HU-15 | — |

---

### 3. Matriz Consolidada de Rastreabilidade dos Requisitos

| ID | Requisito Formal | Implementação no Protótipo |
| :---: | :--- | :--- |
| **RF-03 / RF-06** | Construção do mapa e percurso do robô | Frame `Maze Grid` com malha dimensional ortogonal de fundo (RF-03) e vetor dinâmico alaranjado `Robot Path` indicando a rota percorrida e a odometria (RF-06). |
| **RF-05** | Localização do robô | Marcador circular `Robot` indicando em tempo real a célula discreta ocupada pelo micromouse. |
| **RF-07** | Detecção do objetivo | Célula de destino com realce avermelhado no canto diametralmente oposto; aciona o badge de missão cumprida ao ser interceptada. |
| **RF-08 / RF-14** | Telemetria / Dashboard Web | Painel lateral contendo cartões desacoplados em auto layout para consumo de bateria, velocidade média (com pico) e tempo. |
| **RF-09** | Armazenamento em banco de dados | Transição para o estado `03_Completed`, consolidando métricas finais para envio ao repositório relacional. |
| **RF-10** | Consulta de todos os labirintos | Modo de visualização global da tabela histórica agregando as corridas de todas as configurações. |
| **RF-15** | Gestão de Corridas / Filtros | Barra de controle segmentado (`Todos`, `4×4`, `8×4`, `12×4`) permitindo a filtragem imediata das consultas. |
| **RNF-03** | Tempo limite de execução | Cronômetro com marcador explícito do teto de 10:00 minutos regulamentares da bateria de testes. |

---

### 4. Roteiro Operacional de Navegação do Protótipo

1. **Estado Inicial (`01_Idle`):** Clique no botão primário **`INICIAR CORRIDA`** na barra lateral para iniciar a transmissão de pacotes e avançar para `02_Running`.
2. **Execução e Conclusão (`02_Running` $\rightarrow$ `03_Completed`):** A navegação progride dinamicamente via *After delay* com *Smart Animate* até alcançar a célula de chegada oposta, disparando o badge verde de conclusão e o congelamento do cronômetro.
3. **Nova Tentativa:** No frame `03_Completed`, o botão **`RESETAR / NOVA TENTATIVA`** reinicia o ciclo em `01_Idle` para nova passagem de bancada.
4. **Auditoria de Histórico:** No cabeçalho global, clique em **`Histórico de Consultas`** para alternar para a visão analítica (`04_History_View`), navegando entre os filtros de labirinto para inspecionar os dados persistidos.
5. **Filtragem de Dados:** Na tela de histórico, clique nos botões de controle segmentado (`Todos`, `Labirinto 1`, `Labirinto 2`, `Labirinto 3`) para alternar a exibição filtrada dos registros. Para voltar à bancada ao vivo, selecione a aba **`Telemetria ao Vivo`**.

> 
> - **Descrição da arquitetura da solução de _software_ proposta:**
>   - Esta subseção deve contemplar o documento de arquitetura do sistema e deve ser estruturado segundo as visões (4+1) previstas no processo unificado (UP): lógica, de processos; implementação, implantação e dados (substituirá a visão de casos de uso).
>   - Propósito do *software* (qual o seu papel no sistema);
>   - Padrão adotado: MVC, MVP, Microsserviços, Monolítico, etc (Justificar);
>   - Linguagens de programação: Java, Python, C#, JavaScript, etc.
>   - *Frameworks* e bibliotecas: Spring Boot, .NET Core, React, Angular, Django, etc.
>   - Banco de dados: Relacional (PostgreSQL, MySQL, etc) X NãoSQL (MongoDB, etc).
>   - Persistência de dados: Modelo Entidade-Relacionamento (MER) e seu respectivo Diagrama Entidade-Relacionamento (DER), aplicáveis quando a solução utiliza banco de dados relacional; alternativamente, diagrama de estrutura de documentos, empregado nos casos em que a arquitetura adota banco de dados não relacional.
> - **Roteiro de testes funcionais:**
>   - Código do caso de teste;
>   - Nome do caso de teste;
>   - Rastreabilidade: Link do(a) RF/HU associado(a);
>   - Objetivo do caso de teste;
>   - Pré-condições do sistema para o teste ser realizado, quando se aplicar;
>   - Descrição dos procedimentos a serem executados para o teste;
>   - Resultado esperado para o teste ser aprovado (pós-condição após realizado o teste);
