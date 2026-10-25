"""Testes das etapas de execução dos movimentos (#184) do menu de bancada do deslocamento
(`scripts/bancada_deslocamento.py`).

Não precisam da placa nem do `pyserial`: a `Bancada` é trocada por uma que responde a cada comando
com o JSON combinado no teste, no formato que o `bancada_deslocamento.cpp` imprime. Assim cada
etapa é conferida (APROVADO ou REPROVADO) a partir do que a placa responde e do que a equipe mede.

Uso (a partir de `src/firmware`):
    python3 -m unittest discover -s scripts/testes -v
"""

import contextlib
import io
import os
import sys
import tempfile
import types
import unittest
from unittest import mock

PASTA_SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# O script sai se não achar o `pyserial`; os testes não usam a porta real.
_serial = types.ModuleType("serial")
_serial.Serial = None
_serial.SerialException = Exception
_serial.tools = types.ModuleType("serial.tools")
_serial.tools.list_ports = types.ModuleType("serial.tools.list_ports")
sys.modules.setdefault("serial", _serial)
sys.modules.setdefault("serial.tools", _serial.tools)
sys.modules.setdefault("serial.tools.list_ports", _serial.tools.list_ports)

sys.path.insert(0, PASTA_SCRIPTS)
import bancada_deslocamento  # noqa: E402  (precisa do `serial` falso acima)


class BancadaFalsa(bancada_deslocamento.Bancada):
    """`Bancada` cujos comandos devolvem as respostas combinadas, em ordem, por comando.

    As linhas de `ao_receber` (o log e os `fim_execucao` de cada célula) também são combinadas.
    """

    def __init__(self, respostas, recebidas=None, pasta=None):
        super().__init__("/dev/falsa")
        self.serial = object()
        self.respostas = {texto: list(lista) for texto, lista in respostas.items()}
        self.recebidas = {texto: list(lista) for texto, lista in (recebidas or {}).items()}
        self.enviados = []
        if pasta is not None:
            self.pasta_logs = pasta

    def comando(self, texto, tipo, espera_s=5.0, ao_receber=None):
        self.enviados.append(texto)
        if ao_receber is not None:
            for dados in self.recebidas.get(texto, []):
                ao_receber(dados)
        fila = self.respostas.get(texto)
        return fila.pop(0) if fila else None

    def situacoes(self):
        return {etapa: situacao for etapa, situacao, _ in self.resultados}


def fim_execucao(movimento="avancar_celula", estado="concluido", falha="nenhuma", restante=2, giro=0.0):
    return {"tipo": "fim_execucao", "movimento": movimento, "estado": estado, "falha": falha,
            "interrompido": False, "tempo_ms": 1800, "restante_mm": restante, "avanco_norte_mm": 178.0,
            "avanco_leste_mm": 0.0, "giro_medido_graus": giro, "erro_rumo_graus": 0.3, "orientacao": "N"}


def log():
    return {"tipo": "log", "t_ms": 50, "norte_mm": 6.0, "leste_mm": 0.0, "linha": 0, "coluna": 0,
            "orientacao": "N", "pot_esq": 400, "pot_dir": 400, "tof_frente_mm": 500, "tof_esq_mm": 54,
            "tof_dir_mm": 54}


@contextlib.contextmanager
def sem_interacao(respostas_input=()):
    """Responde ao `input()` com os textos dados (ENTER depois deles) e esconde a saída."""
    fila = list(respostas_input)
    with mock.patch("builtins.input", side_effect=lambda *_: fila.pop(0) if fila else ""), \
            contextlib.redirect_stdout(io.StringIO()):
        yield


class TestAvaliacao(unittest.TestCase):
    def test_parada_depois_de_n_celulas(self):
        self.assertEqual((5.0, True), bancada_deslocamento.avaliar_celulas(2, 365.0))
        self.assertEqual((-12.0, False), bancada_deslocamento.avaliar_celulas(3, 528.0))
        self.assertTrue(bancada_deslocamento.avaliar_celulas(1, 190.0)[1])  # no limite

    def test_giro_executado(self):
        self.assertEqual((2.0, True), bancada_deslocamento.avaliar_giro(90.0, 92.0))
        self.assertEqual((-4.0, False), bancada_deslocamento.avaliar_giro(180.0, 176.0))

    def test_limite_lateral_abaixo_da_folga_do_giro(self):
        self.assertLess(bancada_deslocamento.ERRO_LATERAL_MM, 5.9)


