# Análise de Consumo Energético do Produto

## 1. Identificação dos Subsistemas Elétricos

A arquitetura do Micromouse organiza a distribuição de potência e os níveis lógicos a partir da fonte principal. O subsistema elétrico divide-se em barramentos de potência e barramentos de regulação chaveada. O perfil de carga foi levantado com base nas especificações dos componentes e na folha de dados da equipe. Para o dimensionamento da autonomia contínua, padronizou-se o ciclo de missão em **600 segundos (10 minutos)** de exploração e tomada de tempo.

* **ESP32-C3-MINI-1 (MCU e Telemetria):** Opera sob linha regulada de 3,3 V derivada do conversor Buck dedicado. Consumo médio nominal de **110 mA (0,11 A)** em processamento de controle e transmissões periódicas de rádio.
* **Motores DC N20 com Redução (Par de Atuadores M1 e M2):** Tensão nominal de catálogo de 6,0 V. Consumo médio dinâmico em piso de corrida estimado em **0,15 A por motor (0,30 A conjunto)**.
* **Driver Ponte H Dupla (DRV8833 - Lógica de Controle):** Alimentação lógica e pinos de sinal PWM a 3,3 V comutados pelos GPIOs 4, 5, 6 e 7 do microcontrolador. Apresenta consumo quiescente de **3 mA (0,003 A)**.
* **Encoders Incrementais de Efeito Hall (2 unidades):** Conectados via interrupções de quadratura aos GPIOs do MCU e alimentados pela linha de 3,3 V. Consumo conjunto estimado em **10 mA (0,01 A)**.
* **Sensor LiDAR ToF 360° (Sensoriamento Primário de Pista):** Opera com linha dedicada de alimentação de 5,0 V (+5V0) e interface serial UART. Apresenta consumo nominal médio de **150 mA (0,15 A)** em rotação contínua e varredura óptica.

---

## 2. Cálculo da Energia Consumida por Componente

O cálculo de energia elétrica em regime de corrente contínua adota a relação fundamental:

$$E = V \cdot I \cdot t$$

Onde:
* $E$ = Energia em joules (J)
* $V$ = Tensão de alimentação do subsistema em volts (V)
* $I$ = Corrente média consumida em ampères (A)
* $t$ = Tempo operacional de missão ($t = 600\text{ s}$)

| Componente | Tensão ($V$) | Corrente Média ($I$) | Tempo ($t$) | Energia Consumida ($E$) |
| :--- | :---: | :---: | :---: | :---: |
| ESP32-C3-MINI-1 | 3,3 V | 0,110 A | 600 s | 217,80 J |
| Motores DC N20 (Par M1 + M2) | 6,0 V | 0,300 A | 600 s | 1.080,00 J |
| Driver DRV8833 (Lógica) | 3,3 V | 0,003 A | 600 s | 5,94 J |
| Encoders Incrementais (Par) | 3,3 V | 0,010 A | 600 s | 19,80 J |
| Sensor LiDAR ToF 360° | 5,0 V | 0,150 A | 600 s | 450,00 J |

---

## 3. Estimativa do Consumo Total de Energia

A demanda energética total calculada para um ciclo de 10 minutos de missão ($E_{\text{total}}$) é obtida pela soma das parcelas energéticas de cada componente:

$$E_{\text{total}} = 217,80 + 1080,00 + 5,94 + 19,80 + 450,00$$

$$E_{\text{total}} = \mathbf{1.773,54\text{ J}}$$

---

## 4. Escolha da Fonte de Alimentação

### Conversão para Watt-hora (Wh)
Utilizando a equivalência padrão ($1\text{ Wh} = 3600\text{ J}$):

$$E_{\text{Wh}} = \frac{E_{\text{total}}}{3600} = \frac{1773,54}{3600} \approx \mathbf{0,493\text{ Wh}}$$

### Margem de Segurança e Eficiência Energética
Para robôs móveis autônomos de labirinto, adota-se uma margem de segurança de **40%**, combinada com um rendimento médio de **85%** ($\eta = 0,85$) dos conversores chaveados Buck da placa. Essa folga de projeto sustenta:
1. **Transientes Dinâmicos:** As partidas, frenagens e rotações rápidas dos motores elevam a corrente momentânea muito acima do patamar nominal de regime permanente.
2. **Perdas Térmicas nos Conversores:** O chaveamento e a resistência parasita das bobinas e diodos internos dos módulos Buck degradam parte da potência útil em calor.
3. **Preservação Química da Célula:** Células à base de lítio sofrem danos de capacidade irreversíveis quando descarregadas abaixo de 20% de sua reserva total.

