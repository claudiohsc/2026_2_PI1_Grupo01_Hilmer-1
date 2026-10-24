#include <unity.h>

#include "ClassificadorParede.h"
#include "ConfiguracaoPercepcao.h"
#include "LabirintoSimulado.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp(void) {}
void tearDown(void) {}

/// @brief Cenário 1: Obstáculo próximo (todas as médias abaixo do limiar de 180mm).
void test_classificador_parede_proxima(void) {
  ClassificadorParede classificador(180);

  DistanciasLaterais amostra[3] = {
      {100, 110, 45},
      {90, 85, 92},
      {80, 82, 88}
  };

  auto resultado = classificador.classificar(amostra, 3);

  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.direita);
}

/// @brief Cenário 2: Corredor livre (todas as médias acima do limiar de 180mm).
void test_classificador_passagem_livre(void) {
  ClassificadorParede classificador(180);

  DistanciasLaterais amostra[3] = {
      {300, 250, 400},
      {310, 260, 410},
      {290, 245, 390}
  };

  auto resultado = classificador.classificar(amostra, 3);

  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.direita);
}

/// @brief Cenário 3: Análise exata da condição de borda no limiar (180mm).
void test_classificador_valor_no_limiar(void) {
  ClassificadorParede classificador(180);

  DistanciasLaterais amostra[2] = {
      {180, 181, 180},
      {180, 181, 180}
  };

  auto resultado = classificador.classificar(amostra, 2);

  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.direita);
}

/// @brief Cenário 4: Validação de alteração e consulta do limiar configurável.
void test_classificador_limiar_configuravel(void) {
  ClassificadorParede classificador(180);

  classificador.definirLimiar(250);
  TEST_ASSERT_EQUAL_UINT16(250, classificador.obterLimiar());

  DistanciasLaterais amostra = {200, 200, 200};
  auto resultado = classificador.classificar(&amostra, 1);

  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.frente);
}

/// @brief Cenário 5: Filtragem de ruídos pontuais no sensor via cálculo da mediana.
void test_classificador_leituras_com_ruido(void) {
  ClassificadorParede classificador(180);

  DistanciasLaterais amostra[5] = {
      {100, 100, 300},
      {100, 100, 300},
      {300, 100, 50},
      {100, 100, 300},
      {100, 100, 300}
  };

  auto resultado = classificador.classificar(amostra, 5);

  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.direita);
}

/// @brief Cenário 6: Tratamento de caso de borda para vetor de amostra vazio.
void test_classificador_vetor_vazio(void) {
  ClassificadorParede classificador(180);

  DistanciasLaterais amostra = {};
  auto resultado = classificador.classificar(&amostra, 0);

  TEST_ASSERT_EQUAL(EstadoParede::Desconhecido, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Desconhecido, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Desconhecido, resultado.direita);

  auto resNull = classificador.classificar(nullptr, 5);
  TEST_ASSERT_EQUAL(EstadoParede::Desconhecido, resNull.frente);

  TEST_ASSERT_EQUAL_UINT16(0, classificador.filtrarAmostras(nullptr, 5));
  uint16_t v = 50;
  TEST_ASSERT_EQUAL_UINT16(0, classificador.filtrarAmostras(&v, 0));
}

