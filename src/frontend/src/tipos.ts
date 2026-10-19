/**
 * Tipos das tabelas do Supabase, com os mesmos nomes de colunas da migração
 * `src/backend/supabase/migrations/20261014180000_esquema_inicial.sql` (#228).
 * Assim, quando os dados de exemplo forem trocados pelos dados reais (HUs 08, 09, 10 e 15),
 * os componentes não precisam mudar.
 */

/** Formatos de labirinto previstos no projeto (coluna `labirintos.tipo`). */
export type TipoLabirinto = '4x4' | '8x4' | '12x4';

/** Tabela `labirintos`. */
export interface Labirinto {
  id: number;
  tipo: TipoLabirinto;
  /** Eixo Norte-Sul. */
  linhas: number;
  /** Eixo Leste-Oeste. */
  colunas: number;
}

/** Coluna `corridas.status`. */
export type StatusCorrida = 'em_andamento' | 'concluida' | 'nao_concluida';

/** Tabela `corridas`. Datas em ISO 8601, como o supabase-js devolve `timestamptz`. */
export interface Corrida {
  id: number;
  labirinto_id: number;
  numero_tentativa: number;
  data_hora_inicio: string;
  data_hora_fim: string | null;
  /** Segundos. */
  tempo_final: number | null;
  /** m/s. */
  velocidade_media: number | null;
  /** m/s. */
  velocidade_pico: number | null;
  /**
   * Consumo de bateria. A migração ainda admite "mAh ou % consumida"; esta tela assume
   * **percentual consumido** (0 a 100) até a HU-09 fechar a unidade.
   */
  consumo_bateria: number | null;
  status: StatusCorrida;
}

/** Coluna `telemetria.orientacao` (L = Leste, O = Oeste). */
export type Orientacao = 'N' | 'S' | 'L' | 'O';

/** Coluna `telemetria.evento`. */
export type EventoTelemetria = 'colisao' | 'objetivo_alcancado' | null;

/** Tabela `telemetria`: uma leitura enviada pelo robô durante a corrida. */
export interface Telemetria {
  id: number;
  corrida_id: number;
  numero_sequencia: number;
  timestamp: string;
  posicao_linha: number;
  posicao_coluna: number;
  orientacao: Orientacao;
  /** m/s. */
  velocidade: number;
  /** V. */
  tensao_bateria: number;
  /** A. */
  corrente_bateria: number;
  /** Segundos desde o início da corrida. */
  tempo_decorrido: number;
  estado_corrida: string | null;
  evento: EventoTelemetria;
}

/**
 * Tabela `mapa_celulas` (sem `id`/`corrida_id`), no mesmo formato do JSON do mapa
 * enviado pelo firmware (#178): `true` = parede, `false` = livre, `null` = não explorada.
 */
export interface MapaCelula {
  linha: number;
  coluna: number;
  parede_norte: boolean | null;
  parede_sul: boolean | null;
  parede_leste: boolean | null;
  parede_oeste: boolean | null;
}

/** Célula do labirinto. (0, 0) é a largada, no canto inferior esquerdo. */
export interface Posicao {
  linha: number;
  coluna: number;
}

/** Estados da tela ao vivo (HU-14, critério 3), um por frame do protótipo. */
export type EstadoPainel =
  | 'aguardando' // Frame 01_Idle
  | 'em_execucao' // Frame 02_Running
  | 'concluido' // Frame 03_Completed
  | 'nao_concluido'; // tempo esgotado (10:00) sem chegar ao objetivo

/** Abas do cabeçalho. */
export type Aba = 'ao_vivo' | 'historico';