A energia total demandada na fonte é:

$$E_{\text{necessária}} = \frac{E_{\text{Wh}}}{\eta \cdot (1 - \text{Margem})} = \frac{0,493}{0,85 \cdot (1 - 0,40)} \approx \mathbf{0,967\text{ Wh}}$$

### Especificação da Bateria Selecionada
* **Topologia:** Pacote de bateria recarregável Li-ion / LiPo 2S.
* **Faixa de Tensão:** 7,4 V nominal e 8,4 V em patamar de carga plena.
* **Capacidade Mínima Teórica Requerida:**

$$\text{Capacidade (Ah)} = \frac{E_{\text{necessária}}}{V_{\text{nominal}}} = \frac{0,967\text{ Wh}}{7,4\text{ V}} \approx 0,130\text{ Ah} \implies \mathbf{130\text{ mAh}}$$

* **Modelo Comercial Homologado:** Bateria LiPo 2S de **650 mAh** (modelo **Tattu 650mAh 2S1P 75C / 150C burst** com conector XT-30).
* **Justificativa Técnica:** Essa capacidade garante fornecimento contínuo de pico de até 48,75 A sem colapsar a tensão do barramento (*voltage sag*), assegura peso baixo compatível com o chassi (43 g ±20 g) e estende a autonomia contínua para mais de 65 minutos em regime de corrida, cobrindo com folga de +118% a meta regulamentar de 30 minutos dos 3 labirintos (RNF-04).

### 4.5 Mapeamento Matemático da Curva de Descarga (Issue #139)

O comportamento de descarga de células eletroquímicas à base de Polímero de Lítio (LiPo) não é linear, apresentando três zonas operacionais características: uma queda exponencial pronunciada logo após a plena carga, um extenso platô de tensão relativamente constante e um joelho de descarga profunda com decaimento acentuado de tensão.

