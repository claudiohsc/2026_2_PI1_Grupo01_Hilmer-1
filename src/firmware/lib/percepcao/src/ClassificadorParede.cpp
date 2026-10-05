#include "ClassificadorParede.h"

namespace micromouse {
    ClassificadorParede::ClassificadorParede(uint16_t limiar) {
        limiarDistanciaMm = limiar;
    }
    
    ClassificadorParede::~ClassificadorParede() = default;

    void ClassificadorParede::definirLimiar(uint16_t limiarMm) {
        limiarDistanciaMm = limiarMm;
    }

    uint16_t ClassificadorParede::obterLimiar() const {
        return limiarDistanciaMm;
    }

    ClassificadorParede::Resultado ClassificadorParede::classificar(const std::vector<DistanciasLaterais>& amostras) const {
        Resultado resultado;
        if (amostras.empty()) {
            resultado.direita = EstadoParede::Parede;
            resultado.esquerda = EstadoParede::Parede;
            resultado.frente = EstadoParede::Parede;
            return resultado;
        }
        uint32_t somaDireita = 0;
        uint32_t somaEsquerda = 0;
        uint32_t somaFrente = 0;

        for (const auto& amostra : amostras) {
            somaDireita += amostra.direitaMm;
            somaEsquerda += amostra.esquerdaMm;
            somaFrente += amostra.frenteMm;
        }
        auto qtd = amostras.size();
        uint16_t mediaDireita = static_cast<uint16_t> (somaDireita / qtd);
        uint16_t mediaEsquerda = static_cast<uint16_t> (somaEsquerda / qtd);
        uint16_t mediaFrente = static_cast<uint16_t> (somaFrente / qtd);

        resultado.direita = (mediaDireita <= limiarDistanciaMm) ? EstadoParede::Parede : EstadoParede::Livre;
        resultado.esquerda = (mediaEsquerda <= limiarDistanciaMm) ? EstadoParede::Parede : EstadoParede::Livre;
        resultado.frente = (mediaFrente <= limiarDistanciaMm) ? EstadoParede::Parede : EstadoParede::Livre;

        return resultado;
    }
} // namespace micromouse