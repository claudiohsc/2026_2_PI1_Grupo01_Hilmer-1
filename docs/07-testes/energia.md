# Testes de Energia

>Apresentar com detalhes os testes feitos para conferir se as demandas energéticas do projeto foram atendidas.

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 2.1 | Ensaio de corte por subtensão (BMS 2S / UVLO) sob rampa decrescente na fonte ajustável ($8{,}4\text{ V} \to 6{,}0\text{ V}$) — [#207](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/207) | 17/10/2026 | 17/10/2026 | Paulo Melo | Corte primário UVLO em $6{,}60\text{ V}$ (protegendo *dropout* do LM2596) e corte físico BMS 2S ao atingir $6{,}40\text{ V}$ ($3{,}20\text{ V/célula}$) | Corte UVLO atuou em $6{,}60\text{ V}$ ($V_{\text{ADC}}=1{,}638\text{ V}$, PWM=0%) e corte BMS 2S seccionou `P+` em $6{,}40\text{ V}$ ($t_{\text{delay}}=98\text{ ms}$) | S | — | Detalhado em [4.2.2 Ensaios de subtensão e PPTC](../04-projeto-conceitual/ensaios-protecoes.md) |
| 2.2 | Ensaio de velocidade de desarme (*Time-to-Trip*) do fusível rearmável PPTC (Bourns MF-R185) sob sobrecorrente simulada $> 3{,}5\text{ A}$ — [#207](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/207) | 17/10/2026 | 17/10/2026 | Paulo Melo | Sustentação contínua em $1{,}85\text{ A}$ ($I_{\text{hold}}$) e desarme térmico comprovado acima de $3{,}5\text{ A}$ | Estável sem desarme em $1{,}85\text{ A}$; desarme em $9{,}10\text{ s}$ (@ $3{,}60\text{ A}$), $8{,}70\text{ s}$ (@ $3{,}70\text{ A}$), $3{,}10\text{ s}$ (@ $5{,}00\text{ A}$) e $0{,}60\text{ s}$ (@ $9{,}25\text{ A}$), com $I_{\text{res}}=118\text{ mA}$ | S | — | Detalhado em [4.2.2 Ensaios de subtensão e PPTC](../04-projeto-conceitual/ensaios-protecoes.md) |
| 2.3 | [Inspeção e Carga de Bateria LiPo 2S em Bancada](energia/teste-componentes/bateria/teste-bancada-bateria.md) | 21/10/2026 | 21/10/2026 | Equipe de Energia | Setup instrumental calibrado e bateria carregada a 8,4 V | Setup montado com iMAX B6 V3, fonte Yaxun e multímetro; bateria LiPo 2S inspecionada, balanceada e carregada a 8,40 V | S | S | Issue #201 concluída |
| 2.4 | Ensaio de autonomia contínua da bateria LiPo 2S em bancada — [#209](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/209) | 24/10/2026 | 24/10/2026 | Paulo Melo | Autonomia $\ge 25\text{ min}$ com tensão $> 6{,}4\text{ V}$ sob corrente representativa ($\ge 0{,}466\text{ A}$) | Autonomia de $34\text{ min }49\text{ s}$ comprovada sob descarga de $0{,}90\text{ A}$, mantendo $7{,}43\text{ V}$ ($208\text{ mAh}$ consumidos) | S | — | Detalhado abaixo |
| 2.5 | | | | | | | | | |
| 2.6 | | | | | | | | | |
| 2.7 | | | | | | | | | |
| 2.8 | | | | | | | | | |
| 2.9 | | | | | | | | | |
| 2.10 | | | | | | | | | |
| 2.12 | | | | | | | | | |
| 2.13 | | | | | | | | | |
| 2.15 | | | | | | | | | |
| 2.16 | | | | | | | | | |

---

## Procedimento de Teste 2.4: Ensaio de Autonomia em Bancada (Issue #209)

### 1. Parâmetros do Ensaio
Validação experimental da autonomia da bateria LiPo 2S sob descarga contínua controlada no descarregador digital **iMAX B6 V3**, visando atender ao requisito **RNF-04** (autonomia suficiente para completar os 3 labirintos consecutivos de 10 minutos, totalizando 30 minutos).

* **Consumo médio estimado do Micromouse:** $\approx 0{,}466\text{ A}$ (cargas lógicas, sensores e motores em regime de cruzeiro).
* **Corrente de ensaio aplicada:** **$0{,}90\text{ A}$** (condição de sobrecarga intencional, $\approx 1{,}93\times$ o consumo nominal).
* **Critérios de aprovação:** Autonomia $\ge 25\text{ min}$ e tensão terminal mantida acima do corte seguro ($> 6{,}40\text{ V}$).

### 2. Evidência Experimental e Medições

<figure markdown>

![Registro do Ensaio de Autonomia no Carregador iMAX B6 V3](../figs/ensaio-autonomia-b6v3.png){ width="500" }

<figcaption>

**Figura 1.** Display do descarregador iMAX B6 V3 ao término da prova (`Li2S DSC`): 34 min 49 s sob 0,90 A contínuos, com tensão estabilizada em 7,43 V e 208 mAh drenados.

</figcaption>

</figure>

| Parâmetro Registrado | Valor Medido | Requisito / Limite do Projeto | Avaliação |
| :--- | :---: | :---: | :---: |
| **Modo de Ensaio** | `Li2S` / `DSC` (Descarga) | Descarga com controle de corrente constante | Conforme |
| **Corrente Aplicada** | **$0{,}90\text{ A}$** | Sobrecarga em relação ao consumo do robô ($0{,}466\text{ A}$) | Conforme (Estresse) |
| **Tempo Transcorrido** | **$34\text{ min }49\text{ s}$** | Mínimo de 25 min (30 min regulamentares) | **Aprovado** |
| **Tensão Final Registrada** | **$7{,}43\text{ V}$** ($3{,}715\text{ V/cél}$) | Teto de corte seguro: $> 6{,}40\text{ V}$ (corte UVLO em 6,60 V) | **Aprovado (Folga de +1,03 V)** |
| **Capacidade Descarregada** | **$208\text{ mAh}$** | Capacidade nominal do pack: $650\text{ mAh}$ | Conforme (~32% consumidos) |

---

## Histórico de Versão

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
| :---: | :---: | :--- | :--- | :--- |
| 1.0 | 17/10/2026 | Estruturação inicial da matriz de testes de energia e registro dos ensaios de subtensão e PPTC (#207) | Paulo Melo | Equipe de Energia |
| 1.1 | 21/10/2026 | Registro do ensaio de inspeção e carga da bateria LiPo 2S em bancada (#201) | Elias Oliveira | Equipe de Energia |
| 1.2 | 24/10/2026 | Registro do ensaio de autonomia contínua em bancada com iMAX B6 V3 (#209) | Paulo Melo | Equipe de Energia |

