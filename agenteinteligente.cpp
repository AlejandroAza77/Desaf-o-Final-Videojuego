#include "agenteinteligente.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr int kNumSectoresAngulo = 8; // 8 sectores de 45 grados
constexpr double kUmbralSaludRetirada = 0.20; // 20% de salud
constexpr double kFactorMejoraReaccion = 0.85; // 15% menos retardo = 15% mas velocidad de reaccion

QPointF normalizarVector(const QPointF& v) {
    const double normaSq = v.x() * v.x() + v.y() * v.y();
    if (normaSq <= 1e-12) {
        return QPointF(0.0, 0.0);
    }
    const double invNorma = 1.0 / std::sqrt(normaSq);
    return QPointF(v.x() * invNorma, v.y() * invNorma);
}
}

void AgenteInteligente::validarParametros(double radioVision,
                                          double retardoReaccion,
                                          int umbralAprendizaje) {
    if (radioVision <= 0.0) {
        throw std::invalid_argument("Error: radioVision debe ser mayor que cero.");
    }
    if (retardoReaccion <= 0.0) {
        throw std::invalid_argument("Error: retardoReaccion debe ser mayor que cero.");
    }
    if (umbralAprendizaje <= 0) {
        throw std::invalid_argument("Error: umbralAprendizaje debe ser mayor que cero.");
    }
}

AgenteInteligente::AgenteInteligente(double radioVision,
                                     double retardoReaccion,
                                     int umbralAprendizaje,
                                     double radioAudicion)
    : m_radioVision(radioVision),
    m_radioAudicion(radioAudicion),
    m_retardoReaccion(retardoReaccion),
    m_umbralAprendizaje(umbralAprendizaje),
    m_coberturaActiva(false),
    m_ultimoSectorCobertura(-1),
    m_direccionPatrulla(1.0, 0.0) {
    validarParametros(m_radioVision, m_retardoReaccion, m_umbralAprendizaje);
    if (m_radioAudicion < 0.0) {
        throw std::invalid_argument("Error: radioAudicion no puede ser negativo.");
    }
    m_memoriaImpactos.reserve(kNumSectoresAngulo * 3);
}

AgenteInteligente& AgenteInteligente::operator=(const AgenteInteligente& otro) {
    if (this == &otro) {
        return *this;
    }
    m_radioVision = otro.m_radioVision;
    m_radioAudicion = otro.m_radioAudicion;
    m_retardoReaccion = otro.m_retardoReaccion;
    m_umbralAprendizaje = otro.m_umbralAprendizaje;
    m_coberturaActiva = otro.m_coberturaActiva;
    m_ultimoSectorCobertura = otro.m_ultimoSectorCobertura;
    m_direccionPatrulla = otro.m_direccionPatrulla;
    m_memoriaImpactos = otro.m_memoriaImpactos;
    return *this;
}

Estimulo AgenteInteligente::percibir(const QPointF& posAgente,
                                     const QPointF& posJugador,
                                     double saludRelativa) const {
    const double dx = posJugador.x() - posAgente.x();
    const double dy = posJugador.y() - posAgente.y();
    const double distSq = dx * dx + dy * dy;
    const double radioAuditivoSq = m_radioAudicion * m_radioAudicion;

    Estimulo est;
    est.distanciaCuadrada = distSq;
    est.oyeJugador = (distSq <= radioAuditivoSq);
    est.saludRelativa = std::max(0.0, std::min(1.0, saludRelativa));
    est.posAgente = posAgente;
    est.posJugador = posJugador;
    return est;
}

EstadoAgente AgenteInteligente::razonar(const Estimulo& estimulo) const {
    // Prioridad 1: si la salud es menor al 20%, se retira
    if (estimulo.saludRelativa < kUmbralSaludRetirada) {
        return EstadoAgente::Retirada;
    }

    // Comparacion directa de d^2 con R^2 sin sacar raiz cuadrada
    const double radioSq = m_radioVision * m_radioVision;
    if (estimulo.distanciaCuadrada <= radioSq) {
        return EstadoAgente::Ataque;
    }

    return EstadoAgente::Patrulla;
}

QPointF AgenteInteligente::actuar(EstadoAgente estado, const Estimulo& estimulo) const {
    const QPointF haciaJugador = estimulo.posJugador - estimulo.posAgente;

    switch (estado) {
    case EstadoAgente::Ataque: {
        // Si tiene cobertura activa tras aprender, busca desplazamiento lateral leve para cubrirse
        if (m_coberturaActiva) {
            const QPointF dir = normalizarVector(haciaJugador);
            const QPointF lateral(-dir.y(), dir.x());
            return normalizarVector(dir * 0.5 + lateral * 0.866);
        }
        return normalizarVector(haciaJugador);
    }
    case EstadoAgente::Retirada: {
        // Avanza en sentido opuesto al jugador
        return normalizarVector(QPointF(-haciaJugador.x(), -haciaJugador.y()));
    }
    case EstadoAgente::Patrulla:
    default: {
        // Si oye al jugador pero esta fuera de vision, gira ligeramente hacia el sonido
        if (estimulo.oyeJugador) {
            const QPointF dirSonido = normalizarVector(haciaJugador);
            return normalizarVector(normalizarVector(m_direccionPatrulla) * 0.6 + dirSonido * 0.4);
        }
        return normalizarVector(m_direccionPatrulla);
    }
    }
}

