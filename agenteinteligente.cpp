#include "agenteinteligente.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr int kNumSectoresAngulo = 8;           // 8 sectores de 45 grados
constexpr double kUmbralSaludRetirada = 0.20;   // 20 % de salud
constexpr double kFactorMejoraReaccion = 0.85;  // 15 % menos retardo = 15 % mas rapido
constexpr double kRetardoMinimo = 0.05;
}

void AgenteInteligente::validarParametros(double radioVision, double retardoReaccion, int umbralAprendizaje) {
    if (radioVision <= 0.0) throw std::invalid_argument("Error: radioVision debe ser mayor que cero.");
    if (retardoReaccion <= 0.0) throw std::invalid_argument("Error: retardoReaccion debe ser mayor que cero.");
    if (umbralAprendizaje <= 0) throw std::invalid_argument("Error: umbralAprendizaje debe ser mayor que cero.");
}

AgenteInteligente::AgenteInteligente(double radioVision, double retardoReaccion, int umbralAprendizaje,
                                     const QPointF& puntoA, const QPointF& puntoB, double radioAudicion)
    : m_radioVision(radioVision), m_radioAudicion(radioAudicion), m_retardoReaccion(retardoReaccion),
      m_umbralAprendizaje(umbralAprendizaje), m_puntoA(puntoA), m_puntoB(puntoB), m_tiempoReaccion(0.0),
      m_coberturaActiva(false), m_ultimoSectorCobertura(-1), m_origenUltimoImpacto(0.0, 0.0),
      m_direccionPatrulla(normalizado(puntoB - puntoA)) {
    validarParametros(m_radioVision, m_retardoReaccion, m_umbralAprendizaje);
    if (m_radioAudicion < 0.0) throw std::invalid_argument("Error: radioAudicion no puede ser negativo.");
    m_memoriaImpactos.reserve(kNumSectoresAngulo * 3);
}

AgenteInteligente& AgenteInteligente::operator=(const AgenteInteligente& otro) {
    if (this == &otro) return *this;
    m_radioVision = otro.m_radioVision;
    m_radioAudicion = otro.m_radioAudicion;
    m_retardoReaccion = otro.m_retardoReaccion;
    m_umbralAprendizaje = otro.m_umbralAprendizaje;
    m_puntoA = otro.m_puntoA;
    m_puntoB = otro.m_puntoB;
    m_tiempoReaccion = otro.m_tiempoReaccion;
    m_coberturaActiva = otro.m_coberturaActiva;
    m_ultimoSectorCobertura = otro.m_ultimoSectorCobertura;
    m_origenUltimoImpacto = otro.m_origenUltimoImpacto;
    m_direccionPatrulla = otro.m_direccionPatrulla;
    m_memoriaImpactos = otro.m_memoriaImpactos;
    return *this;
}

bool AgenteInteligente::operator==(const AgenteInteligente& o) const {
    return casiIgual(m_radioVision, o.m_radioVision) && casiIgual(m_radioAudicion, o.m_radioAudicion) &&
           casiIgual(m_retardoReaccion, o.m_retardoReaccion) && m_umbralAprendizaje == o.m_umbralAprendizaje &&
           m_puntoA == o.m_puntoA && m_puntoB == o.m_puntoB && casiIgual(m_tiempoReaccion, o.m_tiempoReaccion) &&
           m_coberturaActiva == o.m_coberturaActiva && m_ultimoSectorCobertura == o.m_ultimoSectorCobertura &&
           m_origenUltimoImpacto == o.m_origenUltimoImpacto && m_direccionPatrulla == o.m_direccionPatrulla &&
           m_memoriaImpactos == o.m_memoriaImpactos;
}

