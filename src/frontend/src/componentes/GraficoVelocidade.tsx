import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts';
import type { Telemetria } from '../tipos';

interface GraficoVelocidadeProps {
  leituras: Telemetria[];
}

/**
 * Gráfico de exemplo com Recharts: velocidade instantânea ao longo da tentativa.
 * Cresce a cada leitura recebida; antes da largada mostra só um aviso.
 */
export function GraficoVelocidade({ leituras }: GraficoVelocidadeProps) {
  if (leituras.length === 0) {
    return <p className="grafico-vazio">O gráfico aparece quando a corrida começar.</p>;
  }

  const dados = leituras.map((l) => ({ tempo: l.tempo_decorrido, velocidade: l.velocidade }));

  return (
    <ResponsiveContainer width="100%" height={180}>
      <LineChart data={dados} margin={{ top: 8, right: 16, bottom: 0, left: -8 }}>
        <CartesianGrid stroke="#E2E8F0" strokeDasharray="3 3" />
        <XAxis
          dataKey="tempo"
          type="number"
          domain={[0, 'dataMax']}
          tickFormatter={(segundos: number) => `${Math.round(segundos)} s`}
          tick={{ fontSize: 11, fill: '#64748B' }}
          stroke="#CBD5E1"
        />
        <YAxis
          domain={[0, 0.5]}
          tickFormatter={(valor: number) => valor.toFixed(1)}
          tick={{ fontSize: 11, fill: '#64748B' }}
          stroke="#CBD5E1"
          width={40}
        />
        <Tooltip
          formatter={(valor: number | string | Array<number | string>) => [
            `${Number(valor).toFixed(2)} m/s`,
            'Velocidade',
          ]}
          labelFormatter={(segundos: number) => `${segundos.toFixed(1)} s`}
        />
        <Line
          type="monotone"
          dataKey="velocidade"
          stroke="#4C1DF5"
          strokeWidth={2}
          dot={false}
          isAnimationActive={false}
        />
      </LineChart>
    </ResponsiveContainer>
  );
}