/// @brief Cenário 7: Limiares independentes por lado (frente, esquerda e direita).
void test_classificador_limiares_independentes_por_lado(void) {
  ClassificadorParede classificador(140, 110, 160);

  TEST_ASSERT_EQUAL_UINT16(140, classificador.obterLimiar(Lado::Frente));
  TEST_ASSERT_EQUAL_UINT16(110, classificador.obterLimiar(Lado::Esquerda));
  TEST_ASSERT_EQUAL_UINT16(160, classificador.obterLimiar(Lado::Direita));

  // Amostra com 130mm em todas as direções:
  // Frente: 130 <= 140 -> Parede
  // Esquerda: 130 > 110 -> Livre
  // Direita: 130 <= 160 -> Parede
  DistanciasLaterais amostra = {130, 130, 130};
  auto resultado = classificador.classificar(amostra);

  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.direita);

  // Alteração dinâmica individual de limiar para todos os lados
  classificador.definirLimiar(Lado::Frente, 125);
  classificador.definirLimiar(Lado::Esquerda, 135);
  classificador.definirLimiar(Lado::Direita, 120);

  TEST_ASSERT_EQUAL_UINT16(125, classificador.obterLimiar(Lado::Frente));
  TEST_ASSERT_EQUAL_UINT16(135, classificador.obterLimiar(Lado::Esquerda));
  TEST_ASSERT_EQUAL_UINT16(120, classificador.obterLimiar(Lado::Direita));

  resultado = classificador.classificar(amostra);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.direita);

  // Alteração em bloco via definirLimiares
  classificador.definirLimiares(200, 210, 220);
  TEST_ASSERT_EQUAL_UINT16(200, classificador.obterLimiar(Lado::Frente));
  TEST_ASSERT_EQUAL_UINT16(210, classificador.obterLimiar(Lado::Esquerda));
  TEST_ASSERT_EQUAL_UINT16(220, classificador.obterLimiar(Lado::Direita));

  // Configuração completa
  ConfiguracaoPercepcao cfg = classificador.obterConfiguracao();
  cfg.limiarFrenteMm = 155;
  classificador.definirConfiguracao(cfg);
  TEST_ASSERT_EQUAL_UINT16(155, classificador.obterLimiar(Lado::Frente));
}

/// @brief Cenário 8: Tratamento explícito de leituras "abaixo do mínimo" (< 30 mm).
/// Leituras abaixo do mínimo são limitadas ao mínimo operacional e classificadas como Parede próxima.
void test_classificador_tratamento_abaixo_do_minimo(void) {
  ClassificadorParede classificador;

  // Verificação de detecção
  TEST_ASSERT_TRUE(classificador.ehAbaixoDoMinimo(0));
  TEST_ASSERT_TRUE(classificador.ehAbaixoDoMinimo(15));
  TEST_ASSERT_TRUE(classificador.ehAbaixoDoMinimo(29));
  TEST_ASSERT_FALSE(classificador.ehAbaixoDoMinimo(30));
  TEST_ASSERT_FALSE(classificador.ehAbaixoDoMinimo(100));

  // Verificação de saturação ao mínimo (30 mm)
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MINIMA_TOF_MM, classificador.tratarDistancia(0));
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MINIMA_TOF_MM, classificador.tratarDistancia(10));
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MINIMA_TOF_MM, classificador.tratarDistancia(29));
  TEST_ASSERT_EQUAL_UINT16(50, classificador.tratarDistancia(50));

  // Classificação direta: abaixo do mínimo conta como parede muito próxima
  TEST_ASSERT_EQUAL(EstadoParede::Parede, classificador.classificarDistancia(Lado::Frente, 10));
  TEST_ASSERT_EQUAL(EstadoParede::Parede, classificador.classificarDistancia(Lado::Esquerda, 0));
  TEST_ASSERT_EQUAL(EstadoParede::Parede, classificador.classificarDistancia(Lado::Direita, 25));

  DistanciasLaterais leitura = {5, 12, 20};
  auto resultado = classificador.classificar(leitura);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resultado.direita);
}

/// @brief Cenário 9: Tratamento explícito de "fora de alcance" (> 1200 mm ou código 8190 mm).
/// Leituras fora de alcance são limitadas a 1200 mm e classificadas como Livre.
void test_classificador_tratamento_fora_de_alcance(void) {
  ClassificadorParede classificador;

  // Verificação de detecção
  TEST_ASSERT_FALSE(classificador.ehForaDeAlcance(1200));
  TEST_ASSERT_TRUE(classificador.ehForaDeAlcance(1201));
  TEST_ASSERT_TRUE(classificador.ehForaDeAlcance(DISTANCIA_OUT_OF_RANGE_TOF_MM));
  TEST_ASSERT_TRUE(classificador.ehForaDeAlcance(65535));

  // Verificação de saturação ao máximo (1200 mm)
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MAXIMA_TOF_MM, classificador.tratarDistancia(1201));
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MAXIMA_TOF_MM, classificador.tratarDistancia(DISTANCIA_OUT_OF_RANGE_TOF_MM));
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MAXIMA_TOF_MM, classificador.tratarDistancia(8191));

  // Classificação direta: fora de alcance conta como lado livre
  TEST_ASSERT_EQUAL(EstadoParede::Livre,
                    classificador.classificarDistancia(Lado::Frente, DISTANCIA_OUT_OF_RANGE_TOF_MM));
  TEST_ASSERT_EQUAL(EstadoParede::Livre,
                    classificador.classificarDistancia(Lado::Esquerda, 8191));
  TEST_ASSERT_EQUAL(EstadoParede::Livre,
                    classificador.classificarDistancia(Lado::Direita, 2000));

  DistanciasLaterais leitura = {8190, 8190, 8190};
  auto resultado = classificador.classificar(leitura);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, resultado.direita);
}

