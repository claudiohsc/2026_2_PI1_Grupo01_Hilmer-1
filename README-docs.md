# Documentação (MkDocs + Material)

Este projeto usa [MkDocs](https://www.mkdocs.org) para transformar os
arquivos Markdown da pasta `docs/` em um site navegável, publicado no GitHub Pages.

## Pré-requisitos

Instale o MkDocs com o tema Material (de preferência em um ambiente virtual):

```bash
python3 -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install mkdocs-material
```

## Comandos

- `mkdocs serve` — sobe o servidor local com recarga automática em <http://127.0.0.1:8000>.
- `mkdocs build` — gera o site estático na pasta `site/`.
- `mkdocs build --strict` — gera o site tratando avisos (links quebrados etc.) como erro.
- `mkdocs gh-deploy` — gera e publica na branch `gh-pages` (feito automaticamente pelo CI).
- `mkdocs -h` — mostra a ajuda.

## Estrutura do projeto

    mkdocs.yml                        # Configuração do site (tema, navegação, extensões).
    docs/
        README.md                     # Página inicial do site.
        01-tap.md                     # Documentos numerados (entregas do projeto).
        02-requisitos.md
        03-eap.md
        04-projeto-conceitual/        # Seção: README (índice) + 4.1 a 4.4.
        05-cronograma.md
        06-orcamento.md
        07-testes/                    # Seção: README (índice) + 7.1 a 7.5.
        08-avaliacao-de-desempenho.md
        09-relatorio-de-encerramento.md
        figs/                         # Imagens usadas na documentação.
    .github/workflows/
        deploy-docs.yml               # Publicação automática no GitHub Pages.

## Como adicionar uma página

1. Crie o arquivo `.md` dentro de `docs/`.
2. Registre-o em `nav:` no `mkdocs.yml` (define o título e a ordem no menu).
3. Rode `mkdocs serve` para visualizar antes de commitar.

## O que pode ir na branch `documentacao`

Regra combinada pelo grupo: **a `documentacao` não recebe código**. A `main` pode receber
documentação normalmente.

| Pode ir na `documentacao` | Não pode (vai por PR para a `main`) |
|---|---|
| `docs/` (páginas e imagens) e `mkdocs.yml` | `src/` (firmware, backend e frontend, incluindo os `README.md` dessas pastas) |
| Arquivos de projeto das frentes: `hw/` (KiCad, PDFs) e `mec/` (CAD) | `scripts/` |
| Arquivos `.gitignore` | Workflows de CI do código (`.github/workflows/ci-*.yml`) |

Todo PR para a `documentacao` passa pelo check **"Sem código na documentação"**
(`.github/workflows/sem-codigo-na-documentacao.yml`). Se o PR alterar algum arquivo de código, o
check reprova e o resumo do workflow lista os arquivos: tire-os do PR ou abra um PR separado
para a `main`.

> **Regra do grupo:** PR para a `documentacao` com o check "Sem código na documentação"
> vermelho não é integrado.

Os arquivos de código continuam existindo na `documentacao`, porque toda branch tem a árvore
completa do repositório e a `documentacao` volta para a `main` nos marcos. **Não apague**
`src/` dela: no PR `documentacao → main`, a remoção apagaria o código da `main`. O check garante
que nenhuma *mudança* de código entre pela `documentacao`.

## Publicação

O site é publicado automaticamente pelo workflow `deploy-docs.yml` a cada push na
branch `documentacao` que altere `docs/`, `mkdocs.yml` ou o próprio workflow. O resultado
fica disponível em:

<https://fcte-pi1.github.io/2026_2_PI1_Grupo01_Hilmer/>

> **Configuração única:** em *Settings → Pages*, defina a origem como
> *Deploy from a branch* → **`gh-pages` / (root)**.
