import { TIPOS_LABIRINTO, nomeLabirintoHistorico } from '../lib/formatacao';
import type { TipoLabirinto } from '../tipos';
import './FiltroLabirinto.css';

export type Filtro = 'todos' | TipoLabirinto;

interface FiltroLabirintoProps {
  valor: Filtro;
  aoMudar: (filtro: Filtro) => void;
}

const OPCOES: { valor: Filtro; rotulo: string }[] = [
  { valor: 'todos', rotulo: 'Todos os Labirintos' },
  ...TIPOS_LABIRINTO.map((tipo) => ({ valor: tipo, rotulo: nomeLabirintoHistorico(tipo) })),
];

/** Controle segmentado do histórico (Frames 04, M1, M2 e M3; HU-15, critério 4). */
export function FiltroLabirinto({ valor, aoMudar }: FiltroLabirintoProps) {
  return (
    <div className="filtro" role="group" aria-label="Filtrar por labirinto">
      {OPCOES.map((opcao) => (
        <button
          key={opcao.valor}
          type="button"
          aria-pressed={valor === opcao.valor}
          className={`filtro__opcao${valor === opcao.valor ? ' filtro__opcao--ativa' : ''}`}
          onClick={() => aoMudar(opcao.valor)}
        >
          {opcao.rotulo}
        </button>
      ))}
    </div>
  );
}
