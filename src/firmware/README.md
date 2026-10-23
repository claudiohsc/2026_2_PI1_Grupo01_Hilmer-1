# Firmware

Firmware do micromouse para o ESP32-C3 (Arduino Core, C++17), organizado nas camadas *sense-think-act* definidas na [arquitetura de software](../../docs/04-projeto-conceitual/software.md). O projeto usa [PlatformIO](https://platformio.org/).

A documentação detalhada do código (camadas, tipos, pinagem, interface do LiDAR, tarefas e testes) está em [Firmware: implementação](../../docs/04-projeto-conceitual/firmware.md). Os arquivos `.h` têm comentários de documentação no formato Doxygen (`///`).

## Estrutura

```
src/firmware/
├── platformio.ini     # ambientes esp32c3 (placa), native (testes no computador), native_cobertura (testes com cobertura, usado no CI) e bancada_tof (firmware de bancada dos VL53L0X)
├── src/main.cpp       # ponto de entrada: cria as tarefas FreeRTOS (navegação e telemetria)
├── src/bancada/       # programas de bancada, fora do firmware do robô
├── lib/               # uma biblioteca por camada
├── scripts/           # ferramentas de bancada no computador (Python)
└── test/              # testes unitários e de integração (Unity)
```

| Pasta em `lib/` | Camada | O que entra |
|---|---|---|
| `nucleo` | Base | Tipos compartilhados (`Direcao`, `Lado`, `EstadoParede`, `PosicaoCelula`, `Pose`, `DistanciasLaterais`), constantes e pinagem |
| `hal` | Interfaces de hardware | Interfaces abstratas (`ILidar`, `IEncoder` e, nos próximos épicos, motores) |
| `percepcao` | Percepção | Classificação `parede`/`livre` a partir das distâncias do LiDAR |
| `tof` | Sensoriamento | Regra de validade das leituras dos 3 VL53L0X (`CombinadorTof`) e seus parâmetros provisórios (`ConfiguracaoTof.h`), sem Arduino |
| `deslocamento` | Sensoriamento | Deslocamento sem encoder: estimativa pela potência dos motores (`EncoderEstimado`, `MotoresComEstimativa`) e correção pelos ToF; todos os ajustes em `ConfiguracaoDeslocamento.h` |
| `encoders` | Sensoriamento | Decodificação em quadratura (x4) dos canais A e B, conversão de pulsos em milímetros (calibrável) e distância percorrida por cada roda |
| `lidar` | Drivers / Hardware | Driver de leitura do sensor LiDAR 360° via UART (decodificador LD06) e agregação de leituras por setores |
| `mapeamento` | Mapeamento | Mapa do labirinto (4 paredes por célula), atualização a partir das leituras classificadas e armazenamento em memória |
| `navegacao` | Navegação | Localização, decisão de movimento e detecção do objetivo |
| `atuacao` | Atuação | Controle dos motores e correção de trajetória |
| `comunicacao` | Comunicação | Pacotes de telemetria, serialização JSON e *buffer* de reenvio |
| `simulacao` | Falsos para teste | `LidarSimulado`, um `ILidar` que devolve as leituras configuradas pelo teste, `LabirintoSimulado`, o labirinto de referência que alimenta o `LidarSimulado`, e `EncoderSimulado`, um `IEncoder` com a contagem definida pelo teste, sem hardware |
| `drivers_esp32` | Implementações do hardware | Drivers que dependem do Arduino (3 VL53L0X pelo I²C, encoders, DRV8833, WiFi) |

## Regras de dependência

- As camadas de lógica (`nucleo`, `percepcao`, `mapeamento`, `navegacao`, `comunicacao`, `simulacao`) **não incluem `Arduino.h`**. Assim elas compilam e são testadas no computador, sem a placa.
- O acesso ao hardware passa pelas interfaces de `hal`. Os drivers reais ficam em `drivers_esp32`, e `src/main.cpp` liga cada driver à sua interface.
- Para testar a lógica sem sensor, use o `LidarSimulado` (`lib/simulacao`): `definirLeituraFixa` repete uma leitura, `enfileirarLeitura` entrega leituras em sequência, `simularFalha(true)` faz o sensor parar de responder e, com um `LabirintoSimulado`, `posicionar` faz a leitura vir da posição do robô (veja [Labirinto simulado](#labirinto-simulado)).
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

O armazenamento não pode ser copiado, e o acesso `const` impede que a telemetria altere o mapa; as duas regras são conferidas em tempo de compilação.

#### Mapa final da corrida

Ao alcançar a célula objetivo, a navegação chama `congelarMapa()`. O armazenamento copia o mapa da corrida para um segundo `Mapa`, o **mapa final**, que a partir daí só é lido: a navegação pode continuar registrando paredes no mapa da corrida (por exemplo, no caminho de volta) sem mudar o resultado ([#180](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/180), HU-04).

```cpp
if (chegouAoObjetivo) {
  armazenamentoMapa.congelarMapa();  // true na primeira vez; false se já estava congelado
}

static char json[TAMANHO_MAX_JSON_MAPA + 1];
size_t tamanho = MapaJson::serializarFinal(armazenamentoMapa, json, sizeof(json));  // 0 se ainda não congelou
```

| Regra | Comportamento |
|---|---|
| Congelar antes de `iniciarCorrida()` | Recusado: `congelarMapa()` devolve `false` |
| Congelar de novo | Recusado: devolve `false`, e o mapa final continua o da primeira vez |
| `mapaFinal()` | `nullptr` enquanto não congelou; depois, o mapa final, só para leitura |
| Escritas depois de congelar | Mudam só o mapa da corrida (`mapa()`); o mapa final e o JSON dele não mudam |
| Nova corrida | `iniciarCorrida()` destrava o congelamento, e `mapaFinal()` volta a ser `nullptr` |
| `serializarFinal()` | O mesmo JSON de `serializar()` (formato da [#178](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/178)), mas do mapa final; devolve `0` e deixa a string vazia se o mapa não foi congelado ou se o buffer não comporta o JSON |

**Memória.** Com o mapa final, o objeto passou de 196 para **392 bytes** (`sizeof` no computador): dois mapas de 195 bytes e os dois indicadores, de corrida iniciada e de mapa congelado. O limite do teste `test_armazenamento_ocupa_pouca_memoria` subiu de 256 para 512 bytes. Antes do mapa final, a medida no ESP32-C3 era de 196 bytes de RAM estática (`.bss`), igual ao `sizeof`.

## Atualização do mapa

O `AtualizadorMapa` (`lib/mapeamento`) integra a percepção ao mapa: a partir da `Pose` do robô e da classificação das leituras (`ClassificadorParede`), converte as direções relativas (`Frente`, `Esquerda`, `Direita`) para as direções absolutas do mapa (`Norte`, `Leste`, `Sul`, `Oeste`) e atualiza as paredes no `Mapa`.

```cpp
Mapa mapa;
Pose pose = {{0, 0}, Direcao::Norte};
ClassificadorParede classificador;

// Sobrecarga de conveniência: recebe amostras do LiDAR e classifica antes de atualizar
DistanciasLaterais amostras[3] = { {80, 400, 420}, {85, 390, 410}, {90, 410, 400} };
ResultadoAtualizacao res = AtualizadorMapa::atualizar(mapa, pose, classificador, amostras, 3);
```

| Regra | Comportamento |
|---|---|
| Conversão de eixos | Converte `Frente`, `Esquerda` e `Direita` da `Pose` para as direções cardeais absolutas |
| `Desconhecido` | Leituras desconhecidas não alteram o mapa e retornam `Invalida` |
| Conflitos | Se uma leitura contradiz o mapa, `temConflito` é marcado como `true`, retorna `Conflito` e o valor antigo do mapa é preservado; a política de navegação fica com quem chama (#188) |
| Pose inválida | Coordenadas fora dos limites do mapa retornam `Invalida` para todas as direções |

## Odometria

A `Odometria` (`lib/navegacao`) estima a pose do robô (célula e direção) pela odometria diferencial dos encoders: a diferença entre as rodas gira a direção, e a média avança o robô na direção atual. A arquitetura não tem IMU. Não depende do Arduino e só usa inteiros.

```cpp
Odometria odometria;                              // parte em (0, 0), virada para o Norte
odometria.atualizar(encoderEsq, encoderDir);      // a cada ciclo, com os dois IEncoder
Pose pose = odometria.pose();                     // célula e direção
char letra = letraDaDirecao(pose.direcao);        // 'N', 'L', 'S' ou 'O', como no banco
```

| Regra | Comportamento |
|---|---|
| Giro | `contagem esquerda − contagem direita` dividida por `pulsosDiferenciaisPor90Graus`, arredondada para o múltiplo de 90° mais próximo; só muda a direção depois de 45° |
| Avanço | Média das duas rodas, na direção atual; a célula muda ao cruzar a parede, a meia célula do centro |
| Giro no lugar | Não muda a célula |
| Fora da malha | A célula fica limitada a 0 e 255, e `posicaoValida()` devolve `false` |
| Reinício | `reiniciar(pose, contagemEsquerda, contagemDireita)` define a pose e a referência das contagens |
| Parâmetros | `ConfiguracaoOdometria.h`: 251 µm por pulso (roda de 32 mm, 400 pulsos por volta) e **625 pulsos por 90° (provisório)**, a calibrar na bancada |

No `main.cpp`, a tarefa de navegação atualiza a odometria a cada 10 ms, e a telemetria acrescenta `linha`, `coluna`, `orientacao` e `pose_valida` à linha de JSON dos encoders.

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
| Telemetria | Campos `deslocamento` (`"estimado"` ou `"encoder"`) e `sem_correcao_mm`. O comando `Z` reposiciona o robô no centro da célula atual |
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
| `TABELA_VELOCIDADE` | 150 → 45, 400 → 120, 700 → 210, 1000 → 300 (potência → mm/s) | Etapa 2 da bancada abaixo: 2 s por ponto, medidos na régua; o menu imprime a tabela pronta para colar |
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

## Serialização do mapa em JSON

O `MapaJson` (`lib/comunicacao`) converte o `Mapa` em JSON (ArduinoJson) para o envio na telemetria. Não depende do Arduino, então é testado no ambiente `native`.

```cpp
char buffer[TAMANHO_MAX_JSON_MAPA + 1];
size_t tamanho = MapaJson::serializar(mapa, buffer, sizeof(buffer));
// {"dimensao":"4x4","linhas":4,"colunas":4,"celulas":[{"linha":0,"coluna":0,
//  "parede_norte":null,"parede_sul":true,"parede_leste":null,"parede_oeste":true}, ...]}
```

| Campo | Valor |
|---|---|
| `dimensao` | `"4x4"`, `"8x4"` ou `"12x4"` (colunas × linhas, como `labirintos.tipo`) |
| `linhas`, `colunas` | Tamanho do labirinto |
| `linha`, `coluna` | Posição da célula; a partida é (0, 0) |
| `parede_norte`, `parede_sul`, `parede_leste`, `parede_oeste` | `true` se há parede, `false` se está livre e `null` se ainda é desconhecida |

Os nomes dos campos das células são os das colunas da tabela `mapa_celulas` do banco, então a lista pode ser inserida nela, acrescentando só o `corrida_id`.

| Regra | Comportamento |
|---|---|
| Saída | Compacta, sem espaços nem quebras de linha, terminada em `\0` |
| Buffer | Do chamador, de tamanho fixo; a ArduinoJson só usa memória dinâmica durante a conversão, liberada ao final |
| `TAMANHO_MAX_JSON_MAPA` (5.100 bytes) | Pior caso: um 12×4 com as 48 células conhecidas e todas as paredes internas `Livre` (`false` ocupa mais caracteres que `true` ou `null`; o perímetro continua `true`). Maior que a pilha das tarefas (4.096 bytes), por isso precisa ser `static` ou global |
| Capacidade insuficiente, ou `saida` nulo/capacidade zero | Devolve `0` e deixa `saida` com string vazia, sem escrever além da capacidade informada |

## Labirinto simulado

O `LabirintoSimulado` (`lib/simulacao`) é um labirinto com todas as paredes conhecidas. Com ele, o `LidarSimulado` devolve o que o sensor real veria na posição do robô, sem digitar leituras à mão. Serve para testar a classificação de paredes e a atualização do mapa num labirinto inteiro.

**Formato do texto.** O labirinto é escrito em ASCII: cada célula ocupa 4 colunas e 2 linhas de texto, `+` marca os cantos, `---` uma parede horizontal, `|` uma parede vertical e espaço é passagem livre. A primeira linha é a borda Norte e a partida (0, 0) é a célula do canto inferior esquerdo, como no `Mapa`. Cada parede aparece uma única vez, então as duas células vizinhas sempre concordam. Há um exemplo de cada tamanho (4×4, 8×4 e 12×4) em `test/test_labirinto_simulado/LabirintosExemplo.h`.

```cpp
LabirintoSimulado labirinto;
labirinto.carregar(exemplos::LABIRINTO_4X4);   // ResultadoCarga::Carregado
labirinto.temParede({0, 0}, Direcao::Leste);   // true

LidarSimulado lidar;
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
| `LidarSimulado::usarLabirinto` e `posicionar` | Liga o modo labirinto. A falha e a fila continuam com prioridade; sem `posicionar`, vale a leitura fixa; com a pose fora do labirinto, a leitura falha |

## Sensores de distância (VL53L0X)

Na Rev 2.0 do hardware, o LiDAR deu lugar a 3 VL53L0X no I²C (esquerdo, frontal e direito). O `TofVl53l0x` (`lib/drivers_esp32`) implementa o `ILidar`, então mapa, percepção e navegação não mudaram ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344)). A regra de validade fica no `CombinadorTof` (`lib/tof`), testado no `native`; a biblioteca `pololu/VL53L0X` só entra no `esp32c3`.

```cpp
Wire.begin(pinos::I2C_SDA, pinos::I2C_SCL);   // barramento compartilhado com o MPU6050
TofVl53l0x tof(Wire, {pinos::XSHUT_FRONTAL, pinos::ENDERECO_TOF_FRONTAL},
               {pinos::XSHUT_ESQ, pinos::ENDERECO_TOF_ESQ},
               {pinos::XSHUT_DIR, pinos::ENDERECO_TOF_DIR});
tof.iniciar();                                // XSHUT um por vez e endereços 0x31, 0x30, 0x32

DistanciasLaterais d;
if (tof.lerDistanciasLaterais(d)) { /* os 3 sensores com medida válida e recente */ }
tof.amostra(Lado::Frente);                    // medida bruta, estado e instante, mesmo com a leitura recusada
```

| Regra | Comportamento |
|---|---|
| Inicialização | Os 3 XSHUT em nível baixo; cada sensor é solto, inicializado e recebe o endereço antes do próximo. Sensor que não responde fica em reset (`sensorIniciado(lado)` diz quais iniciaram) |
| Leitura | Nunca espera medida: só lê os sensores que já têm medida pronta. A leitura vale com os 3 entre 30 e 1200 mm e a amostra mais antiga com até 100 ms |
| Recusa | `false` com algum sensor em `Falha`, `AbaixoDoMinimo`, `ForaDeAlcance` (inclui os 8190 mm "sem alvo") ou parado há mais de 100 ms |
| Idade | `idadeUltimaLeituraMs()`: idade do sensor mais atrasado, para a navegação não repetir dado antigo |
| Referência | Distância a partir da face de cada sensor; a geometria do robô e os limiares ficam na percepção ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)) |

Os parâmetros estão em `lib/tof/src/ConfiguracaoTof.h` e são **provisórios** até a bancada ([#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175)). A telemetria serial do `main.cpp` traz os campos `tof_frente_mm`, `tof_esq_mm`, `tof_dir_mm`, `tof_valida` e `tof_idade_ms`.

### Bancada dos VL53L0X

Para conferir o driver no hardware montado, junto com a eletrônica, use o menu de bancada (precisa de `pip install pyserial`):

```bash
./scripts/bancada_tof.py                   # escolhe a porta; ou --porta /dev/ttyACM0 (COM5 no Windows)
```

1. **Opção 1:** grava o firmware de bancada (`pio run -e bancada_tof -t upload`). Ele mantém os motores freados o tempo todo.
2. **Opção 3:** roteiro completo, com APROVADO, REPROVADO ou OBSERVADO em cada verificação, e um relatório em Markdown no fim, para registrar em `docs/07-testes/software.md`.
3. Ao terminar, grave de volta o firmware do robô: `pio run -e esp32c3 -t upload`.

| Etapa | O que verifica |
|---|---|
| 1. Fiação | Cada XSHUT liga só o seu sensor, que aparece sozinho no 0x29 |
| 2. Inicialização | Os 3 sensores iniciam em 0x30, 0x31 e 0x32, e o 0x29 fica livre |
| 3. Taxa e tempo | 20 a 40 Hz por sensor, leitura em até 5 ms e ≥ 95% de leituras válidas |
| 4. Régua | Média, erro e desvio contra distâncias medidas (tolerância de ±10 mm ou ±5%) |
| 5. Fora da faixa | Sensor sem alvo e sensor encostado: leitura recusada, com o motivo na amostra |
| 6. Sensor ausente | Um sensor desconectado (com a placa desligada) não impede os outros |

Os critérios ficam no topo do script e são provisórios. O firmware de bancada também funciona direto no monitor serial: `pio device monitor -e bancada_tof` e o comando `AJUDA` listam `SCAN`, `ISOLAR`, `INIT`, `LER`, `FLUXO` e `ESTAT`; cada resposta é uma linha JSON.

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
| Abrir o monitor serial | `pio device monitor` |
| Rodar os testes no computador | `pio test -e native` |
| Rodar um teste específico | `pio test -e native -f test_nucleo` |

## Padrão de testes

| Nível | Onde roda | Ferramenta | Quando usar |
|---|---|---|---|
| Unitário | Computador (`native`) | Unity | Lógica pura: tipos, classificação, mapa, serialização |
| Integração | Computador (`native`) | Unity, com falsos de `simulacao` no lugar do hardware | Percepção → mapa em labirinto simulado |
| Bancada | Placa com o hardware real | Monitor serial, checklist do teste e os scripts de `scripts/` | Sensores VL53L0X (`bancada_tof.py`), encoders, motores |

Para criar um teste:

1. Crie a pasta `test/test_<modulo>/` (o prefixo `test_` é obrigatório).
2. Adicione um arquivo `.cpp` com `setUp`, `tearDown`, as funções `test_*` e um `main` com `RUN_TEST`. Use `test/test_nucleo/test_direcao.cpp` como modelo.
3. Rode `pio test -e native -f test_<modulo>`.

Nomeie as funções de teste pelo comportamento esperado (`test_classifica_como_parede_quando_distancia_abaixo_do_limiar`) e mantenha cada teste verificando uma única regra.

## Convenções

**Idioma do código:** nomes de pastas, arquivos, tipos, funções, variáveis, constantes e testes em português, sem acentos (identificadores só com ASCII), e comentários em português. Mantêm-se em inglês apenas as APIs e os termos consagrados: `setup`/`loop`, `Serial`, `xTaskCreate`, `LiDAR`, `UART`, `PWM`, `buffer`, `ILidar` (prefixo `I` de interface).

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
- A pinagem está em `lib/nucleo/src/Pinos.h` e segue os [diagramas de hardware](../../docs/04-projeto-conceitual/hardware-diagramas.md).

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
