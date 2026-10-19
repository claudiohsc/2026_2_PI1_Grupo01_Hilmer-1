import { CartaoMetrica } from '../componentes/CartaoMetrica';
import { GradeLabirinto } from '../componentes/GradeLabirinto';
import { GraficoVelocidade } from '../componentes/GraficoVelocidade';
import { TAMANHO_CELULA_M, type CorridaExemplo } from '../dados/corridaExemplo';
import {
  LIMITE_TEMPO_S,
  MAXIMO_TENTATIVAS,
  TENSAO_CHEIA_V,
  formatarMinutos,
  formatarTempo,
  formatarVelocidade,
  notaDaTentativa,
  percentualBateria,
} from '../lib/formatacao';
import { mapaConhecido } from '../lib/labirinto';
import type { ReproducaoCorrida } from '../hooks/useReproducaoCorrida';
import type { Orientacao, Posicao } from '../tipos';
import './TelaAoVivo.css';

interface TelaAoVivoProps {
  corridaExemplo: CorridaExemplo;
  reproducao: ReproducaoCorrida;
  tentativa: number;
  aoIniciar: () => void;
  aoResetar: () => void;
}

/**
 * Tela "Telemetria ao Vivo" (Frames 01, 02 e 03; HU-14): mapa com o trajeto à esquerda e,
 * à direita, os cards de bateria, velocidade e tempo e o botão de controle da corrida.
 */
export function TelaAoVivo({ corridaExemplo, reproducao, tentativa, aoIniciar, aoResetar }: TelaAoVivoProps) {
  const { labirinto, mapa } = corridaExemplo;
  const { estado, tempo, leituras } = reproducao;
  const ultima = leituras.at(-1);

  // Mapa e trajeto
  const percurso: Posicao[] = leituras.map((l) => ({ linha: l.posicao_linha, coluna: l.posicao_coluna }));
  const robo: Posicao & { orientacao: Orientacao } = ultima
    ? { linha: ultima.posicao_linha, coluna: ultima.posicao_coluna, orientacao: ultima.orientacao }
    : { linha: 0, coluna: 0, orientacao: 'N' };

  // Cards (sem leituras, valores nominais do Frame 01)
  const tensao = ultima?.tensao_bateria ?? TENSAO_CHEIA_V;
  const correnteMa = Math.round((ultima?.corrente_bateria ?? 0) * 1000);
  const distancia = Math.max(0, leituras.length - 1) * TAMANHO_CELULA_M;
  const velocidadeMedia = ultima && ultima.tempo_decorrido > 0 ? distancia / ultima.tempo_decorrido : 0;
  const velocidadePico = leituras.reduce((maximo, l) => Math.max(maximo, l.velocidade), 0);

  const detalheTempo =
    estado === 'concluido'
      ? `Concluído na Tentativa ${tentativa} · Nota ${notaDaTentativa(tentativa)}`
      : estado === 'nao_concluido'
        ? `Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Tempo esgotado`
        : `Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Limite: ${formatarMinutos(LIMITE_TEMPO_S)}`;

  return (
    <div className="ao-vivo">
      <section className="ao-vivo__mapa cartao" aria-label="Mapa do labirinto">
        <GradeLabirinto
          linhas={labirinto.linhas}
          colunas={labirinto.colunas}
          mapa={mapaConhecido(mapa, percurso)}
          percurso={percurso}
          robo={robo}
        />

        <div className="ao-vivo__grafico">
          <h2 className="ao-vivo__titulo">Velocidade × tempo</h2>
          <GraficoVelocidade leituras={leituras} />
        </div>
      </section>

      <aside className="ao-vivo__lateral" aria-label="Telemetria">
        <CartaoMetrica
          rotulo="Consumo de bateria"
          valor={`${percentualBateria(tensao)}%`}
          detalhe={`${tensao.toFixed(1)}V · ${correnteMa} mA`}
        />
        <CartaoMetrica
          rotulo="Velocidade média"
          valor={formatarVelocidade(velocidadeMedia)}
          detalhe={`Pico máx: ${formatarVelocidade(velocidadePico)}`}
        />
        <CartaoMetrica rotulo="Tempo de conclusão" valor={formatarTempo(tempo)} detalhe={detalheTempo} />

        {estado === 'aguardando' && (
          <button type="button" className="botao botao--primario" onClick={aoIniciar}>
            INICIAR CORRIDA
          </button>
        )}
        {estado === 'em_execucao' && (
          <button type="button" className="botao botao--contorno" disabled>
            CORRIDA EM ANDAMENTO
          </button>
        )}
        {(estado === 'concluido' || estado === 'nao_concluido') && (
          <button type="button" className="botao botao--primario" onClick={aoResetar}>
            RESETAR / NOVA TENTATIVA
          </button>
        )}
      </aside>
    </div>
  );
}
