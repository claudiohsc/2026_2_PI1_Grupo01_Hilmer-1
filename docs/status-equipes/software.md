# Status de Software

**Gerente:** Cláudio Henrique. **Integrantes:** Ana Carolina Fialho (também gerente geral do projeto), Eduardo Viana, Marllon Fausto e Rafael Lima.

**Última atualização:** 19/10/2026 (Sprint 4). **Atualizado por:** Cláudio Henrique (gerente de software).

**O que a equipe entrega:** o firmware do ESP32-C3 (percepção, mapa, navegação, controle dos motores), o backend no Supabase (banco e Edge Function) e o dashboard web. Detalhes em [Software](../04-projeto-conceitual/software.md) e [Firmware](../04-projeto-conceitual/firmware.md).

## Onde está

| **Épico** | **Marco** | **Situação** |
|-----------|:---------:|--------------|
| 01 - Percepção e Mapeamento | ED1 | Todas as tarefas fechadas, exceto o [teste de bancada com o LiDAR real (#175)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175) |
| 02 - Navegação e Controle Autônomo | ED1 | Em andamento: resolução do labirinto, encoders, odometria e centralização prontos; faltam execução dos movimentos, colisão e corrida completa |
| 03 - Telemetria e Dashboard | ED2 | Base pronta (banco, registro de corridas, estrutura do frontend); telemetria do robô e dashboard ao vivo ainda não iniciados |
| 04 - Inicialização | ED2 | Não iniciado (Sprint 8) |

!!! note "Sobre o cronograma"
    A tabela de software em [Cronograma](../05-cronograma.md) ainda mostra 0% e "A fazer" em linhas já entregues. Para o progresso real, vale o estado das *issues* e dos PRs abaixo.

## Entregue (mesclado em `main`)

- **Base:** projeto base do firmware (#172), CI do firmware com testes nativos e build ESP32-C3 (#223) e simulador de labirinto (#224).
- **Percepção:** driver de leitura do LiDAR (#173) e classificação parede/livre (#174).
- **Mapa:** estrutura de dados (#176), atualização a cada célula (#177), armazenamento em memória (#179), mapa final serializado (#180) e formato JSON (#178).
- **Navegação:** algoritmo de resolução (#183), leitura dos encoders (#186), odometria (#187), centralização no corredor e parada frontal (#191), esta já com o driver PWM do DRV8833 (`MotoresDrv8833`).
- **Backend:** Supabase com esquema do banco (#228) e registro e encerramento de corridas por Edge Function (#233).
- **Frontend:** estrutura React + Vite e layout do dashboard (#230).

## Em revisão

| **PR** | **Assunto** | **Tipo** |
|:------:|-------------|:--------:|
| [#351](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/351) | Detecção da célula objetivo e telemetria (#189) | Código |
| [#367](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/367) | Tela de histórico com dados reais do Supabase (#235) | Código |
| [#350](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/350) | Documentação da Edge Function de corridas (#233) | Documentação |
| [#362](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/362) | Documentação do frontend do dashboard (#230) | Documentação |
| [#368](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/368) | Documentação da consulta ao histórico (#235) | Documentação |

## Em aberto, por prioridade para a ED1 (26/10)

- **Motores e LiDAR:** [driver do DRV8833 e detecção de motor travado (#341)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341) e [interface do LiDAR: comando de partida, UART1 e log pela USB nativa (#344)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344). O PWM básico do DRV8833 já existe; a #341 ajusta os parâmetros informados pela eletrônica e protege o driver contra motor travado. Sem a #344, o driver do LiDAR não recebe dados de um sensor que só transmite após um comando de partida.
- **Navegação:** execução dos movimentos entre células (#184), robustez da pose (#357), correção do erro acumulado pelas paredes (#188), compensação de irregularidades (#190, #181), colisão e recuperação (#192, #182) e teste de corrida completa (#185).
- **Backend:** CI do backend (#345).

**Épico 03 e 04 (depois da ED1):** envio de telemetria (#236), buffer e reenvio (#238), latência (#237), persistência (#229), dashboard ao vivo (#231), mapa e percurso (#239), filtro do histórico (#234), teste ponta a ponta (#232), boot (#50), calibração (#240), falha de sensor (#241) e estado "Aguardando Largada" (#242).

## Precisa ou aguarda de outras equipes

| **De** | **O quê** | **Dependência** |
|--------|-----------|:---------------:|
| Eletrônica | Protocolo do LiDAR (modelo, bytes de partida, formato do pacote) | D1 |
| Eletrônica | Pinagem final, sem conflito entre encoders e driver | D2 |
| Estrutura | Diâmetro da roda e bitola definitivos. A odometria usa valores provisórios (por exemplo, pulsos por giro de 90 graus provisório em `ConfiguracaoOdometria.h`) | D3 |
| Estrutura | Labirinto de testes físico | D4 |
| Energia | Tabela de estado de carga e leitura do ADC para telemetria de bateria | D7 |

Veja a tabela completa em [Status das equipes](README.md#dependencias-entre-equipes).

## Riscos conhecidos

- Faltam sete dias para a ED1 e a interface do LiDAR (#344) e a proteção de motor travado (#341) seguem abertas; os testes de software com hardware dependem delas.
- Não consta nos documentos nenhum teste do firmware com o hardware real concluído; os testes registrados rodam no computador.
- Não consta nos documentos qual pino de ADC o firmware usará para medir a bateria, nem tarefa de software para essa leitura.
- A CI cobre o firmware; o backend ([#345](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/345)) e o frontend ainda não têm verificação automática em PR.

## Fontes

[Épicos e tarefas de software](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues?q=is%3Aissue+label%3Asoftware) · [Cronograma de software](../05-cronograma.md#software-epicos-sprints-e-historias-de-usuario) · [Testes de software](../07-testes/software.md)
