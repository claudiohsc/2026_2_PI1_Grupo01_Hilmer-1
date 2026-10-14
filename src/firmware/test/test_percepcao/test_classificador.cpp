#include <unity.h>
#include "ClassificadorParede.h"

// Executados pelo Unity antes e depois de cada teste individual
void setUp(void) {}
void tearDown(void) {}

 /// @brief Cenário 1: Obstáculo próximo (todas as médias abaixo do limiar de 180mm).
 /// 
 /// Verifica se leituras curtas são classificadas como 'Parede' para todas as direções.
 /// - Médias calculadas: Frente = 90mm | Esquerda = 92.3mm | Direita = 75mm
void test_classificador_parede_proxima(void) {
    micromouse::ClassificadorParede classificador(180);

    micromouse::DistanciasLaterais amostra[3] = {
        { 100, 110, 45 },
        { 90, 85, 92 },
        { 80, 82, 88 }
    };

    auto resultado = classificador.classificar(amostra, 3);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.direita);
}

 /// @brief Cenário 2: Corredor livre (todas as médias acima do limiar de 180mm).
 /// 
 /// Verifica se distâncias longas são identificadas como 'Livre' para todas as direções.
 /// - Médias calculadas: Frente = 300mm | Esquerda = 251.6mm | Direita = 400mm
void test_classificador_passagem_livre(void) {
    micromouse::ClassificadorParede classificador(180);

    micromouse::DistanciasLaterais amostra[3] = {
        { 300, 250 , 400 },
        { 310, 260, 410 },
        { 290, 245, 390 }
    };

    auto resultado = classificador.classificar(amostra, 3);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.direita);
}

 /// @brief Cenário 3: Análise exata da condição de borda no limiar (180mm).
 /// 
 /// Valida a regra de decisão: <= 180mm é 'Parede', enquanto > 180mm é 'Livre'.
 /// - Média Frente: 180mm (<=     -> Parede)
 /// - Média Esquerda: 181mm (> 180 -> Livre)
 /// Média Direita: 180mm (<= 180 -> Parede)
 
void test_classificador_valor_no_limiar(void) {
    micromouse::ClassificadorParede classificador(180);

    micromouse::DistanciasLaterais amostra[2] = {
        { 180, 181, 180},
        { 180, 181, 180}
    };

    auto resultado = classificador.classificar(amostra, 2);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.direita);
}
 /// @brief Cenário 4: Validação de alteração e consulta do limiar configurável.
 ///
 /// Valida os métodos `definirLimiar()` e `obterLimiar()`, garantindo o ajuste dinâmico
 /// do limiar de detecção em tempo de execução:
 /// - Consulta limiar inicial padrão (180mm).
 /// - Altera o limiar para 250mm via `definirLimiar(250)`.
 /// - Reavalia amostra de 200mm com o novo limiar:
 /// - 200mm (<= 250mm -> 'Parede', enquanto com 180mm seria 'Livre').

void test_classificador_limiar_configuravel(void) {
    micromouse::ClassificadorParede classificador(180);
    
    // Altera o limiar para 250 mm
    classificador.definirLimiar(250);
    TEST_ASSERT_EQUAL_UINT16(250, classificador.obterLimiar());

    // Com o novo limiar de 250 mm, uma leitura de 200 mm deve ser classificada como Parede (com 180 mm seria Livre)
    micromouse::DistanciasLaterais amostra = {200, 200, 200};
    auto resultado = classificador.classificar(&amostra, 1);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
}

 /// @brief Cenário 5: Filtragem de ruídos pontuais no sensor via cálculo da média.
 /// 
 /// Demonstra que picos espúrios em leituras isoladas são atenuados pelo vetor de amostra.
 /// - Média Frente: (100+100+300+100+100)/5 = 140mm (Parede) -> Filtra o pico de 300mm
 /// - Média Esquerda: 500/5 = 100mm (Parede)
 /// Média Direita: 1250/5 = 250mm (Livre) -> Filtra a queda momentânea de 50mm
 
void test_classificador_leituras_com_ruido(void) {
    micromouse::ClassificadorParede classificador(180);

    micromouse::DistanciasLaterais amostra[5] = {
        { 100, 100, 300},
        { 100, 100, 300},
        { 300, 100, 50},
        { 100, 100, 300},
        { 100, 100, 300}
    };

    auto resultado = classificador.classificar(amostra, 5);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Parede, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Livre, resultado.direita);
}

 /// @brief Cenário 6: Tratamento de caso de borda para vetor de amostra vazio.
 /// 
 /// Atua como proteção defensiva (fail-safe): sem dados dos sensores, assume
 /// 'Desconhecido' para evitar colisões na navegação do robô.
void test_classificador_vetor_vazio(void) {
    micromouse::ClassificadorParede classificador(180);

    micromouse::DistanciasLaterais amostra = {};

    auto resultado = classificador.classificar(&amostra, 0);

    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Desconhecido, resultado.frente);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Desconhecido, resultado.esquerda);
    TEST_ASSERT_EQUAL(micromouse::EstadoParede::Desconhecido, resultado.direita);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_classificador_parede_proxima);
    RUN_TEST(test_classificador_passagem_livre);
    RUN_TEST(test_classificador_valor_no_limiar);
    RUN_TEST(test_classificador_limiar_configuravel);
    RUN_TEST(test_classificador_leituras_com_ruido);
    RUN_TEST(test_classificador_vetor_vazio);

    return UNITY_END();
}