#include "elementodebatalla.h"

ElementoDeBatalla::ElementoDeBatalla(QPointF pos, QPointF vel, QPointF tam, double vida)
    : m_posicion(pos), m_velocidad(vel), m_tamano(tam), m_integridad(vida), m_activo(true) {}

QPointF ElementoDeBatalla::posicion() const {
    return m_posicion;
}

void ElementoDeBatalla::setPosicion(const QPointF& pos) {
    m_posicion = pos;
}

QPointF ElementoDeBatalla::velocidad() const {
    return m_velocidad;
}

void ElementoDeBatalla::setVelocidad(const QPointF& vel) {
    m_velocidad = vel;
}

QRectF ElementoDeBatalla::limites() const {
    // Retorna el rectangulo delimitador centrado en la posicion actual
    return QRectF(m_posicion.x() - m_tamano.x() / 2.0,
                  m_posicion.y() - m_tamano.y() / 2.0,
                  m_tamano.x(),
                  m_tamano.y());
}

void ElementoDeBatalla::recibirImpacto(double dano) {
    if (!m_activo) return;

    m_integridad -= dano;
    if (m_integridad <= 0.0) {
        m_integridad = 0.0;
        m_activo = false; // Se desactiva automaticamente al llegar a cero la vida
    }
}

bool ElementoDeBatalla::estaActivo() const {
    return m_activo;
}

void ElementoDeBatalla::desactivar() {
    m_activo = false;
}