Estimulo AgenteInteligente::percibir(const QPointF& posAgente, const QPointF& posJugador,
                                     double saludRelativa) const {
    const double dx = posJugador.x() - posAgente.x();
    const double dy = posJugador.y() - posAgente.y();
    Estimulo est;
    est.distanciaCuadrada = dx * dx + dy * dy;
    est.oyeJugador = est.distanciaCuadrada <= m_radioAudicion * m_radioAudicion;
    est.saludRelativa = std::max(0.0, std::min(1.0, saludRelativa));
    est.posAgente = posAgente;
    est.posJugador = posJugador;
    return est;
}

EstadoAgente AgenteInteligente::razonar(const Estimulo& estimulo) const {
    if (estimulo.saludRelativa < kUmbralSaludRetirada) return EstadoAgente::Retirada;
    // d^2 contra R^2: sin raiz cuadrada
    if (estimulo.distanciaCuadrada <= m_radioVision * m_radioVision) return EstadoAgente::Ataque;
    return EstadoAgente::Patrulla;
}

QPointF AgenteInteligente::actuar(EstadoAgente estado, const Estimulo& estimulo) {
    const QPointF haciaJugador = estimulo.posJugador - estimulo.posAgente;

    switch (estado) {
    case EstadoAgente::Ataque: {
        const QPointF dir = normalizado(haciaJugador);
        if (m_coberturaActiva) {  // tras aprender, se acerca de forma lateral para cubrirse
            const QPointF lateral(-dir.y(), dir.x());
            return normalizado(dir * 0.5 + lateral * 0.866);
        }
        return dir;
    }
    case EstadoAgente::Retirada:
        return normalizado(QPointF(-haciaJugador.x(), -haciaJugador.y()));
    case EstadoAgente::Patrulla:
    default: {
        // Va de A a B y vuelve: al llegar a un extremo invierte el sentido.
        const QPointF ab = m_puntoB - m_puntoA;
        const double largo2 = ab.x() * ab.x() + ab.y() * ab.y();
        if (largo2 > 1e-12) {
            const QPointF ap = estimulo.posAgente - m_puntoA;
            const double t = (ap.x() * ab.x() + ap.y() * ab.y()) / largo2;
            if (t >= 1.0) m_direccionPatrulla = normalizado(QPointF(-ab.x(), -ab.y()));
            else if (t <= 0.0) m_direccionPatrulla = normalizado(ab);
        }
        if (estimulo.oyeJugador)  // si lo oye pero no lo ve, gira un poco hacia el sonido
            return normalizado(m_direccionPatrulla * 0.6 + normalizado(haciaJugador) * 0.4);
        return m_direccionPatrulla;
    }
    }
}

int AgenteInteligente::calcularSectorAngulo(const QPointF& posAgente, const QPointF& origenImpacto) const {
    double angulo = std::atan2(origenImpacto.y() - posAgente.y(), origenImpacto.x() - posAgente.x());
    if (angulo < 0.0) angulo += 2.0 * kPi;
    const int sector = static_cast<int>(angulo / ((2.0 * kPi) / kNumSectoresAngulo));
    return std::min(sector, kNumSectoresAngulo - 1);
}

int AgenteInteligente::calcularBandaDistancia(double distanciaSq) const {
    const double r1 = m_radioVision * 0.35, r2 = m_radioVision * 0.75;
    if (distanciaSq <= r1 * r1) return 0;  // cercana
    if (distanciaSq <= r2 * r2) return 1;  // media
    return 2;                              // lejana
}

void AgenteInteligente::activarCobertura(int sector, const QPointF& origenImpacto) {
    m_coberturaActiva = true;
    m_ultimoSectorCobertura = sector;
    m_origenUltimoImpacto = origenImpacto;
    m_retardoReaccion = std::max(kRetardoMinimo, m_retardoReaccion * kFactorMejoraReaccion);
}

