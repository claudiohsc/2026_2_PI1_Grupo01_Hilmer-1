# Testes de Estrutura

> Apresentar com detalhes os testes feitos para conferir se os objetivos da estrutura foram atendidos, incluindo:
>
> - Imagem com todas as peças separadas da estrutura (carcaça) do veículo;
> - Imagem que apresente a estrutura já montada (sem necessidade de partes internas, como eletrônica ou motores);
> - Ilustração contemplando o carro montado virtualmente em alguma ferramenta de CAD, no intuito de verificar se as peças foram construídas e montadas em conformidade com o projeto.

Os itens **1.1–1.6** referem-se à validação em CAD do **labirinto de testes 4×4** (base 720 × 720 mm). Detalhes, desenhos e arquivos em [Labirinto de testes](../04-projeto-conceitual/labirinto-testes.md). As linhas em branco ficam para chassi, tração e montagem física.

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 1.1 | Conferência das dimensões da base no CAD (720 × 720 mm, grade 4 × 4) — [#270](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/270) | 14/10/2026 | 19/10/2026 | Ana Carolina Nunes e Ágata Dias Vieira| Contorno externo 720 × 720 mm e 16 células no desenho técnico | Contorno 720 × 720 mm confirmado nas folhas do Modelo 1 e do Modelo 2; grade 4 × 4 | S | — | Prints nas Figuras 2 e 6 de [Labirinto de testes](../04-projeto-conceitual/labirinto-testes.md) |
| 1.2 | Conferência do passo nominal da célula (180 mm) no CAD | 14/10/2026 | 19/10/2026 | Ana Carolina Nunes e Ágata Dias Vieira| Passo entre eixos de parede = 180 mm | Passo de 180 mm conferido na grade do desenho (paredes internas 168 mm + espessura 12 mm) | S | — | Ver seção 2 e desenhos em [Labirinto de testes](../04-projeto-conceitual/labirinto-testes.md) |
| 1.3 | Conferência do comprimento e do encaixe das paredes internas (168 × 12 × 50 mm) | 14/10/2026 | 19/10/2026 | Ana Carolina Nunes e Ágata Dias Vieira | Parede interna 168 mm; montagem sem sobrepor a célula de 180 mm | Peça 168 × 12 × 50 mm (Modelo 2) / 168 × 12 × 56 mm com inserção (Modelo 1) dimensionada no CAD e nos PDFs | S | — | Figuras 1 e 5 (montagem virtual) e folhas de parede nos PDFs |
| 1.4 | Conferência dos rasgos do Modelo 1 (largura ~12,4 mm, profundidade 6 mm) | 14/10/2026 | 19/10/2026 | Ana Carolina Nunes e Ágata Dias Vieira | Rasgos compatíveis com parede de 12 mm e inserção de 6 mm | Desenho técnico do Modelo 1 com rasgos dimensionados; vista CAD da base com grade de rasgos | S | — | Figuras 1–4; arquivo `mec/micromouse_modelo1_rasgos.step` |
| 1.5 | Conferência dos furos e cavilhas do Modelo 2 (Ø4,2 mm, cavilha Ø4 × 14 mm) | 14/10/2026 | 19/10/2026 | Ana Carolina Nunes e Ágata Dias Vieira | Furos e cavilhas alinhados à parede; base 720 × 720 × 15 mm | Desenho e CAD do Modelo 2 com furos Ø4,2 mm e base 15 mm; montagem virtual com paredes | S | — | Figuras 5–9; arquivo `mec/micromouse_modelo2_cavilhas.step` |
| 1.6 | Montagem virtual do labirinto no CAD (dois modelos) — [#272](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/272) | 14/10/2026 | 19/10/2026 | Ana Carolina Nunes e Ágata Dias Vieira | Conjunto montado no CAD sem interferência grosseira; paredes e base coerentes | Vistas isométricas dos dois modelos geradas; PDFs de fabricação e STEPs publicados | S | — | Figuras 1 e 5; PDFs em `mec/` |
| 1.7 | | | | | | | | | |
| 1.8 | | | | | | | | | |
| 1.9 | | | | | | | | | |
| 1.10 | | | | | | | | | |
| 1.11 | | | | | | | | | |
| 1.12 | | | | | | | | | |
| 1.13 | | | | | | | | | |
| 1.14 | | | | | | | | | |
| 1.15 | | | | | | | | | |
| 1.16 | | | | | | | | | |

## Evidências (labirinto — CAD)

| Teste | Evidência |
|-------|-----------|
| 1.1–1.6 | [Labirinto de testes](../04-projeto-conceitual/labirinto-testes.md) (Figuras 1–9) |
| Modelo 1 | `mec/micromouse_modelo1_rasgos.step`, `mec/micromouse_modelo1_rasgos-desenho-v1.pdf` |
| Modelo 2 | `mec/micromouse_modelo2_cavilhas.step`, `mec/micromouse_modelo2_cavilhas-desenho-v1.pdf` |

> **Pendente para a fabricação física:** fotos das peças separadas e do labirinto montado (itens sugeridos no cabeçalho desta página). Incluir nas linhas 1.7+ quando houver protótipo.