int AgenteInteligente::calcularSectorAngulo(const QPointF& posAgente,
                                            const QPointF& origenImpacto) const {
    const double dx = origenImpacto.x() - posAgente.x();
    const double dy = origenImpacto.y() - posAgente.y();
    double angulo = std::atan2(dy, dx); // Rango [-pi, pi]
    if (angulo < 0.0) {
        angulo += 2.0 * kPi; // Rango [0, 2*pi)
    }
    const double tamSector = (2.0 * kPi) / static_cast<double>(kNumSectoresAngulo);
    int sector = static_cast<int>(angulo / tamSector);
    if (sector >= kNumSectoresAngulo) {
        sector = kNumSectoresAngulo - 1;
    }
    return sector;
}

int AgenteInteligente::calcularBandaDistancia(double distanciaSq) const {
    const double r1Sq = (m_radioVision * 0.35) * (m_radioVision * 0.35);
    const double r2Sq = (m_radioVision * 0.75) * (m_radioVision * 0.75);
    if (distanciaSq <= r1Sq) {
        return 0; // Banda cercana
    }
    if (distanciaSq <= r2Sq) {
        return 1; // Banda media
    }
    return 2; // Banda lejana
}

bool AgenteInteligente::aprender(const QPointF& posAgente, const QPointF& origenImpacto) {
    const double dx = origenImpacto.x() - posAgente.x();
    const double dy = origenImpacto.y() - posAgente.y();
    const double distSq = dx * dx + dy * dy;

    const int sector = calcularSectorAngulo(posAgente, origenImpacto);
    const int banda = calcularBandaDistancia(distSq);

    for (RegistroImpacto& reg : m_memoriaImpactos) {
        if (reg.sectorAngulo == sector && reg.bandaDistancia == banda) {
            ++reg.conteoImpactos;
            if (reg.conteoImpactos >= m_umbralAprendizaje) {
                m_coberturaActiva = true;
                m_ultimoSectorCobertura = sector;
                // Gana 15% de velocidad de reaccion (reduce el retardo en un 15%)
                m_retardoReaccion = std::max(0.05, m_retardoReaccion * kFactorMejoraReaccion);
                return true;
            }
            return false;
        }
    }

    RegistroImpacto nuevo;
    nuevo.sectorAngulo = sector;
    nuevo.bandaDistancia = banda;
    nuevo.conteoImpactos = 1;
    m_memoriaImpactos.push_back(nuevo);

    if (nuevo.conteoImpactos >= m_umbralAprendizaje) {
        m_coberturaActiva = true;
        m_ultimoSectorCobertura = sector;
        m_retardoReaccion = std::max(0.05, m_retardoReaccion * kFactorMejoraReaccion);
        return true;
    }
    return false;
}

double AgenteInteligente::radioVision() const { return m_radioVision; }
void AgenteInteligente::setRadioVision(double radio) {
    if (radio <= 0.0) {
        throw std::invalid_argument("Error: radioVision debe ser mayor que cero.");
    }
    m_radioVision = radio;
}

double AgenteInteligente::radioAudicion() const { return m_radioAudicion; }
void AgenteInteligente::setRadioAudicion(double radio) {
    if (radio < 0.0) {
        throw std::invalid_argument("Error: radioAudicion no puede ser negativo.");
    }
    m_radioAudicion = radio;
}

double AgenteInteligente::retardoReaccion() const { return m_retardoReaccion; }
void AgenteInteligente::setRetardoReaccion(double retardo) {
    if (retardo <= 0.0) {
        throw std::invalid_argument("Error: retardoReaccion debe ser mayor que cero.");
    }
    m_retardoReaccion = retardo;
}

int AgenteInteligente::umbralAprendizaje() const { return m_umbralAprendizaje; }
void AgenteInteligente::setUmbralAprendizaje(int umbral) {
    if (umbral <= 0) {
        throw std::invalid_argument("Error: umbralAprendizaje debe ser mayor que cero.");
    }
    m_umbralAprendizaje = umbral;
}

bool AgenteInteligente::coberturaActiva() const { return m_coberturaActiva; }
void AgenteInteligente::setCoberturaActiva(bool activa) { m_coberturaActiva = activa; }

int AgenteInteligente::ultimoSectorCobertura() const { return m_ultimoSectorCobertura; }

QPointF AgenteInteligente::direccionPatrulla() const { return m_direccionPatrulla; }
void AgenteInteligente::setDireccionPatrulla(const QPointF& dir) { m_direccionPatrulla = dir; }

const std::vector<RegistroImpacto>& AgenteInteligente::memoriaImpactos() const {
    return m_memoriaImpactos;
}

void AgenteInteligente::limpiarMemoria() {
    m_memoriaImpactos.clear();
    m_coberturaActiva = false;
    m_ultimoSectorCobertura = -1;
}