#include "soldadocentinela.h"
#include <algorithm>
#include <cmath>

namespace {
QPointF normalizarDir(const QPointF& v) {
    const double lenSq = v.x() * v.x() + v.y() * v.y();
    if (lenSq <= 1e-12) {
        return QPointF(0.0, 0.0);
    }
    const double invLen = 1.0 / std::sqrt(lenSq);
    return QPointF(v.x() * invLen, v.y() * invLen);
}
}

SoldadoCentinela::SoldadoCentinela(const QPointF& pos,
                                   double vida,
                                   RolCentinela rol,
                                   double velocidadMovimiento,
                                   double cooldownDisparo,
                                   double danoProyectil,
                                   double gravedadProyectil,
                                   const QPointF& tam,
                                   AgenteInteligente* cerebroExterno,
                                   bool tomarPropiedadCerebro)
    : ElementoDeBatalla(pos, QPointF(0.0, 0.0), tam, vida),
    m_cerebro(nullptr),
    m_duenoCerebro(false),
    m_rol(rol),
    m_velocidadMovimiento(velocidadMovimiento),
    m_cooldownDisparo(cooldownDisparo),
    m_temporizadorDisparo(0.0),
    m_retardoAcumulado(0.0),
    m_danoProyectil(danoProyectil),
    m_gravedadProyectil(gravedadProyectil),
    m_objetivo(pos),
    m_tieneObjetivo(false),
    m_saludMaxima(vida > 0.0 ? vida : 100.0) {
    if (vida <= 0.0) {
        throw std::invalid_argument("Error: la vida inicial del centinela debe ser mayor que cero.");
    }
    if (m_velocidadMovimiento < 0.0) {
        throw std::invalid_argument("Error: la velocidad de movimiento no puede ser negativa.");
    }
    if (m_cooldownDisparo <= 0.0) {
        throw std::invalid_argument("Error: el cooldown de disparo debe ser mayor que cero.");
    }
    if (m_danoProyectil < 0.0) {
        throw std::invalid_argument("Error: el dano del proyectil no puede ser negativo.");
    }
    if (m_gravedadProyectil <= 0.0) {
        throw std::invalid_argument("Error: la gravedad del proyectil debe ser mayor que cero.");
    }

    if (cerebroExterno != nullptr) {
        m_cerebro = cerebroExterno;
        m_duenoCerebro = tomarPropiedadCerebro;
    } else {
        m_cerebro = new AgenteInteligente();
        m_duenoCerebro = true;
    }
}

void SoldadoCentinela::liberarCerebro() {
    if (m_duenoCerebro && m_cerebro != nullptr) {
        delete m_cerebro;
    }
    m_cerebro = nullptr;
    m_duenoCerebro = false;
}

void SoldadoCentinela::copiarDesde(const SoldadoCentinela& otro) {
    m_rol = otro.m_rol;
    m_velocidadMovimiento = otro.m_velocidadMovimiento;
    m_cooldownDisparo = otro.m_cooldownDisparo;
    m_temporizadorDisparo = otro.m_temporizadorDisparo;
    m_retardoAcumulado = otro.m_retardoAcumulado;
    m_danoProyectil = otro.m_danoProyectil;
    m_gravedadProyectil = otro.m_gravedadProyectil;
    m_objetivo = otro.m_objetivo;
    m_tieneObjetivo = otro.m_tieneObjetivo;
    m_saludMaxima = otro.m_saludMaxima;

    if (otro.m_cerebro != nullptr) {
        m_cerebro = new AgenteInteligente(*(otro.m_cerebro));
        m_duenoCerebro = true;
    } else {
        m_cerebro = nullptr;
        m_duenoCerebro = false;
    }
}

SoldadoCentinela::SoldadoCentinela(const SoldadoCentinela& otro)
    : ElementoDeBatalla(otro),
    m_cerebro(nullptr),
    m_duenoCerebro(false) {
    copiarDesde(otro);
}

SoldadoCentinela::~SoldadoCentinela() {
    liberarCerebro();
}

