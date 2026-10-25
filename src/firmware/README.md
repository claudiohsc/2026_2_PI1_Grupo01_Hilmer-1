# Firmware

Firmware do micromouse para o ESP32-C3 (Arduino Core, C++17), organizado nas camadas *sense-think-act* definidas na [arquitetura de software](../../docs/04-projeto-conceitual/software.md). O projeto usa [PlatformIO](https://platformio.org/).

A documentação detalhada do código (camadas, tipos, pinagem, interface do LiDAR, tarefas e testes) está em [Firmware: implementação](../../docs/04-projeto-conceitual/firmware.md). Os arquivos `.h` têm comentários de documentação no formato Doxygen (`///`).

## Estrutura

```
src/firmware/
├── platformio.ini     # ambientes esp32c3 (placa), native (testes no computador), native_cobertura (testes com cobertura, usado no CI), bancada_tof e bancada_deslocamento (firmwares de bancada)
├── src/main.cpp       # ponto de entrada: cria as tarefas FreeRTOS (navegação e telemetria)
├── src/bancada/       # programas de bancada, fora do firmware do robô
├── lib/               # uma biblioteca por camada
├── scripts/           # ferramentas de bancada no computador (Python)
└── test/              # testes unitários e de integração (Unity)
```

| Pasta em `lib/` | Camada | O que entra |
|---|---|---|
| `nucleo` | Base | Tipos compartilhados (`Direcao`, `Lado`, `EstadoParede`, `PosicaoCelula`, `Pose`, `DistanciasLaterais`), constantes e pinagem |
| `hal` | Interfaces de hardware | Interfaces abstratas (`ISensorDistancia`, `IEncoder`, `IMotores`, `IBarramentoI2c` e `IImu`) |
| `percepcao` | Percepção | Classificação `parede`/`livre` para os 3 sensores ToF (VL53L0X) com limiares por lado, filtro digital e saturação defensiva (#379) |
| `inicializacao` | Boot | Autoteste do boot (`AutotesteBoot`): varredura do I²C, conferência dos 4 dispositivos e relatório (`RelatorioBoot`), com os parâmetros em `ConfiguracaoBoot.h` |
| `tof` | Sensoriamento | Regra de validade das leituras dos 3 VL53L0X (`CombinadorTof`) e seus parâmetros provisórios (`ConfiguracaoTof.h`), sem Arduino |
| `imu` | Sensoriamento | Giroscópio: conversão da leitura do MPU6050, calibração do viés, ângulo de giro (`IntegradorGiro`) e o ciclo da tarefa da IMU (`AcompanhadorGiro`) |
| `deslocamento` | Sensoriamento | Deslocamento sem encoder: estimativa pela potência dos motores (`EncoderEstimado`, `MotoresComEstimativa`) e correção pelos ToF; todos os ajustes em `ConfiguracaoDeslocamento.h` |
| `encoders` | Sensoriamento | Decodificação em quadratura (x4) dos canais A e B, conversão de pulsos em milímetros (calibrável) e distância percorrida por cada roda |
| `lidar` | Drivers / Hardware | Driver de leitura do sensor LiDAR 360° via UART (decodificador LD06) e agregação de leituras por setores |
| `mapeamento` | Mapeamento | Mapa do labirinto (4 paredes por célula) e armazenamento em memória |
| `navegacao` | Navegação | Localização (`Odometria`: pose pela odometria diferencial dos encoders), decisão de movimento (`Navegacao`) e detecção do objetivo (`DetectorObjetivo`) |
| `atuacao` | Atuação | Controle dos motores, correção de trajetória e rumo pelo giroscópio, giros de 90° e 180° em malha fechada (`ControleGiro`) e trava dos motores (`MotoresComTrava`) (#190, #191) |
| `comunicacao` | Comunicação | Pacotes de telemetria (`TelemetriaJson`), serialização JSON do mapa (`MapaJson`) e *buffer* de reenvio |
| `simulacao` | Falsos para teste | `SensorDistanciaSimulado`, um `ISensorDistancia` que devolve as leituras configuradas pelo teste, `LabirintoSimulado`, o labirinto de referência que alimenta o `SensorDistanciaSimulado`, `TofSimulado`, os 3 VL53L0X no labirinto, com amostras brutas pelo `CombinadorTof` real, `EncoderSimulado`, um `IEncoder` com a contagem definida pelo teste, `MotoresSimulados`, um `IMotores` que guarda o último comando, `BarramentoI2cSimulado`, um `IBarramentoI2c` com os dispositivos escolhidos pelo teste, e `ImuSimulado`, um `IImu` com taxa, viés, ruído e falha definidos pelo teste, sem hardware |
| `drivers_esp32` | Implementações do hardware | Drivers que dependem do Arduino (3 VL53L0X e varredura pelo I²C, MPU6050, barramento I²C com mutex, encoders, DRV8833, WiFi) |

## Regras de dependência

- As camadas de lógica (`nucleo`, `percepcao`, `mapeamento`, `navegacao`, `comunicacao`, `simulacao`) **não incluem `Arduino.h`**. Assim elas compilam e são testadas no computador, sem a placa.
- O acesso ao hardware passa pelas interfaces de `hal`. Os drivers reais ficam em `drivers_esp32`, e `src/main.cpp` liga cada driver à sua interface.
- Para testar a lógica sem sensor, use o `SensorDistanciaSimulado` (`lib/simulacao`): `definirLeituraFixa` repete uma leitura, `enfileirarLeitura` entrega leituras em sequência, `simularFalha(true)` faz o sensor parar de responder e, com um `LabirintoSimulado`, `posicionar` faz a leitura vir da posição do robô (veja [Labirinto simulado](#labirinto-simulado)).
- Evite alocação dinâmica na lógica: o maior labirinto tem 12×4 células, então estruturas de tamanho fixo (`MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO`) bastam.

## Mapa do labirinto

O `Mapa` (`lib/mapeamento`) guarda o estado das quatro paredes de cada célula. O tamanho vem do enum `DimensaoMapa` (`Labirinto4x4`, `Labirinto8x4` e `Labirinto12x4`), escrito como colunas × linhas: `Labirinto12x4` tem 12 colunas e 4 linhas. A matriz tem capacidade fixa para o maior labirinto, sem alocação dinâmica, e o objeto ocupa 195 bytes.

**Eixos:** a partida é a célula (0, 0), no canto inferior esquerdo; Norte aumenta a linha e Leste aumenta a coluna. `vizinha()` (`nucleo/Direcao.h`) calcula a célula ao lado de uma posição.

```cpp
Mapa mapa(DimensaoMapa::Labirinto12x4);
mapa.registrarParede({0, 0}, Direcao::Norte, EstadoParede::Parede);
mapa.obterParede({1, 0}, Direcao::Sul);  // Parede: a célula vizinha foi atualizada junto
```

| Regra | Comportamento |
|---|---|
| Paredes ainda não observadas | `Desconhecido` |
| Perímetro | Já nasce como `Parede` |
| Registro | Grava também o lado oposto da célula vizinha |
| Estado já conhecido | `registrarParede` não sobrescreve: retorna `JaConhecida` (mesmo valor) ou `Conflito` (valor diferente); a política para o conflito fica com quem chama |
| Entrada inválida | `Invalida`: célula fora do mapa ou estado `Desconhecido` |
| Comparação | `==` compara o tamanho e todas as paredes |

### Armazenamento do mapa

O `ArmazenamentoMapa` (`lib/mapeamento`) é o dono do único `Mapa` da corrida. Ele fica em `src/main.cpp`, como variável estática fora das tarefas, para que reiniciar o estado de navegação (por exemplo, após uma colisão) ou perder a conexão não apague o mapa (HU-04).

```cpp
armazenamentoMapa.iniciarCorrida(DimensaoMapa::Labirinto12x4);  // único jeito de zerar o mapa
Mapa& paraNavegacao = armazenamentoMapa.mapa();                  // lê e registra paredes
const Mapa& paraTelemetria = armazenamentoMapa.mapa();           // só lê (pelo acesso const)
```

O armazenamento não pode ser copiado, e o acesso `const` impede que a telemetria altere o mapa; as duas regras são conferidas em tempo de compilação. Medido no ESP32-C3, o objeto ocupa **196 bytes** de RAM estática (`.bss`): 195 do mapa e 1 do indicador de corrida.

## Labirinto simulado

O `LabirintoSimulado` (`lib/simulacao`) é um labirinto com todas as paredes conhecidas. Com ele, o `SensorDistanciaSimulado` devolve o que o sensor real veria na posição do robô, sem digitar leituras à mão. Serve para testar a classificação de paredes e a atualização do mapa num labirinto inteiro.

**Formato do texto.** O labirinto é escrito em ASCII: cada célula ocupa 4 colunas e 2 linhas de texto, `+` marca os cantos, `---` uma parede horizontal, `|` uma parede vertical e espaço é passagem livre. A primeira linha é a borda Norte e a partida (0, 0) é a célula do canto inferior esquerdo, como no `Mapa`. Cada parede aparece uma única vez, então as duas células vizinhas sempre concordam. Há um exemplo de cada tamanho (4×4, 8×4 e 12×4) em `test/test_labirinto_simulado/LabirintosExemplo.h`.

```cpp
LabirintoSimulado labirinto;
labirinto.carregar(exemplos::LABIRINTO_4X4);   // ResultadoCarga::Carregado
labirinto.temParede({0, 0}, Direcao::Leste);   // true

SensorDistanciaSimulado lidar;
lidar.usarLabirinto(labirinto);                // o labirinto precisa continuar vivo
lidar.posicionar({{0, 0}, Direcao::Norte});
DistanciasLaterais d;
lidar.lerDistanciasLaterais(d);                // d = {630, 90, 90} (frente, esquerda, direita, em mm)
```

**Distâncias.** O robô fica no centro da célula: a parede da própria célula está a 90 mm e cada célula livre no caminho soma 180 mm. Com o robô virado para Norte em (0, 0) do 4×4 há três células livres à frente, ou seja, 90 + 3 × 180 = 630 mm.

| Operação | O que faz |
|---|---|
| `carregar(texto)` | Lê o labirinto. Se o texto for inválido, devolve o motivo (`TamanhoInvalido`, `CaractereInvalido`, `ParedeIncompleta` ou `PerimetroAberto`) e mantém o labirinto anterior |
| `temParede(celula, direcao)` | `true` se há parede naquele lado; célula fora do labirinto conta como parede |
| `distancias(pose, saida)` | Distâncias à frente, à esquerda e à direita para a pose; `false` sem carga ou com a célula fora do labirinto |
| `SensorDistanciaSimulado::usarLabirinto` e `posicionar` | Liga o modo labirinto. A falha e a fila continuam com prioridade; sem `posicionar`, vale a leitura fixa; com a pose fora do labirinto, a leitura falha |

## Percepção ToF (VL53L0X) e classificação de paredes

A camada de percepção (`lib/percepcao`, `ClassificadorParede`) processa as leituras de distância dos 3 sensores Time-of-Flight VL53L0X (frontal, esquerdo e direito, I²C) introduzidos na Rev 2.0 em substituição ao LiDAR ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379), HU-02).

```cpp
ClassificadorParede classificador; // Inicializa com ConfiguracaoPercepcao padrão
DistanciasLaterais leitura = {54, 250, 52}; // Leitura dos ToF em mm
ClassificadorParede::ResultadoClassificacao paredes = classificador.classificar(leitura);
// paredes.frente: Parede, paredes.esquerda: Livre, paredes.direita: Parede
```

### Geometria dos sensores e dedução dos limiares

No labirinto da competição, a célula mede 180 mm (`TAMANHO_CELULA_MM`) e as paredes têm 12 mm de espessura (`ESPESSURA_PAREDE_MM`), resultando em um corredor livre de 168 mm. Com o robô centralizado, a distância do centro do robô à parede é de **84 mm** (`DISTANCIA_CENTRO_A_PAREDE_MM`).

O chassi possui 120 mm de largura (meia largura: 60 mm) e 100 mm de comprimento (meio comprimento: 50 mm). Se os sensores laterais fossem montados na borda externa do chassi, a distância até a parede seria de apenas $84 - 60 = 24\text{ mm}$, valor que cai abaixo do limite operacional confiável do VL53L0X (~30 mm). Portanto, a montagem física exige sensores recuados em relação à borda.

Para um sensor com offset $d_{\text{offset}}$ em relação ao centro do robô na direção da medição:
- **Distância nominal com parede imediata:** $d_{\text{parede}} = 84 - d_{\text{offset}}$
- **Distância nominal com passagem livre (parede na célula seguinte):** $d_{\text{livre}} = d_{\text{parede}} + 180\text{ mm}$
- **Limiar de decisão ideal (ponto médio):** $\text{limiar} = d_{\text{parede}} + 90\text{ mm}$

Adotando provisoriamente um offset de montagem de $30\text{ mm}$ (sensores recuados 30 mm para o interior do chassi):
- $d_{\text{parede}} = 84 - 30 = 54\text{ mm}$ (com boa margem acima dos 30 mm mínimos do VL53L0X).
- $d_{\text{livre}} = 54 + 180 = 234\text{ mm}$ (ou fora de alcance em corredores abertos).
- $\text{Limiar ideal} = 54 + 90 = 144\text{ mm}$, adotado provisoriamente como **140 mm**.

O valor de 140 mm também é 100% compatível com o ambiente simulado (`LabirintoSimulado`), onde paredes imediatas medem 90 mm ($\le 140\text{ mm} \to \text{Parede}$) e passagens livres medem $\ge 270\text{ mm}$ ($> 140\text{ mm} \to \text{Livre}$).

### Valores provisórios de configuração (`ConfiguracaoPercepcao.h`)

| Parâmetro | Constante | Valor provisório | Origem / Justificativa |
|---|---|---|---|
| Limiar frontal | `limiarFrenteMm` | **140 mm** | Ponto de decisão para offset frontal de 30 mm (centro da célula: 54 mm vs 234 mm) |
| Limiar esquerdo | `limiarEsquerdaMm` | **140 mm** | Ponto de decisão para offset lateral de 30 mm (robô centralizado: 54 mm vs 234 mm) |
| Limiar direito | `limiarDireitaMm` | **140 mm** | Ponto de decisão para offset lateral de 30 mm (robô centralizado: 54 mm vs 234 mm) |
| Distância mínima | `distanciaMinimaMm` | **30 mm** | Limite operacional inferior do VL53L0X (datasheet ST DocID029104) |
| Distância máxima | `distanciaMaximaMm` | **1200 mm** | Alcance máximo confiável em ambiente interno sem alvos difusos de alta refletância |
| Código fora de alcance | `DISTANCIA_OUT_OF_RANGE_TOF_MM` | **8190 mm** | Código retornado pelo firmware/hardware VL53L0X na ausência de eco de retorno |
| Tipo de filtro | `tipoFiltro` | **`Mediana`** | Mediana sobre janela de até 16 amostras, imune a outliers pontuais |

> [!WARNING]
> **Valores provisórios:** os limiares e offsets acima foram calculados teoricamente a partir das dimensões do labirinto e do esquemático Rev 2.0. A calibração final e o ajuste dos offsets dependem da medição na bancada com o hardware real montado ([#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175)).

### Tratamento defensivo de limites e filtro de ruído

- **"Fora de alcance" ($\ge 8190\text{ mm}$ ou $> 1200\text{ mm}$):** interpretado como lado livre. A função `tratarDistancia()` satura a medida em 1200 mm, garantindo que o valor fique estritamente acima do limiar ($\to \text{Livre}$).
- **"Abaixo do mínimo" ($< 30\text{ mm}$):** interpretado como parede muito próxima (ou robô encostado na parede). A função satura a medida em 30 mm, garantindo que o valor fique estritamente abaixo do limiar ($\to \text{Parede}$).
- **Filtro digital:** o método `classificar(amostras, quantidade)` filtra um lote de amostras antes da decisão. O filtro padrão por **mediana** descarta outliers pontuais (como um falso fora de alcance de 8190 mm durante a passagem por uma parede ou uma queda espúria momentânea para 0 mm num corredor aberto), mantendo a integridade do mapa.

## Controle de movimento

O `ControleMovimento` e o `ControleGiro` (`lib/atuacao`) controlam a trajetória e orientação do robô no labirinto: avanço retilíneo centralizado no corredor com parada frontal ([#191](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/191), HU-12), correção contínua do desvio de rumo pelo giroscópio e giros de 90° e 180° no próprio eixo em malha fechada ([#190](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/190), HU-11).

```cpp
// Avanço com centralização lateral e correção de rumo pelo giroscópio:
ControleMovimento controle;
controle.definirRumoAlvo(0); // Rumo reto na partida (0 m°)
EstadoMovimento estado = controle.atualizar(lidar, motores, 100, leituraGiro);
if (estado == EstadoMovimento::ParadoNaParede) {
  // Giro de 90° para a direita em malha fechada:
  ControleGiro giro;
  giro.iniciarGiro90Direita(leituraGiro.anguloMiligraus);
  while (!giro.concluido()) {
    giro.atualizar(leituraGiro, motores, 10);
  }
  controle.reiniciar(giro.anguloAlvoMiligraus()); // Novo rumo alvo travado
}
```

| Regra | Comportamento |
|---|---|
| Centralização lateral | PD sobre o deslocamento lateral: `(esquerda - direita) / 2` com parede dos dois lados, ou a diferença para 84 mm com uma só; sem parede ao lado, segue reto |
| Correção de rumo | PD sobre o erro angular de guinada: $e = \text{rumoAlvo} - \text{anguloAtual}$. Em trechos abertos (`SemReferencia`), o giroscópio é a referência que mantém a linha reta; em corredores com paredes, atua como amortecimento |
| Correção total | Soma da centralização lateral com a correção de rumo, limitada a `correcaoMaximaPercentualDaBase` (60%) da potência base, impedindo que qualquer roda inverta durante cruzeiro |
| Parada frontal | Cruzeiro (400) até 264 mm da parede, rampa até a mínima (150) e parada a 84 mm, o centro da célula |
| Giros de 90° e 180° | Rotação diferencial no lugar em malha fechada sobre o ângulo integrado (`ControleGiro`): rampa proporcional com desaceleração suave até a potência mínima (150), parada com freio ao atingir a tolerância configurável (padrão $\pm 2,0^\circ$) e confirmação por ciclos de estabilização |
| Deslizamento do piso | Sem encoders nas rodas na Rev 2.0, a malha fechada no ângulo do giroscópio compensa escorregamento dos pneus e desníveis do solo (HU-11), mantendo giros precisos e avanço retilíneo |
| Parado / Falha | `ParadoNaParede` e `ParadoSemLeitura` freiam até `reiniciar()`; timeout de giro acusa `Timeout` e freia |

### Parâmetros provisórios de giro e rumo (`ConfiguracaoGiro.h` e `ConfiguracaoControle.h`)

| Parâmetro | Constante / Campo | Valor provisório | Origem / Justificativa |
|---|---|---|---|
| Tolerância do giro | `toleranciaMiligraus` | **2 000 m° (2,0°)** | Erro angular admissível para o robô sair alinhado na célula seguinte |
| Potência máxima de giro | `potenciaMaxima` | **350** | Limita a aceleração angular para evitar derrapagem violenta |
| Potência mínima de giro | `potenciaMinima` | **150** | Vence a zona morta de atrito estático dos micromotores N20 |
| Ganho proporcional do giro | `ganhoProporcionalDecimos` | **40 (4,0 pot/°)** | Desacelera a rotação conforme o robô se aproxima do alvo angular |
| Timeout do giro | `tempoMaximoGiroMs` | **2 500 ms** | Fail-safe caso o robô colida ou fique preso durante a rotação |
| Ciclos de estabilização | `ciclosEstabilizacao` | **3 ciclos (30 ms)** | Impede falsos positivos causados por simples cruzamento rápido da meta |
| Ganho proporcional de rumo | `ganhoRumoProporcionalDecimos` | **80 (8,0 pot/°)** | Corrige desvios angulares durante o avanço retilíneo |
| Ganho derivativo de rumo | `ganhoRumoDerivativoDecimos` | **20 (2,0 pot/(°/s))** | Amortece a taxa de rotação para evitar serpentear no corredor |
| Correção máxima de rumo | `correcaoRumoMaxima` | **200** | Limita a autoridade de correção angular sobre os motores |

O `MotoresDrv8833` (`lib/drivers_esp32`) aplica a potência com PWM de 20 kHz e 10 bits nos GPIO 4 a 7, no modo *fast decay*; `parar()` liga IN1 e IN2 (freio). O motor direito fica invertido no `main.cpp`, pela montagem espelhada.

## Deslocamento sem encoder

Na Rev 2.0 os encoders ainda não estão instalados. Enquanto isso, o deslocamento é **estimado pela potência comandada vezes o tempo** e **corrigido pelos ToF** ([#380](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/380)). O `EncoderEstimado` implementa o `IEncoder`, então a `Odometria` não muda; o `MotoresComEstimativa` implementa o `IMotores`, então quem aciona as rodas também não.

```cpp
DeslocamentoEstimado deslocamento(motoresDrv8833);
IMotores& motores = deslocamento.motores();      // quem aciona as rodas usa este, não o driver
// a cada 10 ms, na tarefa de navegação:
deslocamento.ciclo(10, odometria, leituraNova ? &leitura : nullptr);
bool poseValida = odometria.posicaoValida() && deslocamento.poseConfiavel();
```

| Regra | Comportamento |
|---|---|
| Estimativa | Cada roda anda a velocidade da `TABELA_VELOCIDADE` para a potência comandada vezes o tempo; abaixo do primeiro ponto, fica parada (zona morta) |
| Correção pela frente | Com parede a até 300 mm do sensor frontal, a posição é ajustada pela distância à parede |
| Correção pelas laterais | Quando um lado passa de parede para livre (ou o contrário) com o robô avançando, a posição é ajustada para a quina da parede, na fronteira de célula |
| Eixo corrigido | Só quanto o robô andou; o desvio lateral e o rumo são da #188 e da #190 |
| Proteções | Correção acima de 70 mm é descartada; durante um giro, o ToF não corrige |
| `pose_valida` | Fica `false` depois de 360 mm sem nenhuma correção, até a próxima |
| Telemetria | Campos `deslocamento` (`"estimado"` ou `"encoder"`) e `sem_correcao_mm`. O comando `Z` reposiciona o robô no centro da célula atual e reinicia o `DetectorObjetivo` |
| Objetivo | O `DetectorObjetivo` freia por `deslocamento.motores()`, para o estimador saber que o robô parou, e só declara a chegada com `poseConfiavel()` |
| Troca para o encoder real | `ENCODERS_INSTALADOS = true` em `ConfiguracaoDeslocamento.h`; nada mais muda |

### Limites deste modo

- **Mais deriva:** patinagem, bateria fraca, piso diferente ou uma tabela mal calibrada viram erro de posição, que só some na próxima referência do ToF (parede à frente ou quina lateral).
- **Sem velocidade medida:** a velocidade é a que a tabela supõe. Um motor travado ou uma roda patinando não aparecem na estimativa.
- **Sem inércia:** a roda para no instante do comando.
- **Giro grosseiro:** o giro vem da diferença de potência entre as rodas; o rumo confiável é o do giroscópio (#190).
- **Corredores longos:** enquanto a #379 não tratar "fora de alcance" como livre, uma leitura com um lado aberto para mais de 1,2 m é recusada e não corrige (8×4 e 12×4).

### Valores provisórios

Todos ficam em **`lib/deslocamento/src/ConfiguracaoDeslocamento.h`**, e cada um diz o que faz, de onde veio e como medir:

| Valor | Hoje | Como medir na bancada |
|---|---|---|
| `TABELA_VELOCIDADE` | 150 → 45, 400 → 120, 700 → 210, 1000 → 300 (potência → mm/s) | Etapa 2 da bancada abaixo: 2 s por ponto, medidos na régua; o menu imprime a tabela pronta para colar. Só com o limite de duty de 70% da #341 valendo, senão os mm/s ficam acima dos reais |
| `DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM` | 50 | Régua do centro entre as rodas até a face do sensor frontal |
| `AVANCO_SENSORES_LATERAIS_MM` | 0 | Régua do centro até o eixo dos sensores laterais, para a frente |
| `DISTANCIA_FRONTAL_MAXIMA_PARA_CORRIGIR_MM` | 300 | Até onde o erro do sensor frontal fica pequeno (#175) |
| `LIMIAR_PAREDE_LATERAL_MM` / `HISTERESE_PAREDE_LATERAL_MM` | 120 / 40 | Usar os limiares da #379 |
| `CORRECAO_MAXIMA_MM` | 70 | Maior que a deriva entre duas referências, menor que meia célula |
| `DISTANCIA_MAXIMA_SEM_CORRECAO_MM` | 360 | Quanto o robô anda antes de errar a célula; aumentar com a tabela calibrada |

### Bancada do deslocamento sem encoder

Para conferir a estimativa com o robô montado (motores e ToF), use o menu de bancada (precisa de `pip install pyserial`):

```bash
./scripts/bancada_deslocamento.py          # escolhe a porta; ou --porta /dev/ttyACM0 (COM5 no Windows)
```

**ATENÇÃO:** as etapas acionam os motores. Robô no chão, com espaço livre. **Ctrl+C freia na hora**, e cada movimento dura no máximo 10 s.

1. **Opção 1:** grava o firmware de bancada (`pio run -e bancada_deslocamento -t upload`).
2. **Opção 3:** roteiro completo. Em cada etapa o menu explica o que montar, aciona o robô e mostra o log ao vivo (posição estimada, célula, `pose_valida`, distância sem correção, ToF e cada correção aplicada), pede a medida da régua e dá APROVADO, REPROVADO ou OBSERVADO.
3. **Logs:** cada movimento vira um CSV numa pasta `bancada_deslocamento_<data>/`, junto com o `relatorio.md`, para registrar em `docs/07-testes/software.md`.
4. Ao terminar, grave de volta o firmware do robô: `pio run -e esp32c3 -t upload`.

| Etapa | O que verifica |
|---|---|
| 1. Estado inicial | Modo estimado, ToF iniciados e pose (0, 0) Norte confiável |
| 2. Tabela de velocidade | Zona morta e cada ponto da tabela contra a régua (±10%); imprime a tabela medida para colar em `ConfiguracaoDeslocamento.h` |
| 3. Uma célula | ~180 mm sem correção: erro até ±18 mm e célula estimada certa |
| 4. Corredor com ToF | ~450 mm num corredor com parede no fim e uma abertura lateral: correção aplicada, erro até ±15 mm e pose válida |
| 5. Confiança | ~450 mm sem correção: `pose_valida` falso; `ZERAR` devolve a confiança |
| 6. Giro de 90° | Direção estimada Leste e giro real de 90 ± 20° |

Há ainda **movimento livre** (potência, duração e correção à escolha, com o log ao vivo) e **ver o estado atual**. Os critérios ficam no topo do script e são provisórios. O firmware também funciona direto no `pio device monitor -e bancada_deslocamento`: `AJUDA` lista `INFO`, `VEL`, `ZERAR`, `CORRECAO`, `ANDAR`, `GIRAR` e `LER`.

## Detecção do objetivo e conclusão do desafio

O `DetectorObjetivo` (`lib/navegacao`) identifica o instante em que o robô chega à célula de destino (canto diametralmente oposto à partida) a partir da pose estimada pela odometria ([#189](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/189), HU-07 / RF-07):

```cpp
DetectorObjetivo detector(DimensaoMapa::Labirinto4x4); // Objetivo em (3, 3)
// A cada ciclo da tarefa de navegação:
if (detector.processarChegada(pose, motores, &armazenamentoMapa)) {
  // Motores freados imediatamente e mapa final congelado
}
```

| Critério de aceite | Solução implementada |
|---|---|
| Chegada detectada na malha | `DetectorObjetivo::calcularObjetivo` determina a célula oposta (`linhas - 1, colunas - 1`) para 4x4 (`(3, 3)`), 8x4 (`(3, 7)`), 12x4 (`(3, 11)`) ou dimensões customizadas, comparando com a célula atual estimada pela odometria |
| Motores parados imediatamente | Ao detectar a chegada, aciona `motores.parar()` (freio elétrico instantâneo via DRV8833) e congela o estado de navegação |
| Congelamento do mapa final | `ArmazenamentoMapa::congelarMapa()` salva a imagem imutável das paredes exploradas para telemetria e persistência (HU-04) |
| Evento pontual na telemetria | `DetectorObjetivo::consumirEventoChegada()` sinaliza e consome a chegada pontual, fazendo `TelemetriaJson` incluir `"evento": "objetivo_alcancado"` em um único pacote serializado após a chegada, voltando a `null` nos pacotes subsequentes |

> [!NOTE]
> **Dimensão do labirinto em `main.cpp`:** a inicialização com `Labirinto4x4` no `main.cpp` (tanto em `detectorObjetivo` quanto em `armazenamentoMapa.iniciarCorrida`) é **provisória** para bancada e testes iniciais. Em labirintos 8x4 ou 12x4 a chegada só é detectada se o tamanho correto for configurado. A seleção definitiva do tamanho será recebida dinamicamente no início da corrida.
>
> **Sincronização entre tarefas:** `processarChegada`, `consumirEventoChegada` e `resetar` acessam o estado de detecção e odometria sob a seção crítica `travaOdometria`, garantindo thread-safety entre `tarefaNavegacao` (10 ms) e `tarefaTelemetria` (100 ms).
>
> **Arquitetura dos detectores:** atualmente coexistem duas instâncias (`Navegacao::detector_` para planejamento BFS/Flood Fill e `detectorObjetivo` em `main.cpp` para o laço do firmware). Ambas serão unificadas e a API de construtores será simplificada quando `Navegacao` for incorporada diretamente ao laço da tarefa FreeRTOS.

### Telemetria e compatibilidade com o banco de dados

O serializador `TelemetriaJson` (`lib/comunicacao`) gera payloads compatíveis com a tabela `telemetria` do Supabase e diagnósticos de bancada:

| Campo JSON | Tabela `telemetria` (Supabase) | Descrição |
|---|---|---|
| `numero_sequencia` | `numero_sequencia` (INT) | Sequencial monotônico incremental |
| `posicao_linha` | `posicao_linha` (INT) | Coordenada atual da linha na malha |
| `posicao_coluna` | `posicao_coluna` (INT) | Coordenada atual da coluna na malha |
| `orientacao` | `orientacao` (VARCHAR(2)) | Letra da direção: "N", "S", "L", "O" |
| `velocidade` | `velocidade` (FLOAT) | Velocidade linear instantânea (m/s) |
| `tensao_bateria` | `tensao_bateria` (FLOAT) | Tensão instantânea da bateria (V) |
| `evento` | `evento` (VARCHAR(30)) | Evento pontual (`"objetivo_alcancado"`, `"colisao"` ou `null`) |

Campos diagnósticos adicionais (`esq_pulsos`, `dir_pulsos`, `esq_mm`, `dir_mm`, `medio_mm`, `pose_valida`) são úteis para calibração de bancada e scripts de teste (como `calibrar_deslocamento_18cm.py`), e podem ser descartados pelo gateway antes da inserção no banco de dados.

## Detecção de colisão e recuperação (#192)

Se o robô bate numa parede que não viu, ele detecta a batida, recua até o centro da célula e volta a perceber e decidir, **sem perder o mapa nem a posição** da navegação (HU-17). A lógica fica em `lib/execucao` e é testada no computador; só a leitura do acelerômetro é do hardware.

| Peça | Onde | O que faz |
|---|---|---|
| `IAcelerometro` | `lib/hal` | `lerAceleracaoPlano(ax, ay)`: aceleração nos eixos X e Y do chão, em mg |
| `DetectorColisao` | `lib/execucao` | Percebe a colisão por **impacto** (pico de aceleração) ou por **frente parada** (ToF) e a avisa uma vez |
| `RegistradorPotencia` | `lib/atuacao` | Camada de fora de `motoresTracao`: guarda a última potência comandada, que a detecção compara com o movimento |
| `ExecutorMovimentos::iniciarRecuo` | `lib/execucao` | Recuo em linha reta, de ré, segurando o rumo pelo giroscópio |
| `EstadoCiclo::Recuperando` | `lib/execucao` | Etapa do `CicloNavegacao` entre a batida e a nova percepção |

**Como a colisão é percebida.** Duas evidências independentes, ligadas só com o robô sendo conduzido (avançando, girando ou realinhando; parado ou em recuperação a detecção fica desligada, para um robô erguido ou empurrado à mão não contar):

- **Impacto (IMU):** a aceleração no plano se afasta da referência de repouso por `LIMIAR_IMPACTO_MG` (1 g) ou mais. A referência é uma média lenta das leituras calmas, então a inclinação do sensor e o zero dele não contam, e um impacto não vira o "novo repouso".
- **Frente parada (ToF):** potência alta por 500 ms e nem o ToF frontal nem o giroscópio mostram movimento (a mesma regra da `ProtecaoMotorTravado`, #341). Uma frente além do alcance do ToF (leitura no máximo, 1200 mm) não conta: ela também não muda andando por um corredor longo, como o do 12×4.
- A detecção por encoder parado fica para quando os encoders existirem.

**O que o ciclo faz.** Com `EntradaCiclo::colisao` e o robô em movimento:

1. Cancela o movimento, freia e esquece o passo planejado.
2. **Recuperando:** recua até o centro da célula pela odometria (no mínimo 10 mm, no máximo 40 mm). Com a parede da frente à vista, termina antes, quando o ToF mostra o robô a 84 mm dela. O rumo alvo volta ao da direção atual, também depois de um giro interrompido.
3. Volta a **Percebendo** na mesma célula e decide de novo.

**Mapa e posição.** A célula e a direção da navegação não mudam. A única escrita no mapa é a parede da frente, e só quando a batida **se repete** avançando na mesma célula e direção, o robô está onde a batida na parede da própria célula acontece (odometria na mesma célula, a 10–70 mm além do centro) e o ToF frontal não mede a frente livre. Uma batida isolada só recua e percebe de novo: se a percepção vê a parede, o mapa a grava como sempre. Depois de `COLISOES_SEGUIDAS_MAXIMAS` (4) colisões sem concluir uma célula, o ciclo pára em `Falha` (`colisoes_seguidas`).

**Telemetria.** Cada colisão sai no log como `{"status":"colisao","tipo":"impacto","desvio_mg":...}` e como o evento `colisao` da telemetria (um evento por amostra: a chegada ao objetivo vai primeiro, e cada colisão pendente espera a sua amostra).

**Valores provisórios**, para conferir na bancada (`ConfiguracaoColisao.h` e `ConfiguracaoImu.h`):

| Parâmetro | Valor | Como ajustar |
|---|---|---|
| `LIMIAR_IMPACTO_MG` | 1000 mg | Bata o robô devagar contra a parede e veja `desvio_mg` no log; fique entre o maior desvio andando sem bater e o menor batendo |
| `FAIXA_ACELEROMETRO` | ±4 g | Se o pico saturar (4000 mg), suba a faixa |
| Filtro do sensor | 42 Hz (o mesmo do giroscópio) | Uma batida curta pode sair atenuada; se faltar sensibilidade, aumente a banda do filtro |
| `RECUO_COLISAO_MM` e `RECUO_MINIMO_COLISAO_MM` | 40 e 10 mm | Confira onde o robô para depois do recuo |
| Eixos do acelerômetro | qualquer | A detecção usa só o módulo no plano: o sinal e a orientação dos eixos não importam. O sensor tem que estar plano (a gravidade fora de X e Y) |

**Limites.** O recuo corrige a posição ao longo do eixo só até onde a odometria e o ToF frontal permitem. Depois de uma batida num giro, o rumo é refeito pelo giroscópio no giro seguinte, e não no recuo. A colisão lateral (raspar numa parede sem bater de frente) é detectada pelo impacto, mas o robô só recua e percebe de novo: não grava parede lateral.

## Sensores de distância (VL53L0X)

Na Rev 2.0 do hardware, o LiDAR deu lugar a 3 VL53L0X no I²C (esquerdo, frontal e direito). O `TofVl53l0x` (`lib/drivers_esp32`) implementa o `ISensorDistancia`, então mapa, percepção e navegação não mudaram ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344)). A regra de validade fica no `CombinadorTof` (`lib/tof`), testado no `native`; a biblioteca `pololu/VL53L0X` só entra no `esp32c3`.

```cpp
Wire.begin(pinos::I2C_SDA, pinos::I2C_SCL);   // barramento compartilhado com o MPU6050
TofVl53l0x tof(Wire, {pinos::XSHUT_FRONTAL, pinos::ENDERECO_TOF_FRONTAL},
               {pinos::XSHUT_ESQ, pinos::ENDERECO_TOF_ESQ},
               {pinos::XSHUT_DIR, pinos::ENDERECO_TOF_DIR});
tof.iniciar();                                // XSHUT um por vez e endereços 0x31, 0x30, 0x32

DistanciasLaterais d;
if (tof.lerDistanciasLaterais(d)) { /* os 3 sensores respondendo, medidas recentes e limitadas à faixa */ }
tof.amostra(Lado::Frente);                    // medida bruta, estado e instante, mesmo com a leitura recusada
```

| Regra | Comportamento |
|---|---|
| Inicialização | Os 3 XSHUT em nível baixo; cada sensor é solto, inicializado e recebe o endereço antes do próximo. Sensor que não responde fica em reset (`sensorIniciado(lado)` diz quais iniciaram) |
| Leitura | Nunca espera medida: só lê os sensores que já têm medida pronta. A leitura vale com os 3 respondendo e a amostra mais antiga com até 100 ms |
| Fora da faixa | Não recusa a leitura: a medida sai limitada à faixa, como a percepção (#379) trata. `ForaDeAlcance` (acima de 1200 mm, inclui os 8190 mm "sem alvo") vira 1200 mm, lado livre; `AbaixoDoMinimo` (menos de 30 mm) vira 30 mm, parede muito perto. O valor bruto continua em `amostra(lado)` |
| Recusa | `false` com algum sensor em `Falha`, sem nenhuma medida ou parado há mais de 100 ms |
| Idade | `idadeUltimaLeituraMs()`: idade do sensor mais atrasado, para a navegação não repetir dado antigo |
| Referência | Distância a partir da face de cada sensor; a geometria do robô e os limiares ficam na percepção ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)) |

Os parâmetros estão em `lib/tof/src/ConfiguracaoTof.h` e são **provisórios** até a bancada ([#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175)). A telemetria serial do `main.cpp` traz os campos `tof_frente_mm`, `tof_esq_mm`, `tof_dir_mm`, `tof_valida` e `tof_idade_ms`.

No computador, o `TofSimulado` (`lib/simulacao`) faz o papel do driver: gera a amostra bruta de cada sensor no `LabirintoSimulado` (distância da face, 30 mm mais perto que o centro, e 8190 mm sem alvo além de 1200 mm) e a passa pelo `CombinadorTof`. `forcarAmostra(lado, mm)` troca a próxima amostra de um lado (por exemplo, sensor encostado) e `simularFalha(lado, true)` faz um sensor parar de responder. O `test_integracao_tof` explora os labirintos 4×4, 8×4 e 12×4 por essa cadeia, mais um 12×4 com corredores longos que só se fecha com o "sem alvo" tratado como livre, e confere que o mapa final é igual ao labirinto.

### Bancada dos VL53L0X

Para conferir o driver no hardware montado, junto com a eletrônica, use o menu de bancada (precisa de `pip install pyserial`):

```bash
./scripts/bancada_tof.py                   # escolhe a porta; ou --porta /dev/ttyACM0 (COM5 no Windows)
```

1. **Opção 1:** grava o firmware de bancada (`pio run -e bancada_tof -t upload`). Ele mantém os motores freados o tempo todo.
2. **Opção 3:** roteiro completo, com APROVADO, REPROVADO ou OBSERVADO em cada verificação, e um relatório em Markdown no fim, para registrar em `docs/07-testes/software.md`.
3. Ao terminar, grave de volta o firmware do robô: `pio run -e esp32c3 -t upload`.

O CI compila o `bancada_tof` junto com o firmware do robô e roda os testes do menu (`scripts/testes/test_bancada_tof.py`), que conferem cada etapa com respostas falsas da placa, sem hardware nem `pyserial`. Para rodar localmente: `python3 -m unittest discover -s scripts/testes -v`.

| Etapa | O que verifica |
|---|---|
| 1. Fiação | Cada XSHUT liga só o seu sensor, que aparece sozinho no 0x29 |
| 2. Inicialização | Os 3 sensores iniciam em 0x30, 0x31 e 0x32, e o 0x29 fica livre |
| 3. Taxa e tempo | 20 a 40 Hz por sensor, leitura em até 5 ms e ≥ 95% de leituras válidas |
| 4. Régua | Média, erro e desvio contra distâncias medidas (tolerância de ±10 mm ou ±5%) |
| 5. Fora da faixa | Sensor sem alvo e sensor encostado: leitura válida, limitada a 1200 mm ou 30 mm, com o motivo na amostra |
| 6. Sensor ausente | Um sensor desconectado (com a placa desligada) não impede os outros |

Os critérios ficam no topo do script e são provisórios. O firmware de bancada também funciona direto no monitor serial: `pio device monitor -e bancada_tof` e o comando `AJUDA` listam `SCAN`, `ISOLAR`, `INIT`, `LER`, `FLUXO` e `ESTAT`; cada resposta é uma linha JSON.

## Boot e autoteste

Na Rev 2.0, o robô só libera os motores depois de encontrar os 4 dispositivos do I²C ([#50](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/50)). A sequência do `setup()`:

1. Motores freados e **travados** (`MotoresComTrava`): travado, todo comando de potência vira freio.
2. `Wire.begin` e `tof.iniciar()`: os 3 VL53L0X saem do reset um por vez e recebem 0x30, 0x31 e 0x32 (#344).
3. `AutotesteBoot::verificar`: varre de 0x08 a 0x77 e confere os ToF esquerdo (0x30), frontal (0x31) e direito (0x32) e o MPU6050 (0x68, com o `WHO_AM_I` = 0x68).
4. Aprovado, `motoresComTrava.liberar()`; com qualquer falha, os motores ficam travados **até reiniciar** e o log diz o que falhou.
5. A linha `{"tipo":"boot",...}` sai no log, e as tarefas sobem mesmo com falha.

```cpp
MotoresComTrava motoresComTrava(motores);            // quem aciona as rodas usa este, não o driver
ResultadoBoot r = autoteste.verificar(barramento, {tofEsqIniciado, tofFrenteIniciado, tofDirIniciado});
if (r.aprovado()) { motoresComTrava.liberar(); }
```

No `main.cpp`, cada camada dos motores tem o próprio nome (`motores` é o driver, `motoresComTrava` a trava) e a navegação aciona as rodas só por `motoresTracao`, uma referência à camada de fora. Ordem combinada quando as outras camadas entrarem: `MotoresComTrava` (#50) → `MotoresComProtecao` (#341) → `MotoresComEstimativa` (#380) → `MotoresDrv8833`. O `motores.iniciar()` é a primeira linha do `setup()`, para frear antes de tudo.

| Situação | Estado no relatório | Libera os motores? |
|---|---|---|
| Dispositivo responde no endereço esperado | `ok` | Sim, se os 4 estiverem `ok` |
| Ninguém no endereço | `ausente` | Não |
| ToF responde, mas o driver não conseguiu configurá-lo | `nao_iniciou` | Não |
| Algo no 0x68 com outro `WHO_AM_I` (ou sem leitura) | `nao_identificado` | Não |
| Algo no 0x29: um ToF ficou no endereço de fábrica | `tof_0x29: true` | Não |
| Outro endereço qualquer | `inesperados` (só aviso) | Sim |
| Boot acima de 2 s (RNF-09) | `no_prazo: false` (só aviso) | Sim |

Exemplo do relatório:

```json
{"tipo":"boot","ok":false,"motores_liberados":false,"falhas":"tof_frontal","boot_ms":412,"prazo_ms":2000,"no_prazo":true,"dispositivos":{"tof_esquerdo":"ok","tof_frontal":"ausente","tof_direito":"ok","mpu6050":"ok"},"tof_0x29":false,"mpu6050_who_am_i":104,"inesperados":[],"inesperados_total":0}
```

**Na USB nativa, a linha do boot se perde se o monitor abrir depois do reset.** Por isso a telemetria repete `boot_ok`, `boot_falhas` e `boot_ms` em todo pacote, e o comando `B` no monitor serial reimprime o relatório.

O `boot_ms` é o `millis()` no fim do `setup()`, contado desde o início do programa: a ROM e o bootloader do ESP32-C3 rodam antes e ficam de fora. Para o tempo desde o ligar, meça na bancada.

Os parâmetros ficam em `lib/inicializacao/src/ConfiguracaoBoot.h`. O `WHO_AM_I` esperado é **provisório**: módulos com clone do MPU6050 respondem outro valor (0x70, 0x72, 0x98); se a bancada mostrar um desses em `mpu6050_who_am_i`, confirme com a eletrônica e troque lá. Repetir as tentativas durante a corrida é da [#241](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/241), que pode usar a mesma trava (`motoresComTrava.travar()`).

## Instalação

Instale o PlatformIO por uma das opções:

- Extensão **PlatformIO IDE** no VS Code (abra a pasta `src/firmware`).
- Linha de comando: `pip install platformio`.

## Compilar, gravar e testar

Execute os comandos dentro de `src/firmware`:

| Objetivo | Comando |
|---|---|
| Compilar para o ESP32-C3 | `pio run -e esp32c3` |
| Gravar na placa | `pio run -e esp32c3 -t upload` |
| Gravar o firmware de bancada dos VL53L0X | `pio run -e bancada_tof -t upload` (ou pelo `scripts/bancada_tof.py`) |
| Gravar o firmware de bancada do deslocamento | `pio run -e bancada_deslocamento -t upload` (ou pelo `scripts/bancada_deslocamento.py`) |
| Abrir o monitor serial | `pio device monitor` (`B` repete o relatório do boot; `Z` zera os encoders) |
| Rodar os testes no computador | `pio test -e native` |
| Rodar um teste específico | `pio test -e native -f test_nucleo` |

## Pinagem e log serial (Rev 2.0)

A placa é a **ESP32-C3 Super Mini** (esquemático Rev 2.0 de 21/10/2026). Todos os 15 GPIOs do ESP32-C3 estão em uso:

| Função | GPIO |
|---|---|
| I²C: SDA / SCL (3 VL53L0X e MPU6050) | 9 / 8 |
| XSHUT dos VL53L0X: esquerdo / frontal / direito | 0 / 1 / 2 |
| Motor esquerdo no DRV8833: IN1 / IN2 | 10 / 3 |
| Motor direito no DRV8833: IN3 / IN4 | 20 / 21 |
| Encoder esquerdo: A / B (reservado, ainda não instalado) | 4 / 5 |
| Encoder direito: A / B (reservado, ainda não instalado) | 6 / 7 |
| USB nativa: D− / D+ (log e gravação) | 18 / 19 |

O **log serial sai pela USB nativa** (`ARDUINO_USB_MODE=1` e `ARDUINO_USB_CDC_ON_BOOT=1` no `platformio.ini`), porque GPIO20/21, a UART0, são as entradas do driver dos motores. O `main.cpp` não compila sem essas opções. Ao ligar, a ROM do ESP32-C3 ainda escreve no GPIO21 durante o reset: a montagem precisa manter o driver desabilitado no boot (validação da eletrônica na bancada). A porta serial aparece no computador como um dispositivo USB (`pio device monitor` a encontra sozinho); sem um computador conectado, o log é descartado.

## IMU (MPU6050) e barramento I²C

O giroscópio do MPU6050 dá o **ângulo de giro** (yaw) do robô, a principal fonte de orientação enquanto não há encoders. A lógica (`lib/imu`) é testada no computador com o `ImuSimulado`; o driver (`ImuMpu6050`, em `drivers_esp32`) só lê os registradores.

| Peça | Onde | O que faz |
|---|---|---|
| `IImu` | `lib/hal` | `lerTaxaGiroZ(taxa)`: taxa de giro em décimos de °/s, positiva para a esquerda |
| `ImuMpu6050` | `lib/drivers_esp32` | Reinicia o sensor, confere o `WHO_AM_I`, liga o filtro de 42 Hz, configura a faixa e lê os dois bytes do eixo Z (registradores 0x47 e 0x48) |
| `BarramentoI2c` | `lib/drivers_esp32` | O `Wire` com um mutex do FreeRTOS: toda transação I²C passa por `BarramentoI2c::Uso` |
| `IAcelerometro` | `lib/hal` | `lerAceleracaoPlano(ax, ay)`: aceleração no plano do chão, em mg (detecção de colisão, #192). O `ImuMpu6050` também a implementa, lendo os registradores 0x3B a 0x3E |
| `AcompanhadorGiro` | `lib/imu` | Um ciclo da tarefa da IMU: calibra o viés na largada (recomeça sozinha se o robô se mexer), integra o ângulo e invalida o ângulo depois de 5 falhas seguidas |
| `tarefaImu` | `src/main.cpp` | Lê a IMU a cada 10 ms com `vTaskDelayUntil` (intervalo fixo), com prioridade 3, e publica a `LeituraGiro`, que as outras tarefas leem por `lerGiro()`, e a aceleração no plano, que a navegação lê por `lerAceleracao()` |

**Barramento compartilhado.** Os 3 VL53L0X (lidos pela navegação) e o MPU6050 (lido pela tarefa da IMU) usam o mesmo SDA/SCL. Uma transação no meio de outra mistura os bytes dos dois sensores, então **todo acesso ao `Wire` precisa estar dentro de um `BarramentoI2c::Uso`**, inclusive o do driver dos ToF. A proteção é um mutex, e não uma seção crítica (`portENTER_CRITICAL`): a seção crítica desliga as interrupções, de que o driver I²C depende. Se o barramento não vier em `ESPERA_MAXIMA_I2C_MS` (8 ms), a leitura falha e é contada, em vez de atrasar a tarefa.

```cpp
micromouse::BarramentoI2c::Uso uso(barramentoI2c);
if (uso.conseguiu()) {
  valida = tof.lerDistanciasLaterais(distancias);   // o barramento fica livre ao sair do bloco
}
```

**Na largada.** Nos primeiros 2 s (200 leituras), o robô precisa ficar parado: a média vira o viés. A mensagem `{"status":"giroscopio_calibrado"}` sai no log quando a calibração termina. Ao iniciar, o log mostra `imu_iniciada` com o `who_am_i` (0x68 no MPU6050) ou `falha_imu` se o sensor não respondeu.

**Valores provisórios**, para conferir na bancada:

| Parâmetro | Arquivo | Valor |
|---|---|---|
| Faixa do giroscópio | `ConfiguracaoImu.h` | ±500 °/s |
| Faixa do acelerômetro | `ConfiguracaoImu.h` | ±4 g |
| Período de leitura | `ConfiguracaoImu.h` | 10 ms (100 leituras por segundo, igual à taxa configurada no sensor) |
| Amostras da calibração, variação máxima e zona morta | `ConfiguracaoImu.h` | 200, 5 °/s e 0,3 °/s |
| Sensor de cabeça para baixo | `ConfiguracaoImu.h` (`INVERTER_GIRO_Z`) | `false`: girando o robô para a esquerda com a mão, a taxa tem que ser positiva |
| Falhas seguidas que invalidam o ângulo | `ConfiguracaoImu.h` | 5 (50 ms) |
| Frequência do I²C e espera pelo barramento | `Configuracao.h` | 100 kHz e 8 ms |

## Padrão de testes

| Nível | Onde roda | Ferramenta | Quando usar |
|---|---|---|---|
| Unitário | Computador (`native`) | Unity | Lógica pura: tipos, classificação, mapa, serialização |
| Integração | Computador (`native`) | Unity, com falsos de `simulacao` no lugar do hardware | Percepção → mapa em labirinto simulado; ToF → mapa pelas amostras brutas (`test_integracao_tof`) |
| Bancada | Placa com o hardware real | Monitor serial, checklist do teste e os scripts de `scripts/` (testados em `scripts/testes/`, sem a placa) | Sensores VL53L0X (`bancada_tof.py`), deslocamento sem encoder (`bancada_deslocamento.py`), encoders, motores |

Para criar um teste:

1. Crie a pasta `test/test_<modulo>/` (o prefixo `test_` é obrigatório).
2. Adicione um arquivo `.cpp` com `setUp`, `tearDown`, as funções `test_*` e um `main` com `RUN_TEST`. Use `test/test_nucleo/test_direcao.cpp` como modelo.
3. Rode `pio test -e native -f test_<modulo>`.

Nomeie as funções de teste pelo comportamento esperado (`test_classifica_como_parede_quando_distancia_abaixo_do_limiar`) e mantenha cada teste verificando uma única regra.

## Convenções

**Idioma do código:** nomes de pastas, arquivos, tipos, funções, variáveis, constantes e testes em português, sem acentos (identificadores só com ASCII), e comentários em português. Mantêm-se em inglês apenas as APIs e os termos consagrados: `setup`/`loop`, `Serial`, `xTaskCreate`, `LiDAR`, `UART`, `PWM`, `buffer`, `ISensorDistancia` (prefixo `I` de interface).

| Elemento | Estilo | Exemplo |
|---|---|---|
| Tipos e enums | `PascalCase` | `Direcao`, `EstadoParede` |
| Valores de enum | `PascalCase` | `Direcao::Norte` |
| Funções e variáveis | `camelCase` | `girarDireita`, `frenteMm` |
| Constantes | `MAIÚSCULAS_COM_SUBLINHADO`, sem acento | `MAX_LINHAS_LABIRINTO` |
| Unidade no nome | sufixo | `distanciaMm`, `periodoMs` |
| Testes | `test_<comportamento>` | `test_oposta_inverte_a_direcao` |

**Estrutura:**

- Código dentro do namespace `micromouse`.
- Um arquivo `.h` por tipo ou conceito, com `#pragma once`.
- Cada tarefa do épico adiciona seus arquivos na pasta da própria camada e seus testes em `test/test_<modulo>/`.
- A pinagem está em `lib/nucleo/src/Pinos.h` e segue o esquemático Rev 2.0 (ver [hardware](../../docs/04-projeto-conceitual/hardware.md)). Ela é o único lugar com número de GPIO; o teste `test_pinos` reprova repetição e GPIO inexistente.

## CI

Todo PR para a `main` (e todo push nela) roda o workflow **CI Firmware** (`.github/workflows/ci-firmware.yml`). A `main` é protegida: o PR só pode ser integrado com os dois checks verdes.

| Check | O que faz | Reprova quando |
|---|---|---|
| **Testes nativos** | Roda `pio test -e native_cobertura` (os testes do `native` com medição de cobertura) e gera o relatório com o `gcovr` | Algum teste falha ou a cobertura das linhas de `lib/` fica abaixo de 80% |
| **Build ESP32-C3** | Roda `pio run -e esp32c3` | O firmware não compila para a placa |

A cobertura mede só o código compilado nos testes nativos (`lib/`, sem `drivers_esp32`); `src/main.cpp` fica de fora. Código novo em `lib/` sem teste derruba a cobertura e reprova o PR.

### Como ler o resultado

1. No PR, abra a aba **Checks** e escolha **CI Firmware**.
2. Clique no job com ✗ e expanda o passo que falhou:
   - **Rodar os testes com cobertura:** procure a linha `FAIL`. Ela mostra o arquivo, a linha e a mensagem do Unity, por exemplo `Expected 2 Was 1`.
   - **Exigir cobertura mínima de 80%:** a cobertura ficou abaixo da meta. Veja na tabela do resumo quais arquivos têm linhas sem teste.
   - **`pio run -e esp32c3`:** erro de compilação. A mensagem do compilador indica o arquivo e a linha.
3. A tabela de cobertura aparece no **Summary** do workflow. O relatório HTML linha a linha fica no artefato **cobertura-firmware** (no fim da página do Summary, disponível por 14 dias): baixe, descompacte e abra o `index.html`.

### Rodar localmente antes de abrir o PR

Dentro de `src/firmware`:

| Objetivo | Comando |
|---|---|
| Mesmos testes do CI, com cobertura | `pio test -e native_cobertura` |
| Ver a cobertura (precisa de `pip install gcovr`) | `gcovr --root . --filter 'lib/' --object-directory .pio/build/native_cobertura --txt` |
| Conferir a meta de 80% | `gcovr --root . --filter 'lib/' --object-directory .pio/build/native_cobertura --fail-under-line 80` |
| Mesmo build do CI | `pio run -e esp32c3` |

O resultado esperado hoje é **10 testes aprovados** e cobertura de 100%. A pasta `cobertura/` é ignorada pelo Git.
