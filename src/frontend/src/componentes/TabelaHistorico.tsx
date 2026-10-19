import {
  descreverTentativa,
  formatarDataHora,
  formatarTempo,
  formatarVelocidade,
  nomeLabirintoHistorico,
} from '../lib/formatacao';
import type { Corrida, Labirinto } from '../tipos';
import { BadgeDesafio } from './BadgeDesafio';
import './TabelaHistorico.css';

interface TabelaHistoricoProps {
  corridas: Corrida[];
  labirintos: Labirinto[];
}

const TRACO = '—';

/** Tabela do histórico de corridas (Frame 04; HU-10, critério 2). */
export function TabelaHistorico({ corridas, labirintos }: TabelaHistoricoProps) {
  const tipoDoLabirinto = (id: number) => labirintos.find((l) => l.id === id)?.tipo;

  return (
    <div className="tabela-historico">
      <table>
        <caption className="visualmente-oculto">Histórico de corridas</caption>
        <thead>
          <tr>
            <th scope="col">Data / Hora</th>
            <th scope="col">Labirinto</th>
            <th scope="col">Tentativa</th>
            <th scope="col">Tempo final</th>
            <th scope="col">Vel. média</th>
            <th scope="col" title="Carga restante ao final da corrida">Bateria</th>
            <th scope="col">Status</th>
          </tr>
        </thead>
        <tbody>
          {corridas.map((corrida) => {
            const tipo = tipoDoLabirinto(corrida.labirinto_id);
            return (
              <tr key={corrida.id}>
                <td>{formatarDataHora(corrida.data_hora_inicio)}</td>
                <td>{tipo ? nomeLabirintoHistorico(tipo) : TRACO}</td>
                <td>{descreverTentativa(corrida.numero_tentativa, corrida.status)}</td>
                <td className="numero">{corrida.tempo_final != null ? formatarTempo(corrida.tempo_final) : TRACO}</td>
                <td className="numero">
                  {corrida.velocidade_media != null ? formatarVelocidade(corrida.velocidade_media) : TRACO}
                </td>
                <td className="numero">
                  {corrida.consumo_bateria != null ? `${Math.round(100 - corrida.consumo_bateria)}%` : TRACO}
                </td>
                <td>
                  <BadgeDesafio status={corrida.status} />
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>

      {corridas.length === 0 && <p className="tabela-historico__vazio">Nenhuma corrida registrada para este filtro.</p>}
    </div>
  );
}
