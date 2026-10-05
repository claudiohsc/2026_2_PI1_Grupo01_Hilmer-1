#include <unity.h>
#include "ClassificadorParede.h"

// Executados pelo Unity antes e depois de cada teste individual
void setUp(void) {}
void tearDown(void) {}

/**
 * @brief Cenário 1: Obstáculo próximo (todas as médias abaixo do limiar de 180mm).
 * 
 * Verifica se leituras curtas são classificadas como 'Parede' para todas as direções.
 * - Médias calculadas: Frente = 101.6mm | Esquerda = 89mm | Direita = 83.3mm
 */
void test_classificador_parede_proxima(void) {
    micromouse::ClassificadorParede classificador(180);

    std::vector<micromouse::DistanciasLaterais> amostras = {
        {.frenteMm = 100, .esquerdaMm = 90, .direitaMm = 80},
        {.frenteMm = 110, .esquerdaMm = 85, .direitaMm = 82},
        {.frenteMm = 95,  .esquerdaMm = 92, .direitaMm = 88}
    };

    auto resultado = classificador.classificar(amostras);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.direita);
}

/**
 * @brief Cenário 2: Corredor livre (todas as médias acima do limiar de 180mm).
 * 
 * Verifica se distâncias longas são identificadas como 'Livre' para todas as direções.
 * - Médias calculadas: Frente = 300mm | Esquerda = 251.6mm | Direita = 400mm
 */
void test_classificador_passagem_livre(void) {
    micromouse::ClassificadorParede classificador(180);

    std::vector<micromouse::DistanciasLaterais> amostras = {
        {.frenteMm = 300, .esquerdaMm = 250, .direitaMm = 400},
        {.frenteMm = 310, .esquerdaMm = 260, .direitaMm = 410},
        {.frenteMm = 290, .esquerdaMm = 245, .direitaMm = 390}
    };

    auto resultado = classificador.classificar(amostras);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.direita);
}

/**
 * @brief Cenário 3: Análise exata da condição de borda no limiar (180mm).
 * 
 * Valida a regra de decisão: <= 180mm é 'Parede', enquanto > 180mm é 'Livre'.
 * - Média Frente: 180mm (<= 180 -> Parede)
 * - Média Esquerda: 181mm (> 180 -> Livre)
 * - Média Direita: 180mm (<= 180 -> Parede)
 */
void test_classificador_valor_no_limiar(void) {
    micromouse::ClassificadorParede classificador(180);

    std::vector<micromouse::DistanciasLaterais> amostras = {
        {.frenteMm = 180, .esquerdaMm = 181, .direitaMm = 180},
        {.frenteMm = 180, .esquerdaMm = 181, .direitaMm = 180}
    };

    auto resultado = classificador.classificar(amostras);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.direita);
}

/**
 * @brief Cenário 4: Filtragem de ruídos pontuais no sensor via cálculo da média.
 * 
 * Demonstra que picos espúrios em leituras isoladas são atenuados pelo vetor de amostras.
 * - Média Frente: (100+100+300+100+100)/5 = 140mm (Parede) -> Filtra o pico de 300mm
 * - Média Esquerda: 500/5 = 100mm (Parede)
 * - Média Direita: 1250/5 = 250mm (Livre) -> Filtra a queda momentânea de 50mm
 */
void test_classificador_leituras_com_ruido(void) {
    micromouse::ClassificadorParede classificador(180);

    std::vector<micromouse::DistanciasLaterais> amostras = {
        {.frenteMm = 100, .esquerdaMm = 100, .direitaMm = 300},
        {.frenteMm = 100, .esquerdaMm = 100, .direitaMm = 300},
        {.frenteMm = 300, .esquerdaMm = 100, .direitaMm = 50}, // Ruídos isolados na frente e direita
        {.frenteMm = 100, .esquerdaMm = 100, .direitaMm = 300},
        {.frenteMm = 100, .esquerdaMm = 100, .direitaMm = 300}
    };

    auto resultado = classificador.classificar(amostras);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.direita);
}

/**
 * @brief Cenário 5: Tratamento de caso de borda para vetor de amostras vazio.
 * 
 * Atua como proteção defensiva (fail-safe): sem dados dos sensores, assume
 * 'Parede' para evitar colisões na navegação do robô.
 */
void test_classificador_vetor_vazio(void) {
    micromouse::ClassificadorParede classificador(180);

    std::vector<micromouse::DistanciasLaterais> amostras = {};

    auto resultado = classificador.classificar(amostras);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.direita);
}

// Ponto de entrada do executável de testes nativos (Unity)
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_classificador_parede_proxima);
    RUN_TEST(test_classificador_passagem_livre);
    RUN_TEST(test_classificador_valor_no_limiar);
    RUN_TEST(test_classificador_leituras_com_ruido);
    RUN_TEST(test_classificador_vetor_vazio);

    return UNITY_END();
}