SoldadoCentinela& SoldadoCentinela::operator=(const SoldadoCentinela& otro) {
    if (this == &otro) {
        return *this;
    }
    ElementoDeBatalla::operator=(otro);
    liberarCerebro();
    copiarDesde(otro);
    return *this;
}

bool SoldadoCentinela::operator==(const SoldadoCentinela& o) const {
    if (!ElementoDeBatalla::operator==(o)) {
        return false;
    }
    const bool mismoCerebro = (m_cerebro == nullptr && o.m_cerebro == nullptr) ||
                              (m_cerebro != nullptr && o.m_cerebro != nullptr &&
                               m_cerebro->radioVision() == o.m_cerebro->radioVision() &&
                               m_cerebro->retardoReaccion() == o.m_cerebro->retardoReaccion() &&
                               m_cerebro->umbralAprendizaje() == o.m_cerebro->umbralAprendizaje() &&
                               m_cerebro->coberturaActiva() == o.m_cerebro->coberturaActiva());

    return mismoCerebro &&
           m_rol == o.m_rol &&
           casiIgual(m_velocidadMovimiento, o.m_velocidadMovimiento) &&
           casiIgual(m_cooldownDisparo, o.m_cooldownDisparo) &&
           casiIgual(m_temporizadorDisparo, o.m_temporizadorDisparo) &&
           casiIgual(m_retardoAcumulado, o.m_retardoAcumulado) &&
           casiIgual(m_danoProyectil, o.m_danoProyectil) &&
           casiIgual(m_gravedadProyectil, o.m_gravedadProyectil) &&
           m_objetivo == o.m_objetivo &&
           m_tieneObjetivo == o.m_tieneObjetivo &&
           casiIgual(m_saludMaxima, o.m_saludMaxima);
}

void SoldadoCentinela::setObjetivo(const QPointF& posObjetivo) {
    m_objetivo = posObjetivo;
    m_tieneObjetivo = true;
}

void SoldadoCentinela::actualizar(double dt) {
    if (dt <= 0.0 || !estaActivo()) {
        return;
    }

    m_temporizadorDisparo += dt;

    if (m_cerebro == nullptr) {
        m_velocidad = QPointF(0.0, 0.0);
        return;
    }

    const QPointF posReferencia = m_tieneObjetivo ? m_objetivo : m_posicion;
    const double saludRel = (m_saludMaxima > 0.0) ? (m_integridad / m_saludMaxima) : 0.0;

    // 1. Percibir y razonar con el agente inteligente
    const Estimulo est = m_cerebro->percibir(m_posicion, posReferencia, saludRel);
    const EstadoAgente estado = m_cerebro->razonar(est);

    // 2. Respetar retardo de reaccion del agente antes de aplicar nueva direccion
    m_retardoAcumulado += dt;
    if (m_retardoAcumulado >= m_cerebro->retardoReaccion()) {
        m_retardoAcumulado = 0.0;
        const QPointF dir = normalizarDir(m_cerebro->actuar(estado, est));

        double factorRol = 1.0;
        if (m_rol == RolCentinela::Comandante) {
            factorRol = 1.15;
        } else if (m_rol == RolCentinela::EnemigoFinal) {
            factorRol = 1.25;
        }

        m_velocidad = dir * (m_velocidadMovimiento * factorRol);
    }

    m_posicion += m_velocidad * dt;
}

QString SoldadoCentinela::rutaSprite() const {
    switch (m_rol) {
    case RolCentinela::Centinela:
        // Sprite de soldado centinela (Nivel 2)
        return QString();
    case RolCentinela::Comandante:
        // Sprite de comandante realista (Nivel 2)
        return QString();
    case RolCentinela::EnemigoFinal:
        // Sprite de enemigo final Abelardo (Nivel 2)
        return QString();
    }
    // Sprite por defecto si no coincide ningun rol
    return QString();
}

void SoldadoCentinela::recibirImpacto(double dano) {
    ElementoDeBatalla::recibirImpacto(dano);
    if (m_cerebro != nullptr && m_tieneObjetivo) {
        m_cerebro->aprender(m_posicion, m_objetivo);
    }
}

void SoldadoCentinela::registrarImpactoRecibido(const QPointF& origenImpacto, double dano) {
    ElementoDeBatalla::recibirImpacto(dano);
    if (m_cerebro != nullptr) {
        m_cerebro->aprender(m_posicion, origenImpacto);
    }
}