/// @brief Cenário 10: Filtro de mediana descartando spikes espúrios (out-of-range e drop a zero).
void test_classificador_filtro_mediana_rejeita_outliers(void) {
  ClassificadorParede classificador; // usa mediana por padrão e limiares 140 mm

  // Caso 1: Parede real à frente (54 mm), mas 1 leitura espúria de fora de alcance (8190 mm)
  // Amostras: { 54, 52, 8190, 55, 53 }
  // A mediana rejeita o 8190 e mantém ~54 mm <= 140 mm -> Parede
  DistanciasLaterais amostrasParedeComSpike[5] = {
      {54, 250, 250},
      {52, 260, 240},
      {8190, 255, 255},
      {55, 245, 248},
      {53, 252, 251}
  };
  auto res1 = classificador.classificar(amostrasParedeComSpike, 5);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, res1.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, res1.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, res1.direita);

  // Caso 2: Corredor livre à esquerda (250 mm), mas 1 leitura espúria de queda a 0 mm
  // Amostras: { 250, 255, 0, 248, 252 }
  // A mediana rejeita o 0 e mantém ~250 mm > 140 mm -> Livre
  DistanciasLaterais amostrasLivreComDrop[5] = {
      {50, 250, 50},
      {52, 255, 52},
      {48, 0, 49},
      {51, 248, 51},
      {53, 252, 53}
  };
  auto res2 = classificador.classificar(amostrasLivreComDrop, 5);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, res2.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, res2.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, res2.direita);

  // Caso 3: Mediana com número par de amostras (4 amostras)
  uint16_t amostrasPar[4] = {40, 60, 80, 100};
  // Mediana de 4 amostras: (60 + 80) / 2 = 70
  TEST_ASSERT_EQUAL_UINT16(70, classificador.filtrarAmostras(amostrasPar, 4));

  // Caso 4: Amostras excedendo capacidade máxima do buffer estático (> 16)
  uint16_t amostrasMuitas[20];
  DistanciasLaterais muitasLaterais[20];
  for (int i = 0; i < 20; i++) {
    amostrasMuitas[i] = 100;
    muitasLaterais[i] = {100, 100, 100};
  }
  TEST_ASSERT_EQUAL_UINT16(100, classificador.filtrarAmostras(amostrasMuitas, 20));
  auto resMuitas = classificador.classificar(muitasLaterais, 20);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, resMuitas.frente);
}

/// @brief Cenário 11: Filtro configurável de média aritmética.
void test_classificador_filtro_media_aritmetica(void) {
  ConfiguracaoPercepcao config;
  config.limiarFrenteMm = 140;
  config.tipoFiltro = TipoFiltroPercepcao::Media;
  ClassificadorParede classificador(config);

  // Média de {100, 120, 110} = 110 mm <= 140 mm -> Parede
  DistanciasLaterais amostras[3] = {
      {100, 250, 250},
      {120, 260, 270},
      {110, 255, 265}
  };
  auto res = classificador.classificar(amostras, 3);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, res.frente);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, res.esquerda);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, res.direita);

  uint16_t v[3] = {50, 70, 90};
  TEST_ASSERT_EQUAL_UINT16(70, classificador.filtrarAmostras(v, 3));
}

