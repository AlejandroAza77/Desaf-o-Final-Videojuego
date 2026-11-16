#include "objetodelescenario.h"
#include <cmath>

ObjetoDelEscenario::ObjetoDelEscenario(QPointF posBase, TipoObjeto tipo, double amplitud, double frecuencia, double fase, double longitud, QPointF tam)
    : ElementoDeBatalla(posBase, QPointF(0, 0), tam, 100.0),
    m_tipoObjeto(tipo),
    m_amplitud(amplitud),
    m_frecuencia(frecuencia),
    m_fase(fase),
    m_longitudCuerda(longitud),
    m_posicionBase(posBase),
    m_tiempoAcumulado(0.0) {

    // Validacion de parametros fisicos requeridos: A >= 0, omega > 0 y L > 0
    if (m_amplitud < 0.0) {
        throw std::invalid_argument("Error: La amplitud A debe ser mayor o igual a cero.");
    }
    if (m_frecuencia <= 0.0) {
        throw std::invalid_argument("Error: La frecuencia angular omega debe ser mayor que cero.");
    }
    if (m_longitudCuerda <= 0.0) {
        throw std::invalid_argument("Error: La longitud L debe ser mayor que cero.");
    }

   if (m_tipoObjeto == TipoObjeto::PastelDeYuca) {
        m_recogible = true;
    } else {
        m_recogible = false;
    }
}

void ObjetoDelEscenario::actualizar(double dt) {
    if (!estaActivo()) return;

    m_tiempoAcumulado += dt;

    switch (m_tipoObjeto) {
    case TipoObjeto::TroncoFlotante: {
        // Movimiento Armonico Simple (MAS) horizontal: x = cx + A * cos(omega * t + phi)
        double nuevoX = m_posicionBase.x() + m_amplitud * std::cos(m_frecuencia * m_tiempoAcumulado + m_fase);
        m_posicion.setX(nuevoX);
        break;
    }
    case TipoObjeto::TrampaPendulo: {
        // Movimiento pendular: theta(t) = theta_max * cos(omega * t), posicion = pivote + L * (sin theta, cos theta)
        double thetaMax = m_amplitud; // amplitud angulo maximo theta_max
        double theta = thetaMax * std::cos(m_frecuencia * m_tiempoAcumulado);
        double nuevoX = m_posicionBase.x() + m_longitudCuerda * std::sin(theta);
        double nuevoY = m_posicionBase.y() + m_longitudCuerda * std::cos(theta);
        m_posicion = QPointF(nuevoX, nuevoY);
        break;
    }
    case TipoObjeto::PlataformaMovil: {
        // Oscilacion lineal entre dos puntos usando funcion senoidal suave
        double desplazamiento = m_amplitud * std::sin(m_frecuencia * m_tiempoAcumulado);
        m_posicion.setX(m_posicionBase.x() + desplazamiento);
        break;
    }
    case TipoObjeto::BarrilExplosivo:
    case TipoObjeto::PastelDeYuca:
        // Objetos estaticos, no modifican su posicion con el tiempo
        break;
    }
}

QString ObjetoDelEscenario::rutaSprite() const {
    switch (m_tipoObjeto) {
    case TipoObjeto::TroncoFlotante:
        return ":/sprites/nivel1/tronco.png";
    case TipoObjeto::TrampaPendulo:
        return ":/sprites/nivel2/boledora.png";
    case TipoObjeto::PlataformaMovil:
        return ":/sprites/nivel2/plataforma.png";
    case TipoObjeto::BarrilExplosivo:
        return ":/sprites/nivel2/barril.png";
    case TipoObjeto::PastelDeYuca:
        return ":/sprites/nivel2/pastelyuca.png";
    default:
        return ":/sprites/nivel1/tronco.png";
    }
}

TipoObjeto ObjetoDelEscenario::tipo() const {
    return m_tipoObjeto;
}

void ObjetoDelEscenario::setTipo(TipoObjeto tipo) {
    m_tipoObjeto = tipo;
}

bool ObjetoDelEscenario::esRecogible() const {
    return m_recogible;
}

void ObjetoDelEscenario::setRecogible(bool recogible) {
    m_recogible = recogible;
}

void ObjetoDelEscenario::explotar() {
    if (m_tipoObjeto == TipoObjeto::BarrilExplosivo && estaActivo()) {
        recibirImpacto(100.0); // Causa dano maximo y se desactiva
    }
}

void ObjetoDelEscenario::recoger() {
    if (m_recogible && estaActivo()) {
        desactivar(); // Se desactiva al ser recogido por German
    }
}

// Getters y Setters
double ObjetoDelEscenario::amplitud() const { return m_amplitud; }
void ObjetoDelEscenario::setAmplitud(double amplitud) {
    if (amplitud < 0.0) throw std::invalid_argument("La amplitud debe ser mayor o igual a cero.");
    m_amplitud = amplitud;
}

double ObjetoDelEscenario::frecuencia() const { return m_frecuencia; }
void ObjetoDelEscenario::setFrecuencia(double frecuencia) {
    if (frecuencia <= 0.0) throw std::invalid_argument("La frecuencia debe ser mayor que cero.");
    m_frecuencia = frecuencia;
}

double ObjetoDelEscenario::longitudCuerda() const { return m_longitudCuerda; }
void ObjetoDelEscenario::setLongitudCuerda(double longitud) {
    if (longitud <= 0.0) throw std::invalid_argument("La longitud debe ser mayor que cero.");
    m_longitudCuerda = longitud;
}