Armas* SoldadoCentinela::disparar() {
    if (!estaActivo() || m_temporizadorDisparo < m_cooldownDisparo) {
        return nullptr;
    }

    m_temporizadorDisparo = 0.0;

    QPointF dirDisparo(-1.0, -0.35);
    if (m_tieneObjetivo) {
        const QPointF delta = m_objetivo - m_posicion;
        const QPointF n = normalizarDir(delta);
        if (n.x() != 0.0 || n.y() != 0.0) {
            dirDisparo = QPointF(n.x() * 340.0, n.y() * 340.0 - 110.0);
        }
    } else {
        dirDisparo = QPointF(-300.0, -110.0);
    }

    const double danoEfectivo = (m_rol == RolCentinela::EnemigoFinal)
                                    ? m_danoProyectil * 1.25
                                    : m_danoProyectil;

    return new Armas(m_posicion,
                     dirDisparo,
                     danoEfectivo,
                     1,
                     3.5,
                     m_gravedadProyectil,
                     0.45,
                     TipoProyectil::Piedra,
                     QPointF(22.0, 22.0),
                     ModoVista::Lateral);
}

AgenteInteligente* SoldadoCentinela::cerebro() const { return m_cerebro; }
void SoldadoCentinela::setCerebro(AgenteInteligente* nuevoCerebro, bool tomarPropiedad) {
    if (nuevoCerebro == m_cerebro) {
        m_duenoCerebro = tomarPropiedad;
        return;
    }
    liberarCerebro();
    m_cerebro = nuevoCerebro;
    m_duenoCerebro = tomarPropiedad;
}

RolCentinela SoldadoCentinela::rol() const { return m_rol; }
void SoldadoCentinela::setRol(RolCentinela rol) { m_rol = rol; }

double SoldadoCentinela::velocidadMovimiento() const { return m_velocidadMovimiento; }
void SoldadoCentinela::setVelocidadMovimiento(double velocidad) {
    if (velocidad < 0.0) {
        throw std::invalid_argument("Error: la velocidad de movimiento no puede ser negativa.");
    }
    m_velocidadMovimiento = velocidad;
}

double SoldadoCentinela::cooldownDisparo() const { return m_cooldownDisparo; }
void SoldadoCentinela::setCooldownDisparo(double cooldown) {
    if (cooldown <= 0.0) {
        throw std::invalid_argument("Error: el cooldown de disparo debe ser mayor que cero.");
    }
    m_cooldownDisparo = cooldown;
}

double SoldadoCentinela::temporizadorDisparo() const { return m_temporizadorDisparo; }
void SoldadoCentinela::setTemporizadorDisparo(double tiempo) {
    m_temporizadorDisparo = std::max(0.0, tiempo);
}

double SoldadoCentinela::danoProyectil() const { return m_danoProyectil; }
void SoldadoCentinela::setDanoProyectil(double dano) {
    if (dano < 0.0) {
        throw std::invalid_argument("Error: el dano del proyectil no puede ser negativo.");
    }
    m_danoProyectil = dano;
}

double SoldadoCentinela::gravedadProyectil() const { return m_gravedadProyectil; }
void SoldadoCentinela::setGravedadProyectil(double gravedad) {
    if (gravedad <= 0.0) {
        throw std::invalid_argument("Error: la gravedad del proyectil debe ser mayor que cero.");
    }
    m_gravedadProyectil = gravedad;
}

QPointF SoldadoCentinela::objetivo() const { return m_objetivo; }
bool SoldadoCentinela::tieneObjetivo() const { return m_tieneObjetivo; }
void SoldadoCentinela::setTieneObjetivo(bool tiene) { m_tieneObjetivo = tiene; }

double SoldadoCentinela::saludMaxima() const { return m_saludMaxima; }
void SoldadoCentinela::setSaludMaxima(double vidaMax) {
    if (vidaMax <= 0.0) {
        throw std::invalid_argument("Error: la salud maxima debe ser mayor que cero.");
    }
    m_saludMaxima = vidaMax;
}