namespace {

/// Helper para testar a classificação de todas as células de um labirinto simulado,
/// injetando ruído gaussiano/jitter e leituras espúrias ocasionais nas amostras ToF.
void conferirLabirintoComRuido(const char* texto) {
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(texto)));

  ClassificadorParede classificador;

  const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};

  for (uint8_t linha = 0; linha < labirinto.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < labirinto.obterColunas(); coluna++) {
      for (Direcao d : direcoes) {
        const Pose pose = {{linha, coluna}, d};
        DistanciasLaterais nominal = {};
        TEST_ASSERT_TRUE(labirinto.distancias(pose, nominal));

        // Cria lote de 5 amostras com ruído e 1 leitura espúria
        DistanciasLaterais amostras[5];
        const int16_t ruidao[5] = {-12, +15, 0, -8, +10};

        for (int i = 0; i < 5; i++) {
          amostras[i].frenteMm = static_cast<uint16_t>(nominal.frenteMm + ruidao[i]);
          amostras[i].esquerdaMm = static_cast<uint16_t>(nominal.esquerdaMm + ruidao[(i + 1) % 5]);
          amostras[i].direitaMm = static_cast<uint16_t>(nominal.direitaMm + ruidao[(i + 2) % 5]);
        }

        // Injeta 1 leitura espúria no meio do lote (índice 2):
        // Se parede: simula spike para 8190 mm (fora de alcance).
        // Se livre: simula drop para 10 mm (abaixo do mínimo).
        if (nominal.frenteMm <= 140) {
          amostras[2].frenteMm = DISTANCIA_OUT_OF_RANGE_TOF_MM;
        } else {
          amostras[2].frenteMm = 10;
        }

        if (nominal.esquerdaMm <= 140) {
          amostras[2].esquerdaMm = DISTANCIA_OUT_OF_RANGE_TOF_MM;
        } else {
          amostras[2].esquerdaMm = 10;
        }

        if (nominal.direitaMm <= 140) {
          amostras[2].direitaMm = DISTANCIA_OUT_OF_RANGE_TOF_MM;
        } else {
          amostras[2].direitaMm = 10;
        }

        auto classif = classificador.classificar(amostras, 5);

        const EstadoParede esperadoFrente =
            (nominal.frenteMm <= 140) ? EstadoParede::Parede : EstadoParede::Livre;
        const EstadoParede esperadoEsquerda =
            (nominal.esquerdaMm <= 140) ? EstadoParede::Parede : EstadoParede::Livre;
        const EstadoParede esperadoDireita =
            (nominal.direitaMm <= 140) ? EstadoParede::Parede : EstadoParede::Livre;

        TEST_ASSERT_EQUAL(esperadoFrente, classif.frente);
        TEST_ASSERT_EQUAL(esperadoEsquerda, classif.esquerda);
        TEST_ASSERT_EQUAL(esperadoDireita, classif.direita);
      }
    }
  }
}

}  // namespace

/// @brief Cenário 12: Classificação com ruído em todas as células do Labirinto Simulado 4×4.
void test_classificador_labirinto_simulado_4x4_com_ruido(void) {
  conferirLabirintoComRuido(exemplos::LABIRINTO_4X4);
}

/// @brief Cenário 13: Classificação com ruído em todas as células do Labirinto Simulado 8×4.
void test_classificador_labirinto_simulado_8x4_com_ruido(void) {
  conferirLabirintoComRuido(exemplos::LABIRINTO_8X4);
}

/// @brief Cenário 14: Classificação com ruído em todas as células do Labirinto Simulado 12×4.
void test_classificador_labirinto_simulado_12x4_com_ruido(void) {
  conferirLabirintoComRuido(exemplos::LABIRINTO_12X4);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  RUN_TEST(test_classificador_parede_proxima);
  RUN_TEST(test_classificador_passagem_livre);
  RUN_TEST(test_classificador_valor_no_limiar);
  RUN_TEST(test_classificador_limiar_configuravel);
  RUN_TEST(test_classificador_leituras_com_ruido);
  RUN_TEST(test_classificador_vetor_vazio);
  RUN_TEST(test_classificador_limiares_independentes_por_lado);
  RUN_TEST(test_classificador_tratamento_abaixo_do_minimo);
  RUN_TEST(test_classificador_tratamento_fora_de_alcance);
  RUN_TEST(test_classificador_filtro_mediana_rejeita_outliers);
  RUN_TEST(test_classificador_filtro_media_aritmetica);
  RUN_TEST(test_classificador_labirinto_simulado_4x4_com_ruido);
  RUN_TEST(test_classificador_labirinto_simulado_8x4_com_ruido);
  RUN_TEST(test_classificador_labirinto_simulado_12x4_com_ruido);

  return UNITY_END();
}
