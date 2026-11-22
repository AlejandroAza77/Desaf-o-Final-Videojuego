#include "nivelrioteatinos.h"
#include "germanquintero.h"
#include "armas.h"
#include <algorithm>
#include <memory>
#include <typeinfo>

NivelRioTeatinos::NivelRioTeatinos(double tiempoInicial,
                                   GermanQuintero* jugador,
                                   int carretasTotales,
                                   double intervaloAparicion,
                                   const QPointF& corrienteRio,
                                   const QPointF& inicioPuente,
                                   const QPointF& finPuente)
    : NivelDeBatalla(tiempoInicial, jugador),
    m_carretasTotales(carretasTotales),
    m_carretasDestruidas(0),
    m_carretasGeneradas(0),
    m_intervaloAparicion(intervaloAparicion),
    m_temporizadorAparicion(0.0),
    m_corrienteRio(corrienteRio),
    m_inicioPuente(inicioPuente),
    m_finPuente(finPuente),
    m_algunaCruzoPuente(false) {
    if (m_carretasTotales <= 0) {
        throw std::invalid_argument("Error: el total de carretas debe ser mayor que cero.");
    }
    if (m_intervaloAparicion <= 0.0) {
        throw std::invalid_argument("Error: el intervalo de aparicion debe ser mayor que cero.");
    }

    m_carretas.reserve(static_cast<std::size_t>(m_carretasTotales));
    configurarCorrienteJugador();
}

void NivelRioTeatinos::liberarCarretas() {
    for (CarretaDeCanon* c : m_carretas) {
        delete c;
    }
    m_carretas.clear();
}

void NivelRioTeatinos::copiarCarretas(const std::vector<CarretaDeCanon*>& origen,
                                      std::vector<CarretaDeCanon*>& destino) {
    std::vector<CarretaDeCanon*> temporal;
    temporal.reserve(origen.size());
    try {
        for (const CarretaDeCanon* c : origen) {
            if (c != nullptr) {
                std::unique_ptr<CarretaDeCanon> copia(new CarretaDeCanon(*c));
                temporal.push_back(copia.get());
                copia.release();
            }
        }
    } catch (...) {
        for (CarretaDeCanon* p : temporal) {
            delete p;
        }
        throw;
    }
    destino.swap(temporal);
}

NivelRioTeatinos::NivelRioTeatinos(const NivelRioTeatinos& otro)
    : NivelDeBatalla(otro),
    m_carretasTotales(otro.m_carretasTotales),
    m_carretasDestruidas(otro.m_carretasDestruidas),
    m_carretasGeneradas(otro.m_carretasGeneradas),
    m_intervaloAparicion(otro.m_intervaloAparicion),
    m_temporizadorAparicion(otro.m_temporizadorAparicion),
    m_corrienteRio(otro.m_corrienteRio),
    m_inicioPuente(otro.m_inicioPuente),
    m_finPuente(otro.m_finPuente),
    m_algunaCruzoPuente(otro.m_algunaCruzoPuente) {
    copiarCarretas(otro.m_carretas, m_carretas);
    configurarCorrienteJugador();
}

NivelRioTeatinos::~NivelRioTeatinos() {
    liberarCarretas();
}

NivelRioTeatinos& NivelRioTeatinos::operator=(const NivelRioTeatinos& otro) {
    if (this == &otro) {
        return *this;
    }

    std::vector<CarretaDeCanon*> nuevasCarretas;
    copiarCarretas(otro.m_carretas, nuevasCarretas);

    NivelDeBatalla::operator=(otro);
    liberarCarretas();
    m_carretas.swap(nuevasCarretas);

    m_carretasTotales = otro.m_carretasTotales;
    m_carretasDestruidas = otro.m_carretasDestruidas;
    m_carretasGeneradas = otro.m_carretasGeneradas;
    m_intervaloAparicion = otro.m_intervaloAparicion;
    m_temporizadorAparicion = otro.m_temporizadorAparicion;
    m_corrienteRio = otro.m_corrienteRio;
    m_inicioPuente = otro.m_inicioPuente;
    m_finPuente = otro.m_finPuente;
    m_algunaCruzoPuente = otro.m_algunaCruzoPuente;

    configurarCorrienteJugador();
    return *this;
}

bool NivelRioTeatinos::operator==(const NivelRioTeatinos& otro) const {
    if (!NivelDeBatalla::operator==(otro)) {
        return false;
    }
    if (m_carretas.size() != otro.m_carretas.size()) {
        return false;
    }
    for (std::size_t i = 0; i < m_carretas.size(); ++i) {
        const CarretaDeCanon* a = m_carretas[i];
        const CarretaDeCanon* b = otro.m_carretas[i];
        if ((a == nullptr) != (b == nullptr)) {
            return false;
        }
        if (a != nullptr && b != nullptr && !(*a == *b)) {
            return false;
        }
    }

    return m_carretasTotales == otro.m_carretasTotales &&
           m_carretasDestruidas == otro.m_carretasDestruidas &&
           m_carretasGeneradas == otro.m_carretasGeneradas &&
           casiIgual(m_intervaloAparicion, otro.m_intervaloAparicion) &&
           casiIgual(m_temporizadorAparicion, otro.m_temporizadorAparicion) &&
           m_corrienteRio == otro.m_corrienteRio &&
           m_inicioPuente == otro.m_inicioPuente &&
           m_finPuente == otro.m_finPuente &&
           m_algunaCruzoPuente == otro.m_algunaCruzoPuente;
}