class TestEtapaCelulas(unittest.TestCase):
    def setUp(self):
        self.pasta = tempfile.mkdtemp()

    def bancada(self, fim_celulas, fins):
        return BancadaFalsa({"ZERAR": [{"tipo": "estado"}], "CELULA 3": [fim_celulas]},
                            recebidas={"CELULA 3": [log()] + fins}, pasta=self.pasta)

    def test_tres_celulas_dentro_do_limite(self):
        bancada = self.bancada({"tipo": "fim_celulas", "pedidas": 3, "feitas": 3},
                               [fim_execucao(restante=2), fim_execucao(restante=1), fim_execucao(restante=3)])
        with sem_interacao(["3", "", "546", "2"]):
            bancada_deslocamento.verificar_execucao_celulas(bancada)
        self.assertEqual(["ZERAR", "CELULA 3"], bancada.enviados)
        situacoes = bancada.situacoes()
        self.assertEqual("APROVADO", situacoes["Execução, parada depois de n células"])
        self.assertEqual("APROVADO", situacoes["Execução, desvio lateral no fim"])
        detalhe = [d for e, _, d in bancada.resultados if e == "Execução, parada depois de n células"][0]
        self.assertIn("+2, +1, +3", detalhe)
        self.assertTrue(any(nome.endswith("execucao_celula_3.csv") for nome in os.listdir(self.pasta)))

    def test_parada_longe_e_desvio_lateral_reprovam(self):
        bancada = self.bancada({"tipo": "fim_celulas", "pedidas": 3, "feitas": 3}, [fim_execucao()] * 3)
        with sem_interacao(["3", "", "520", "-7"]):
            bancada_deslocamento.verificar_execucao_celulas(bancada)
        situacoes = bancada.situacoes()
        self.assertEqual("REPROVADO", situacoes["Execução, parada depois de n células"])
        self.assertEqual("REPROVADO", situacoes["Execução, desvio lateral no fim"])

    def test_movimento_que_falha_no_meio_reprova_sem_perguntar_a_medida(self):
        bancada = self.bancada({"tipo": "fim_celulas", "pedidas": 3, "feitas": 1},
                               [fim_execucao(), fim_execucao(estado="falha", falha="parede_na_frente")])
        with sem_interacao(["3", ""]):
            bancada_deslocamento.verificar_execucao_celulas(bancada)
        self.assertEqual({"Execução, avanço de célula": "REPROVADO"}, bancada.situacoes())
        self.assertIn("parede_na_frente", bancada.resultados[0][2])

    def test_sem_resposta_nao_registra(self):
        bancada = BancadaFalsa({"ZERAR": [{"tipo": "estado"}]}, pasta=self.pasta)
        with sem_interacao(["3", ""]):
            bancada_deslocamento.verificar_execucao_celulas(bancada)
        self.assertEqual([], bancada.resultados)

    def test_quantidade_de_celulas_limitada(self):
        bancada = BancadaFalsa({"ZERAR": [{"tipo": "estado"}]}, pasta=self.pasta)
        with sem_interacao(["20", ""]):
            bancada_deslocamento.verificar_execucao_celulas(bancada)
        self.assertIn("CELULA 8", bancada.enviados)


class TestEtapaGiros(unittest.TestCase):
    def setUp(self):
        self.pasta = tempfile.mkdtemp()

    def respostas(self, giros):
        respostas = {"CALIBRAR": [{"tipo": "calibracao", "estado": "calibrado", "recusadas": 0}],
                     "ZERAR": [{"tipo": "estado"}]}
        for (comando, _, _), fim in zip(bancada_deslocamento.GIROS_EXECUTADOS, giros):
            respostas[comando] = [fim]
        return respostas

    def test_tres_giros_medidos(self):
        bancada = BancadaFalsa(self.respostas([fim_execucao("girar_90_direita", giro=-89.5),
                                               fim_execucao("girar_90_esquerda", giro=90.2),
                                               fim_execucao("girar_180", giro=179.6)]), pasta=self.pasta)
        # ENTER na calibração; para cada giro, ENTER na pausa e o ângulo medido.
        with sem_interacao(["", "", "91", "", "88.5", "", "184"]):
            bancada_deslocamento.verificar_execucao_giros(bancada)
        self.assertEqual(["CALIBRAR", "ZERAR", "GIRO90D", "GIRO90E", "GIRO180"], bancada.enviados)
        situacoes = bancada.situacoes()
        self.assertEqual("APROVADO", situacoes["Execução, giro 90° para a direita"])
        self.assertEqual("APROVADO", situacoes["Execução, giro 90° para a esquerda"])
        self.assertEqual("REPROVADO", situacoes["Execução, giro 180° (pela esquerda)"])

    def test_giro_que_esgota_o_tempo_reprova(self):
        bancada = BancadaFalsa(self.respostas([fim_execucao("girar_90_direita", "falha", "giro_tempo_esgotado"),
                                               fim_execucao("girar_90_esquerda", giro=90.0),
                                               fim_execucao("girar_180", giro=180.0)]), pasta=self.pasta)
        with sem_interacao(["", "", "", "90", "", "180"]):
            bancada_deslocamento.verificar_execucao_giros(bancada)
        situacoes = bancada.situacoes()
        self.assertEqual("REPROVADO", situacoes["Execução, giro 90° para a direita"])
        self.assertEqual("APROVADO", situacoes["Execução, giro 180° (pela esquerda)"])

    def test_sem_giroscopio_calibrado_nao_gira(self):
        bancada = BancadaFalsa({"CALIBRAR": [{"tipo": "calibracao", "estado": "sem_imu"}]}, pasta=self.pasta)
        with sem_interacao([""]):
            bancada_deslocamento.verificar_execucao_giros(bancada)
        self.assertEqual(["CALIBRAR"], bancada.enviados)
        self.assertEqual({"Execução, giros": "REPROVADO"}, bancada.situacoes())


class TestMenu(unittest.TestCase):
    def test_etapas_novas_no_menu_e_no_roteiro(self):
        funcoes = [funcao for _, funcao in bancada_deslocamento.MENU]
        self.assertIn(bancada_deslocamento.verificar_execucao_celulas, funcoes)
        self.assertIn(bancada_deslocamento.verificar_execucao_giros, funcoes)


if __name__ == "__main__":
    unittest.main()
