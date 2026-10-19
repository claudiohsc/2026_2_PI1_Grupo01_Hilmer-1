import './CartaoMetrica.css';

interface CartaoMetricaProps {
  /** Título em caixa alta (ex.: "Consumo de bateria"). */
  rotulo: string;
  /** Valor em destaque (ex.: "100%"). */
  valor: string;
  /** Linha de detalhe abaixo do valor (ex.: "8.4V · 0 mA"). */
  detalhe: string;
}

/** Card de telemetria do painel lateral (HU-08, critério 3). */
export function CartaoMetrica({ rotulo, valor, detalhe }: CartaoMetricaProps) {
  return (
    <section className="metrica cartao" aria-label={rotulo}>
      <h2 className="metrica__rotulo">{rotulo}</h2>
      <p className="metrica__valor">{valor}</p>
      <p className="metrica__detalhe">{detalhe}</p>
    </section>
  );
}