void NivelRioTeatinos::configurarCorrienteJugador() {
    if (m_jugador != nullptr) {
        m_jugador->setCorriente(m_corrienteRio);
    }
}

void NivelRioTeatinos::agregarCarreta(CarretaDeCanon* nuevaCarreta) {
    if (nuevaCarreta == nullptr) {
        throw std::invalid_argument("Error: no se puede agregar una carreta nula.");
    }
    if (static_cast<int>(m_carretas.size()) >= m_carretasTotales) {
        m_carretasTotales = static_cast<int>(m_carretas.size()) + 1;
    }
    m_carretas.push_back(nuevaCarreta);
    ++m_carretasGeneradas;
}

void NivelRioTeatinos::registrarCarretaDestruida(CarretaDeCanon* carreta) {
    if (carreta != nullptr) {
        carreta->desactivar();
    }
    if (m_carretasDestruidas < m_carretasTotales) {
        ++m_carretasDestruidas;
    }
}

void NivelRioTeatinos::generarCarretaSiCorresponde(double dt) {
    if (m_carretasGeneradas >= m_carretasTotales) {
        return;
    }

    // Genera la primera carreta de inmediato y las siguientes cada m_intervaloAparicion segundos
    if (m_carretasGeneradas == 0) {
        auto* nueva = new CarretaDeCanon(m_inicioPuente, m_finPuente);
        m_carretas.push_back(nueva);
        ++m_carretasGeneradas;
        m_temporizadorAparicion = 0.0;
        return;
    }

    m_temporizadorAparicion += dt;
    if (m_temporizadorAparicion >= m_intervaloAparicion) {
        m_temporizadorAparicion = 0.0;
        auto* nueva = new CarretaDeCanon(m_inicioPuente, m_finPuente);
        m_carretas.push_back(nueva);
        ++m_carretasGeneradas;
    }
}

void NivelRioTeatinos::actualizar(double dt) {
    if (dt <= 0.0) {
        return;
    }

    configurarCorrienteJugador();
    generarCarretaSiCorresponde(dt);

    // Actualiza tiempo, jugador, armas y objetos del escenario de la clase base
    actualizarComun(dt);

    // Actualiza cada carreta y revisa disparos o cruce del puente
    for (CarretaDeCanon* carreta : m_carretas) {
        if (carreta == nullptr) {
            continue;
        }
        const bool estabaActiva = carreta->estaActivo();
        carreta->actualizar(dt);

        if (carreta->cruzoElPuente() && carreta->estaActivo()) {
            m_algunaCruzoPuente = true;
        }

        Armas* bala = carreta->disparar();
        if (bala != nullptr) {
            agregarArma(bala);
        }

        if (estabaActiva && !carreta->estaActivo() && !carreta->cruzoElPuente()) {
            if (m_carretasDestruidas < m_carretasTotales) {
                ++m_carretasDestruidas;
            }
        }
    }
}

bool NivelRioTeatinos::victoria() const {
    return !derrota() && m_carretasDestruidas >= m_carretasTotales;
}

bool NivelRioTeatinos::derrota() const {
    return NivelDeBatalla::derrota() || m_algunaCruzoPuente;
}

const std::vector<CarretaDeCanon*>& NivelRioTeatinos::carretas() const {
    return m_carretas;
}

int NivelRioTeatinos::carretasTotales() const { return m_carretasTotales; }
void NivelRioTeatinos::setCarretasTotales(int total) {
    if (total <= 0) {
        throw std::invalid_argument("Error: el total de carretas debe ser mayor que cero.");
    }
    m_carretasTotales = total;
    m_carretas.reserve(static_cast<std::size_t>(m_carretasTotales));
}

int NivelRioTeatinos::carretasDestruidas() const { return m_carretasDestruidas; }
void NivelRioTeatinos::setCarretasDestruidas(int destruidas) {
    m_carretasDestruidas = std::max(0, destruidas);
}

int NivelRioTeatinos::carretasGeneradas() const { return m_carretasGeneradas; }
double NivelRioTeatinos::intervaloAparicion() const { return m_intervaloAparicion; }
void NivelRioTeatinos::setIntervaloAparicion(double intervalo) {
    if (intervalo <= 0.0) {
        throw std::invalid_argument("Error: el intervalo de aparicion debe ser mayor que cero.");
    }
    m_intervaloAparicion = intervalo;
}

QPointF NivelRioTeatinos::corrienteRio() const { return m_corrienteRio; }
void NivelRioTeatinos::setCorrienteRio(const QPointF& corriente) {
    m_corrienteRio = corriente;
    configurarCorrienteJugador();
}

QPointF NivelRioTeatinos::inicioPuente() const { return m_inicioPuente; }
void NivelRioTeatinos::setInicioPuente(const QPointF& inicio) { m_inicioPuente = inicio; }

QPointF NivelRioTeatinos::finPuente() const { return m_finPuente; }
void NivelRioTeatinos::setFinPuente(const QPointF& fin) { m_finPuente = fin; }

bool NivelRioTeatinos::algunaCruzoPuente() const { return m_algunaCruzoPuente; }