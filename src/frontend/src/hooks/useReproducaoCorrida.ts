import { useCallback, useEffect, useMemo, useState } from 'react';
import { LIMITE_TEMPO_S } from '../lib/formatacao';
import type { EstadoPainel, Telemetria } from '../tipos';

/** Quantas vezes mais rápido que o tempo real a corrida de exemplo é reproduzida. */
export const VELOCIDADE_REPRODUCAO = 4;

/** Intervalo de atualização do cronômetro na tela, em ms. */
const PASSO_TELA_MS = 50;

export interface ReproducaoCorrida {
  estado: EstadoPainel;
  /** Tempo da tentativa atual, em segundos (congela ao concluir). */
  tempo: number;
  /** Leituras já "recebidas" até `tempo`, em ordem de `numero_sequencia`. */
  leituras: Telemetria[];
  iniciar: () => void;
  reiniciar: () => void;
}

/**
 * Reproduz a telemetria de exemplo como se chegasse do robô, para a tela ao vivo passar
 * pelos três estados do protótipo: aguardando (Frame 01) → em execução (Frame 02) →
 * concluído (Frame 03). A corrida termina quando chega a leitura com o evento
 * `objetivo_alcancado` (HU-07) ou quando o tempo passa de 10:00 (RNF-03).
 *
 * Na integração com o robô (HU-08), as leituras passam a vir do Supabase Realtime e este
 * hook é substituído; os componentes da tela continuam recebendo `estado`, `tempo` e `leituras`.
 */
export function useReproducaoCorrida(telemetria: Telemetria[]): ReproducaoCorrida {
  const [estado, setEstado] = useState<EstadoPainel>('aguardando');
  const [tempo, setTempo] = useState(0);

  useEffect(() => {
    if (estado !== 'em_execucao') return;

    const chegada = telemetria.find((l) => l.evento === 'objetivo_alcancado')?.tempo_decorrido ?? Infinity;
    const inicio = performance.now();

    const id = window.setInterval(() => {
      const decorrido = ((performance.now() - inicio) / 1000) * VELOCIDADE_REPRODUCAO;
      if (chegada <= LIMITE_TEMPO_S && decorrido >= chegada) {
        setTempo(chegada);
        setEstado('concluido');
      } else if (decorrido >= LIMITE_TEMPO_S) {
        setTempo(LIMITE_TEMPO_S);
        setEstado('nao_concluido');
      } else {
        setTempo(decorrido);
      }
    }, PASSO_TELA_MS);

    return () => window.clearInterval(id);
  }, [estado, telemetria]);

  const leituras = useMemo(
    () => (estado === 'aguardando' ? [] : telemetria.filter((l) => l.tempo_decorrido <= tempo)),
    [estado, telemetria, tempo],
  );

  const iniciar = useCallback(() => {
    setTempo(0);
    setEstado('em_execucao');
  }, []);

  const reiniciar = useCallback(() => {
    setTempo(0);
    setEstado('aguardando');
  }, []);

  return { estado, tempo, leituras, iniciar, reiniciar };
}
