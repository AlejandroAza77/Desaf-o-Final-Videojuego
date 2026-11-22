#include "carretadecanon.h"
#include <algorithm>
#include <cmath>

void CarretaDeCanon::validarConfiguracion(double velocidadPuente,
                                          int golpesMaximos,
                                          double intervaloDisparo,
                                          double gravedadBala) {
    if (velocidadPuente < 0.0) {
        throw std::invalid_argument("Error: la velocidad en el puente no puede ser negativa.");
    }
    if (golpesMaximos <= 0) {
        throw std::invalid_argument("Error: los golpes maximos deben ser mayores que cero.");
    }
    if (intervaloDisparo <= 0.0) {
        throw std::invalid_argument("Error: el intervalo de disparo debe ser mayor que cero.");
    }
    if (gravedadBala <= 0.0) {
        throw std::invalid_argument("Error: la gravedad de la bala debe ser mayor que cero.");
    }
}

CarretaDeCanon::CarretaDeCanon(const QPointF& puntoInicio,
                               const QPointF& puntoFin,
                               double velocidadPuente,
                               int golpesMaximos,
                               double intervaloDisparo,
                               double danoPorGolpe,
                               double danoBala,
                               double gravedadBala,
                               const QPointF& tam)
    : ElementoDeBatalla(puntoInicio, QPointF(0.0, 0.0), tam, static_cast<double>(golpesMaximos)),
    m_puntoInicio(puntoInicio),
    m_puntoFin(puntoFin),
    m_velocidadPuente(velocidadPuente),
    m_golpesMaximos(golpesMaximos),
    m_golpesRecibidos(0),
    m_danoPorGolpe(danoPorGolpe > 0.0 ? danoPorGolpe : 1.0),
    m_intervaloDisparo(intervaloDisparo),
    m_temporizadorDisparo(0.0),
    m_danoBala(danoBala),
    m_gravedadBala(gravedadBala),
    m_cruzoPuente(false) {
    validarConfiguracion(m_velocidadPuente, m_golpesMaximos, m_intervaloDisparo, m_gravedadBala);
    if (m_danoBala < 0.0) {
        throw std::invalid_argument("Error: el dano de la bala no puede ser negativo.");
    }
    sincronizarIntegridad();
}

CarretaDeCanon& CarretaDeCanon::operator=(const CarretaDeCanon& otro) {
    if (this == &otro) {
        return *this;
    }
    ElementoDeBatalla::operator=(otro);
    m_puntoInicio = otro.m_puntoInicio;
    m_puntoFin = otro.m_puntoFin;
    m_velocidadPuente = otro.m_velocidadPuente;
    m_golpesMaximos = otro.m_golpesMaximos;
    m_golpesRecibidos = otro.m_golpesRecibidos;
    m_danoPorGolpe = otro.m_danoPorGolpe;
    m_intervaloDisparo = otro.m_intervaloDisparo;
    m_temporizadorDisparo = otro.m_temporizadorDisparo;
    m_danoBala = otro.m_danoBala;
    m_gravedadBala = otro.m_gravedadBala;
    m_cruzoPuente = otro.m_cruzoPuente;
    return *this;
}

bool CarretaDeCanon::operator==(const CarretaDeCanon& o) const {
    return ElementoDeBatalla::operator==(o) &&
           m_puntoInicio == o.m_puntoInicio &&
           m_puntoFin == o.m_puntoFin &&
           casiIgual(m_velocidadPuente, o.m_velocidadPuente) &&
           m_golpesMaximos == o.m_golpesMaximos &&
           m_golpesRecibidos == o.m_golpesRecibidos &&
           casiIgual(m_danoPorGolpe, o.m_danoPorGolpe) &&
           casiIgual(m_intervaloDisparo, o.m_intervaloDisparo) &&
           casiIgual(m_temporizadorDisparo, o.m_temporizadorDisparo) &&
           casiIgual(m_danoBala, o.m_danoBala) &&
           casiIgual(m_gravedadBala, o.m_gravedadBala) &&
           m_cruzoPuente == o.m_cruzoPuente;
}

void CarretaDeCanon::sincronizarIntegridad() {
    const double vidaRestante = std::max(0.0, static_cast<double>(m_golpesMaximos - m_golpesRecibidos));
    setIntegridad(vidaRestante);
    if (vidaRestante <= 0.0) {
        desactivar();
    }
}

