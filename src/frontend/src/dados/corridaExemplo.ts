import { TENSAO_CHEIA_V, percentualBateria } from '../lib/formatacao';
import { explorarAteObjetivo, lerLabirintoAscii, orientacaoDoPasso } from '../lib/labirinto';
import type { Corrida, Labirinto, MapaCelula, Orientacao, Posicao, Telemetria } from '../tipos';
import { LABIRINTOS_ASCII } from './labirintosExemplo';

/** Lado da célula, em metros (`TAMANHO_CELULA_MM` = 180 no firmware). */
export const TAMANHO_CELULA_M = 0.18;

/** Corrida de exemplo completa: mapa verdadeiro, telemetria e registro consolidado. */
export interface CorridaExemplo {
  labirinto: Labirinto;
  /** Todas as paredes do labirinto (o que o robô terá descoberto ao final). */
  mapa: MapaCelula[];
  /** Uma leitura por célula percorrida, da largada até o objetivo. */
  telemetria: Telemetria[];
  /** Linha da tabela `corridas` como ficaria ao final (HU-09). */
  corrida: Corrida;
}

const INICIO_EXEMPLO = Date.parse('2026-10-16T14:32:00-03:00');

/** Ordem de tentativa das direções na exploração: escolhida para o trajeto passar por mais células. */
const ORDEM_EXPLORACAO: Orientacao[] = ['O', 'L', 'N', 'S'];

/** Quantos quartos de volta separam duas orientações (0, 1 ou 2). */
function quartosDeVolta(de: Orientacao, para: Orientacao): number {
  const ordem: Orientacao[] = ['N', 'L', 'S', 'O'];
  const diferenca = Math.abs(ordem.indexOf(de) - ordem.indexOf(para)) % 4;
  return diferenca === 3 ? 1 : diferenca;
}

const arredondar = (valor: number, casas: number) => Number(valor.toFixed(casas));

/**
 * Gera uma corrida de exemplo no labirinto dado, no formato das tabelas do Supabase.
 * Os valores são inventados mas coerentes entre si: o trajeto respeita as paredes, o tempo
 * cresce mais nas curvas, a velocidade cai ao virar e a tensão da bateria cai aos poucos.
 */
export function gerarCorridaExemplo(labirinto: Labirinto, numeroTentativa = 1): CorridaExemplo {
  // As dimensões vêm do próprio mapa, para o desenho nunca divergir das paredes.
  const { linhas, colunas, celulas } = lerLabirintoAscii(LABIRINTOS_ASCII[labirinto.tipo]);
  const percurso: Posicao[] = explorarAteObjetivo(celulas, linhas, colunas, ORDEM_EXPLORACAO);
  const corridaId = labirinto.id * 100 + numeroTentativa;

  const telemetria: Telemetria[] = [];
  let tempo = 0;
  let orientacao: Orientacao = 'N';

  percurso.forEach((posicao, i) => {
    let velocidade = 0;
    let corrente = 0.12; // parado na largada: só ESP32 e LiDAR
    if (i > 0) {
      const nova = orientacaoDoPasso(percurso[i - 1], posicao);
      const quartos = quartosDeVolta(orientacao, nova);
      orientacao = nova;
      tempo += 1.2 + 0.5 * quartos + 0.05 * Math.sin(i * 2.3); // anda, para e lê o LiDAR
      velocidade = quartos === 0 ? 0.32 + 0.04 * Math.sin(i) : 0.16 + 0.03 * Math.sin(i);
      corrente = quartos === 0 ? 0.45 + 0.08 * Math.sin(i * 1.3) : 0.6;
    }
    const ultima = i === percurso.length - 1;
    telemetria.push({
      id: i + 1,
      corrida_id: corridaId,
      numero_sequencia: i + 1,
      timestamp: new Date(INICIO_EXEMPLO + tempo * 1000).toISOString(),
      posicao_linha: posicao.linha,
      posicao_coluna: posicao.coluna,
      orientacao,
      velocidade: arredondar(ultima ? 0 : velocidade, 3),
      tensao_bateria: arredondar(TENSAO_CHEIA_V - 0.002 * i - 0.05 * corrente, 3),
      corrente_bateria: arredondar(ultima ? 0.12 : corrente, 3),
      tempo_decorrido: arredondar(tempo, 3),
      estado_corrida: ultima ? 'parado' : 'explorando',
      evento: ultima ? 'objetivo_alcancado' : null,
    });
  });

  const final = telemetria[telemetria.length - 1];
  const distancia = (percurso.length - 1) * TAMANHO_CELULA_M;
  const corrida: Corrida = {
    id: corridaId,
    labirinto_id: labirinto.id,
    numero_tentativa: numeroTentativa,
    data_hora_inicio: new Date(INICIO_EXEMPLO).toISOString(),
    data_hora_fim: final.timestamp,
    tempo_final: final.tempo_decorrido,
    velocidade_media: arredondar(distancia / final.tempo_decorrido, 3),
    velocidade_pico: Math.max(...telemetria.map((t) => t.velocidade)),
    consumo_bateria: 100 - percentualBateria(final.tensao_bateria),
    status: 'concluida',
  };

  return { labirinto: { ...labirinto, linhas, colunas }, mapa: celulas, telemetria, corrida };
}
