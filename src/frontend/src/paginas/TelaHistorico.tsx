import { useState } from 'react';
import { FiltroLabirinto, type Filtro } from '../componentes/FiltroLabirinto';
import { TabelaHistorico } from '../componentes/TabelaHistorico';
import type { Corrida, Labirinto } from '../tipos';
import './TelaHistorico.css';

interface TelaHistoricoProps {
  corridas: Corrida[];
  labirintos: Labirinto[];
}

/**
 * Tela "Histórico de Consultas" (Frame 04 e filtros M1–M3; HUs 10 e 15): todas as
 * corridas, das mais recentes para as mais antigas, com filtro por labirinto.
 */
export function TelaHistorico({ corridas, labirintos }: TelaHistoricoProps) {
  const [filtro, setFiltro] = useState<Filtro>('todos');

  const idsFiltrados =
    filtro === 'todos' ? null : new Set(labirintos.filter((l) => l.tipo === filtro).map((l) => l.id));
  const visiveis = corridas
    .filter((c) => idsFiltrados === null || idsFiltrados.has(c.labirinto_id))
    .sort((a, b) => Date.parse(b.data_hora_inicio) - Date.parse(a.data_hora_inicio));

  return (
    <section className="historico cartao" aria-label="Histórico de corridas">
      <div className="historico__topo">
        <FiltroLabirinto valor={filtro} aoMudar={setFiltro} />
        <span className="historico__contagem">
          {visiveis.length} {visiveis.length === 1 ? 'corrida' : 'corridas'}
        </span>
      </div>
      <TabelaHistorico corridas={visiveis} labirintos={labirintos} />
    </section>
  );
}
