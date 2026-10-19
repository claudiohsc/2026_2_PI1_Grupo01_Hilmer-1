import type { EstadoPainel, StatusCorrida } from '../tipos';
import './BadgeDesafio.css';

type Tom = 'neutro' | 'erro' | 'sucesso';

const POR_ESTADO: Record<EstadoPainel, { texto: string; tom: Tom }> = {
  aguardando: { texto: 'DESAFIO CUMPRIDO: NÃO', tom: 'neutro' },
  em_execucao: { texto: 'DESAFIO CUMPRIDO: NÃO (EM ANDAMENTO)', tom: 'erro' },
  concluido: { texto: 'DESAFIO CUMPRIDO: SIM', tom: 'sucesso' },
  nao_concluido: { texto: 'DESAFIO CUMPRIDO: NÃO', tom: 'erro' },
};

const POR_STATUS: Record<StatusCorrida, { texto: string; tom: Tom }> = {
  concluida: { texto: 'DESAFIO CUMPRIDO: SIM', tom: 'sucesso' },
  nao_concluida: { texto: 'DESAFIO CUMPRIDO: NÃO', tom: 'erro' },
  em_andamento: { texto: 'EM ANDAMENTO', tom: 'neutro' },
};

interface BadgeDesafioProps {
  /** Estado da tela ao vivo (cabeçalho). */
  estado?: EstadoPainel;
  /** Status de uma corrida salva (tabela do histórico). Usado quando `estado` não é passado. */
  status?: StatusCorrida;
  className?: string;
}

/** Badge "Desafio cumprido (S/N)". */
export function BadgeDesafio({ estado, status, className }: BadgeDesafioProps) {
  const { texto, tom } = estado ? POR_ESTADO[estado] : POR_STATUS[status ?? 'em_andamento'];
  return (
    <span
      className={`badge badge--${tom}${className ? ` ${className}` : ''}`}
      role={estado ? 'status' : undefined}
    >
      {texto}
    </span>
  );
}
