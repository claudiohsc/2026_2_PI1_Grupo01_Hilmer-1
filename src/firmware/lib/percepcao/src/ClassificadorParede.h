#pragma once

#include <vector>
#include <cstdint>
#include "Tipos.h"

namespace micromouse {
    class ClassificadorParede {
        public:
            struct Resultado {
                EstadoParede frente;
                EstadoParede direita;
                EstadoParede esquerda;
            };

            explicit ClassificadorParede(uint16_t limiarMm = 180);
            ~ClassificadorParede();

            void definirLimiar(uint16_t limiarMm);
            uint16_t obterLimiar() const;
            Resultado classificar(const std::vector<DistanciasLaterais>& amostras) const;

        private:
            uint16_t limiarDistanciaMm;
    };
    
} // namespace micromouse