Para permitir a simulação do comportamento da bateria no firmware (Issue #215) e o diagnóstico preditivo de carga sem demandar testes destrutivos imediatos de bancada, implementou-se o modelo matemático eletroquímico de **Shepherd modificado** (formalizado por Tremblay e Dessaint):

$$V_{\text{cel}}(Q_{\text{d}}) = E_0 - R \cdot I - K \cdot \left(\frac{Q}{Q - Q_{\text{d}}}\right) \cdot Q_{\text{d}} + A \cdot e^{-B \cdot Q_{\text{d}}}$$

$$V_{\text{pack}}(t) = 2 \cdot V_{\text{cel}}(Q_{\text{d}}(t))$$

Onde:
* $V_{\text{pack}}(t)$: Tensão total nos terminais do pack LiPo 2S (V).
* $E_0$: Tensão constante do patamar eletroquímico por célula ($E_0 = 3{,}740\text{ V}$).
* $R$: Resistência interna série por célula ($R = 15\text{ m}\Omega \implies 30\text{ m}\Omega$ no pack).
* $I$: Corrente média de descarga contínua em regime de missão nominal ($I = 0{,}476\text{ A}$).
* $K$: Coeficiente de polarização na zona de esgotamento ($K = 0{,}085\text{ V}$).
* $Q$: Capacidade nominal total da célula ($Q = 0{,}650\text{ Ah} = 650\text{ mAh}$).
* $Q_{\text{d}}$: Carga acumulada descarregada até o instante $t$ ($Q_{\text{d}} = I \cdot t$).
* $A$: Amplitude da zona exponencial inicial de estabilização pós-carga ($A = 0{,}440\text{ V}$).
* $B$: Constante de capacidade inversa da zona exponencial ($B = 16{,}0\ \text{Ah}^{-1}$).

#### Curva de Descarga Teórica Simulada

O script computacional implementado em `scripts/curva_descarga_lipo.py` modela a resposta dinâmica sob regime de corrida nominal ($476\text{ mA}$) e regime de baixo consumo ($150\text{ mA}$ em *standby* / leitura de sensores), gerando a curva característica apresentada na figura abaixo:

![Curva Teórica de Descarga LiPo 2S](../figs/curva-descarga-lipo-2s.svg)

#### Tabela de Correlação: Estado de Carga (SoC) vs. Tensão de Terminal

A relação entre o Estado de Carga (*State of Charge* - SoC), a tensão sob carga nominal contínua ($I = 476\text{ mA}$) e a energia residual útil do pack Tattu 650 mAh é sumarizada na tabela abaixo:

| Estado de Carga (SoC) | Tensão do Pack 2S ($V_{\text{pack}}$) | Tensão por Célula ($V_{\text{cel}}$) | Capacidade Restante (mAh) | Regime Operacional / Diagnóstico |
| :---: | :---: | :---: | :---: | :--- |
| **100%** | **8,35 V a 8,40 V** | 4,17 V a 4,20 V | 650 mAh | Bateria plena pós-carregamento balanceado |
| **90%**  | 7,78 V | 3,89 V | 585 mAh | Fim da zona de decaimento exponencial inicial |
| **80%**  | 7,57 V | 3,78 V | 520 mAh | Início do platô estável de corrida |
| **70%**  | 7,48 V | 3,74 V | 455 mAh | Platô nominal de regime permanente |
| **60%**  | 7,43 V | 3,72 V | 390 mAh | **Ponto operacional aos 30 min de prova (RNF-04)** |
| **50%**  | 7,39 V | 3,69 V | 325 mAh | Metade da carga útil |
| **40%**  | 7,31 V | 3,66 V | 260 mAh | Operação em corrida contínua segura |
| **30%**  | 7,19 V | 3,59 V | 195 mAh | Alerta amarelo no Dashboard de telemetria |
| **20%**  | 6,92 V | 3,46 V | 130 mAh | Aproximação do joelho de descarga rápida |
| **10%**  | 6,09 V | 3,04 V | 65 mAh | Zona de esgotamento acentuado |
| **0% (Corte UVLO)**| **6,60 V** | **3,30 V** | **~45 mAh (7%)** | **Desarme forçado via software (PWM = 0)** |

#### Integração com a Camada de Software e Firmware

Os coeficientes e a tabela de conversão foram exportados diretamente para o arquivo de cabeçalho C++ `src/firmware/lib/simulacao/TabelaDescargaLiPo.h`. Essa tabela é consumida pela classe de emulação `SensorBateriaMock` (Issue #215), garantindo que:
1. O Dashboard web receba leituras realistas e reproduzíveis de tensão e porcentagem de bateria ao longo do tempo.
2. A máquina de estados de subtensão (*Under-Voltage Lockout* — UVLO) seja testada com antecedência, cortando a tração quando $V_{\text{pack}} \le 6{,}60\text{ V}$.
3. Haja um gabarito teórico formal para validação e cálculo do erro experimental quando a bateria física for submetida ao ensaio de descarga em bancada.

---

## 5. Planejamento do Circuito de Alimentação

A distribuição de energia segrega a malha de acionamento eletromecânico dos circuitos lógicos e de sensoriamento:

* **Entrada de Energia, Gerenciamento e Chaveamento:**
  * O polo positivo da bateria Li-ion 2S passa inicialmente por um módulo **BMS (Battery Management System)**, responsável pelo corte em subtensão, proteção contra curto-circuito e balanceamento de células durante a carga.
  * Em série com a saída do BMS, uma **Chave de Potência** comanda a ligação geral do circuito antes da derivação dos barramentos.
* **Barramento Direto dos Motores (+VBAT_2S / Pinos VM):**
  * O terminal `VM` do driver DRV8833 recebe diretamente a tensão não regulada da bateria (+VBAT_2S, entre 7,4 V e 8,4 V).
  * *Validação da Tensão de 8,4 V em Motores Nominais de 6,0 V:* O DRV8833 suporta até 10,8 V em VM, operando com ampla folga de segurança. Para proteger as bobinas dos micromotores N20 sem a inclusão de um regulador de potência de 6 V, adota-se **limitação por software via modulação PWM**: o *duty cycle* máximo enviado pelo ESP32-C3 é travado em **71%** ($6,0\text{ V} / 8,4\text{ V}$), garantindo que a tensão eficaz nos motores não ultrapasse os 6,0 V nominais sob bateria plena.
* **Barramento de Regulação Chaveada (Dois Módulos Buck Independentes):**
  * **Conversor Buck +5V0:** Regulador Step-Down com capacidade mínima de 1 A contínuo. Alimenta de forma isolada a linha de potência do sensor LiDAR ToF 360°.
  * **Conversor Buck +3V3:** Regulador Step-Down com capacidade mínima de 1 A contínuo. Alimenta a linha digital do microcontrolador ESP32-C3-MINI-1, os encoders de quadratura e a polarização lógica do DRV8833. A especificação de 1 A supre com ampla folga os picos transitórios de rádio do chip ESP32 (de até 350 mA).
* **Controle de Ruído e Estabilidade de Referência (GND Comum):**
  * **Filtragem de Transientes:** Instalação de capacitores eletrolíticos Low-ESR de 220 µF a 470 µF soldados nas proximidades do pino VM do DRV8833, absorvendo o rebote indutivo provocado pelas manobras dos motores.
  * **Desacoplamento de Linha:** Capacitores cerâmicos de 100 nF distribuídos nos terminais de cada circuito integrado e diretamente soldados na carcaça dos motores N20.
  * **Topologia de Referência (GND Único em Estrela):** Todos os módulos compartilham o mesmo potencial de terra, mas as trilhas de retorno de alta corrente (motores e chaveamento) são fisicamente separadas do terra de sinal do MCU e dos sensores, conectando-se em um único nó central junto ao polo negativo do BMS. Isso impede que transitórios induzidos por comutação mecânica gerem flutuações e causem *brownout reset* no processador.

---

## 6. Monitoramento via Software Embarcado

Para registrar a curva de descarga da célula e diagnosticar a integridade da bateria em tempo de execução sem danificar os pinos analógicos do ESP32-C3:

* **Atenuação da Tensão de Bateria (Divisor Resistivo R1/R2):**
  * O barramento +VBAT_2S (variando entre 6,0 V descarregada e 8,4 V em carga plena) passa por um divisor resistivo formado por resistores de precisão com $R_1 = 100\text{ k}\Omega$ e $R_2 = 33\text{ k}\Omega$.
  * A tensão atenuada entregue ao pino ADC do ESP32-C3 é dada por:

$$V_{\text{ADC}} = V_{\text{BAT}} \cdot \left(\frac{R_2}{R_1 + R_2}\right) = V_{\text{BAT}} \cdot \left(\frac{33}{100 + 33}\right) \approx V_{\text{BAT}} \cdot 0,248$$

Sob tensão máxima de 8,4 V, o nível atenuado resultante é de aproximadamente 2,08 V, operando na faixa linear do conversor analógico-digital da placa.

* **Rotinas de Aquisição e Proteção:**
  * O firmware realiza leituras periódicas do canal ADC com filtro digital de média móvel para rejeitar flutuações induzidas pelo chaveamento PWM do driver.
  * O software implementa proteção de subtensão lógica (*Under-Voltage Lockout*): se a leitura atestar tensão global inferior a 6,4 V (equivalente a 3,2 V por célula) em amostragens sucessivas, os sinais de controle PWM para o DRV8833 são zerados, cortando a tração e preservando a bateria contra descarga profunda.
  * A telemetria empacota o tempo contínuo de atividade e a leitura instantânea de tensão, registrando os dados de consumo em tempo real para contraste com o modelo de energia projetado.

---

## 7. Referências Bibliográficas

1. **SHEPHERD, C. M.** *Design of Primary and Secondary Cells - Part 2: An equation describing battery discharge*. Journal of The Electrochemical Society, v. 112, n. 7, p. 657-664, 1965.
2. **TREMBLAY, Olivier; DESSAINT, Louis-A.** *Experimental Validation of a Battery Dynamic Model for EV Applications*. World Electric Vehicle Journal, v. 3, n. 2, p. 289-298, 2009.
3. **TATTU.** *Tattu 650mAh 75C 2S1P Lipo Battery Pack with XT30 Plug Specification Sheet*. Gens Ace & Tattu Battery Co., 2023.
4. **ESPRESSIF SYSTEMS.** *ESP32-C3-MINI-1 Datasheet*. Versão 1.4, 2024. Disponível em: <https://www.espressif.com>.
5. **MOHAN, Ned; UNDELAND, Tore M.; ROBBINS, William P.** *Power Electronics: Converters, Applications, and Design*. 3. ed. Nova York: John Wiley & Sons, 2003.

---

## Histórico de Versão

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
| :---: | :---: | :--- | :--- | :--- |
| 1.0 | 05/10/2026 | Estruturação inicial do consumo de energia e dimensionamento da bateria LiPo 2S | Bruno Duarte e Paulo Melo | Equipe Micromouse |
| 1.1 | 18/10/2026 | Mapeamento matemático da curva de descarga da bateria LiPo 2S (Equação de Shepherd), gráfico vetorial de descarga e geração da tabela SoC para o firmware (#139) | Elias Oliveira | Bruno Duarte (System Architect) |
