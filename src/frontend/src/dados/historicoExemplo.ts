import type { Corrida } from '../tipos';

/**
 * Corridas de exemplo para a tela de histórico (Frame 04), no formato da tabela `corridas`.
 * `labirinto_id` segue o *seed* de `labirintos`: 1 = 4x4, 2 = 8x4, 3 = 12x4.
 * `consumo_bateria` em percentual consumido (veja `Corrida`).
 * Os dados reais entram com a HU-10 (#13), lendo `corridas` pelo supabase-js.
 */
export const CORRIDAS_EXEMPLO: Corrida[] = [
  {
    id: 1,
    labirinto_id: 1,
    numero_tentativa: 1,
    data_hora_inicio: '2026-10-16T14:02:00-03:00',
    data_hora_fim: '2026-10-16T14:02:15.300-03:00',
    tempo_final: 15.3,
    velocidade_media: 0.12,
    velocidade_pico: 0.3,
    consumo_bateria: 1,
    status: 'nao_concluida',
  },
  {
    id: 2,
    labirinto_id: 1,
    numero_tentativa: 2,
    data_hora_inicio: '2026-10-16T14:10:00-03:00',
    data_hora_fim: '2026-10-16T14:10:42.180-03:00',
    tempo_final: 42.18,
    velocidade_media: 0.15,
    velocidade_pico: 0.34,
    consumo_bateria: 3,
    status: 'concluida',
  },
  {
    id: 3,
    labirinto_id: 2,
    numero_tentativa: 1,
    data_hora_inicio: '2026-10-16T14:32:00-03:00',
    data_hora_fim: '2026-10-16T14:33:08.450-03:00',
    tempo_final: 68.45,
    velocidade_media: 0.16,
    velocidade_pico: 0.36,
    consumo_bateria: 4,
    status: 'concluida',
  },
  {
    id: 4,
    labirinto_id: 3,
    numero_tentativa: 1,
    data_hora_inicio: '2026-10-16T15:05:00-03:00',
    data_hora_fim: '2026-10-16T15:15:00-03:00',
    tempo_final: 600,
    velocidade_media: 0.11,
    velocidade_pico: 0.33,
    consumo_bateria: 22,
    status: 'nao_concluida',
  },
  {
    id: 5,
    labirinto_id: 3,
    numero_tentativa: 2,
    data_hora_inicio: '2026-10-16T15:20:00-03:00',
    data_hora_fim: '2026-10-16T15:22:11.900-03:00',
    tempo_final: 131.9,
    velocidade_media: 0.17,
    velocidade_pico: 0.37,
    consumo_bateria: 8,
    status: 'concluida',
  },
];
