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

* **Modelo Comercial Homologado:** Bateria Li-ion / LiPo 2S de **450 mAh a 600 mAh** com taxa de descarga contínua entre **20C e 30C**.
* **Justificativa Técnica:** Essa capacidade garante fornecimento contínuo de pico de até 9 A sem colapsar a tensão do barramento (*voltage sag*), assegura peso baixo compatível com o chassi (30 g a 45 g) e estende o tempo de teste contínuo em bancada para mais de 35 minutos sem interrupção para recarga.

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