void CarretaDeCanon::actualizar(double dt) {
    if (dt <= 0.0 || !estaActivo()) {
        return;
    }

    m_temporizadorDisparo += dt;

    // Avanza de m_puntoInicio hacia m_puntoFin a lo largo del puente
    const QPointF restante = m_puntoFin - m_posicion;
    const double distanciaRestante = std::hypot(restante.x(), restante.y());
    const double paso = m_velocidadPuente * dt;

    if (distanciaRestante <= 1e-6) {
        m_posicion = m_puntoFin;
        m_velocidad = QPointF(0.0, 0.0);
        m_cruzoPuente = true;
        return;
    }

    if (paso >= distanciaRestante) {
        m_posicion = m_puntoFin;
        m_velocidad = QPointF(0.0, 0.0);
        m_cruzoPuente = true;
    } else {
        const QPointF dir(restante.x() / distanciaRestante, restante.y() / distanciaRestante);
        m_velocidad = dir * m_velocidadPuente;
        m_posicion += m_velocidad * dt;
    }
}

QString CarretaDeCanon::rutaSprite() const {
    //asigne el sprite de la carreta de canon
    //Sprite de carreta avanzando por el puente (Nivel 1)
    return QString();
}

Armas* CarretaDeCanon::disparar() {
    if (!estaActivo() || m_temporizadorDisparo < m_intervaloDisparo) {
        return nullptr;
    }

    m_temporizadorDisparo = 0.0;

    // Disparo hacia la orilla inferior/patriota con componente vertical y horizontal moderada
    const QPointF velInicial(-60.0, 140.0);
    return new Armas(m_posicion,
                     velInicial,
                     m_danoBala,
                     1,
                     4.0,
                     m_gravedadBala,
                     0.4,
                     TipoProyectil::BalaDeCanon,
                     QPointF(24, 24),
                     ModoVista::Cenital);
}

bool CarretaDeCanon::cruzoElPuente() const {
    return m_cruzoPuente;
}

void CarretaDeCanon::registrarGolpe(double dano) {
    if (!estaActivo()) {
        return;
    }
    if (dano > 0.0) {
        ++m_golpesRecibidos;
        recibirImpacto(dano);
    } else {
        ++m_golpesRecibidos;
        sincronizarIntegridad();
    }
}

QPointF CarretaDeCanon::puntoInicio() const { return m_puntoInicio; }
void CarretaDeCanon::setPuntoInicio(const QPointF& inicio) { m_puntoInicio = inicio; }

QPointF CarretaDeCanon::puntoFin() const { return m_puntoFin; }
void CarretaDeCanon::setPuntoFin(const QPointF& fin) { m_puntoFin = fin; }

double CarretaDeCanon::velocidadPuente() const { return m_velocidadPuente; }
void CarretaDeCanon::setVelocidadPuente(double velocidad) {
    if (velocidad < 0.0) {
        throw std::invalid_argument("Error: la velocidad en el puente no puede ser negativa.");
    }
    m_velocidadPuente = velocidad;
}

int CarretaDeCanon::golpesMaximos() const { return m_golpesMaximos; }
void CarretaDeCanon::setGolpesMaximos(int golpes) {
    if (golpes <= 0) {
        throw std::invalid_argument("Error: los golpes maximos deben ser mayores que cero.");
    }
    m_golpesMaximos = golpes;
    sincronizarIntegridad();
}

int CarretaDeCanon::golpesRecibidos() const { return m_golpesRecibidos; }
void CarretaDeCanon::setGolpesRecibidos(int golpes) {
    m_golpesRecibidos = std::max(0, golpes);
    sincronizarIntegridad();
}

double CarretaDeCanon::intervaloDisparo() const { return m_intervaloDisparo; }
void CarretaDeCanon::setIntervaloDisparo(double intervalo) {
    if (intervalo <= 0.0) {
        throw std::invalid_argument("Error: el intervalo de disparo debe ser mayor que cero.");
    }
    m_intervaloDisparo = intervalo;
}

double CarretaDeCanon::temporizadorDisparo() const { return m_temporizadorDisparo; }
void CarretaDeCanon::setTemporizadorDisparo(double tiempo) {
    m_temporizadorDisparo = std::max(0.0, tiempo);
}

double CarretaDeCanon::danoPorGolpe() const { return m_danoPorGolpe; }
void CarretaDeCanon::setDanoPorGolpe(double dano) {
    if (dano <= 0.0) {
        throw std::invalid_argument("Error: el dano por golpe debe ser positivo.");
    }
    m_danoPorGolpe = dano;
}

double CarretaDeCanon::danoBala() const { return m_danoBala; }
void CarretaDeCanon::setDanoBala(double dano) {
    if (dano < 0.0) {
        throw std::invalid_argument("Error: el dano de la bala no puede ser negativo.");
    }
    m_danoBala = dano;
}

double CarretaDeCanon::gravedadBala() const { return m_gravedadBala; }
void CarretaDeCanon::setGravedadBala(double gravedad) {
    if (gravedad <= 0.0) {
        throw std::invalid_argument("Error: la gravedad de la bala debe ser mayor que cero.");
    }
    m_gravedadBala = gravedad;
}

void CarretaDeCanon::setCruzoPuente(bool cruzo) { m_cruzoPuente = cruzo; }