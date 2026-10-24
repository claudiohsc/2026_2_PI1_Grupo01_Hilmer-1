import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it, vi } from 'vitest';
import { FiltroLabirinto } from './FiltroLabirinto';

describe('FiltroLabirinto (controle segmentado do histórico)', () => {
  it('mostra as 4 opções na ordem dos Frames M1 a M3: Todos, 4×4, 8×4 e 12×4', () => {
    render(<FiltroLabirinto valor="todos" aoMudar={() => {}} />);

    const grupo = screen.getByRole('group', { name: 'Filtrar por labirinto' });
    const rotulos = Array.from(grupo.querySelectorAll('button')).map((b) => b.textContent);
    expect(rotulos).toEqual([
      'Todos os Labirintos',
      'Labirinto 1 (4×4)',
      'Labirinto 2 (8×4)',
      'Labirinto 3 (12×4)',
    ]);
  });

  it.each([
    ['todos', 'Todos os Labirintos'],
    ['4x4', 'Labirinto 1 (4×4)'],
    ['8x4', 'Labirinto 2 (8×4)'],
    ['12x4', 'Labirinto 3 (12×4)'],
  ] as const)('com o valor %s, só a opção "%s" fica marcada como ativa', (valor, rotulo) => {
    render(<FiltroLabirinto valor={valor} aoMudar={() => {}} />);

    for (const botao of screen.getAllByRole('button')) {
      const ativa = botao.textContent === rotulo;
      expect(botao).toHaveAttribute('aria-pressed', String(ativa));
      expect(botao.classList.contains('filtro__opcao--ativa')).toBe(ativa);
    }
  });

  it('avisa a opção clicada', async () => {
    const user = userEvent.setup();
    const aoMudar = vi.fn();
    render(<FiltroLabirinto valor="todos" aoMudar={aoMudar} />);

    await user.click(screen.getByRole('button', { name: 'Labirinto 3 (12×4)' }));
    await user.click(screen.getByRole('button', { name: 'Labirinto 1 (4×4)' }));
    await user.click(screen.getByRole('button', { name: 'Todos os Labirintos' }));

    expect(aoMudar.mock.calls).toEqual([['12x4'], ['4x4'], ['todos']]);
  });
});