bool AgenteInteligente::aprender(const QPointF& posAgente, const QPointF& origenImpacto) {
    const double dx = origenImpacto.x() - posAgente.x();
    const double dy = origenImpacto.y() - posAgente.y();
    const int sector = calcularSectorAngulo(posAgente, origenImpacto);
    const int banda = calcularBandaDistancia(dx * dx + dy * dy);

    for (RegistroImpacto& reg : m_memoriaImpactos) {
        if (reg.sectorAngulo != sector || reg.bandaDistancia != banda) continue;
        ++reg.conteoImpactos;
        if (reg.conteoImpactos == m_umbralAprendizaje) {  // solo la primera vez: la mejora no se acumula sin fin
            activarCobertura(sector, origenImpacto);
            return true;
        }
        return false;
    }
    RegistroImpacto nuevo;
    nuevo.sectorAngulo = sector;
    nuevo.bandaDistancia = banda;
    nuevo.conteoImpactos = 1;
    m_memoriaImpactos.push_back(nuevo);
    if (m_umbralAprendizaje == 1) {
        activarCobertura(sector, origenImpacto);
        return true;
    }
    return false;
}

bool AgenteInteligente::actualizar(double dt) {
    if (dt <= 0.0) return false;
    m_tiempoReaccion += dt;
    if (m_tiempoReaccion < m_retardoReaccion) return false;
    m_tiempoReaccion = 0.0;
    return true;
}

QPointF AgenteInteligente::coberturaSugerida(const QPointF& posActual) const {
    if (!m_coberturaActiva) return posActual;
    return distancia(m_puntoA, m_origenUltimoImpacto) >= distancia(m_puntoB, m_origenUltimoImpacto) ? m_puntoA
                                                                                                   : m_puntoB;
}

double AgenteInteligente::radioVision() const { return m_radioVision; }
void AgenteInteligente::setRadioVision(double radio) {
    if (radio <= 0.0) throw std::invalid_argument("Error: radioVision debe ser mayor que cero.");
    m_radioVision = radio;
}
double AgenteInteligente::radioAudicion() const { return m_radioAudicion; }
void AgenteInteligente::setRadioAudicion(double radio) {
    if (radio < 0.0) throw std::invalid_argument("Error: radioAudicion no puede ser negativo.");
    m_radioAudicion = radio;
}
double AgenteInteligente::retardoReaccion() const { return m_retardoReaccion; }
void AgenteInteligente::setRetardoReaccion(double retardo) {
    if (retardo <= 0.0) throw std::invalid_argument("Error: retardoReaccion debe ser mayor que cero.");
    m_retardoReaccion = retardo;
}
int AgenteInteligente::umbralAprendizaje() const { return m_umbralAprendizaje; }
void AgenteInteligente::setUmbralAprendizaje(int umbral) {
    if (umbral <= 0) throw std::invalid_argument("Error: umbralAprendizaje debe ser mayor que cero.");
    m_umbralAprendizaje = umbral;
}
QPointF AgenteInteligente::puntoA() const { return m_puntoA; }
QPointF AgenteInteligente::puntoB() const { return m_puntoB; }
void AgenteInteligente::setPuntosPatrulla(const QPointF& a, const QPointF& b) {
    m_puntoA = a;
    m_puntoB = b;
    m_direccionPatrulla = normalizado(b - a);
}
bool AgenteInteligente::coberturaActiva() const { return m_coberturaActiva; }
void AgenteInteligente::setCoberturaActiva(bool activa) { m_coberturaActiva = activa; }
int AgenteInteligente::ultimoSectorCobertura() const { return m_ultimoSectorCobertura; }
QPointF AgenteInteligente::direccionPatrulla() const { return m_direccionPatrulla; }
void AgenteInteligente::setDireccionPatrulla(const QPointF& dir) { m_direccionPatrulla = dir; }
const std::vector<RegistroImpacto>& AgenteInteligente::memoriaImpactos() const { return m_memoriaImpactos; }
void AgenteInteligente::limpiarMemoria() {
    m_memoriaImpactos.clear();
    m_coberturaActiva = false;
    m_ultimoSectorCobertura = -1;
}
