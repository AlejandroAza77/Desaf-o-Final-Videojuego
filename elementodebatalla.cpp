#include "elementodebatalla.h"
#include <algorithm>
#include <stdexcept>

ElementoDeBatalla::ElementoDeBatalla(const QPointF& pos, const QPointF& vel, const QPointF& tam, double vida)
    : m_posicion(pos), m_velocidad(vel), m_tamano(tam), m_integridad(vida), m_activo(true) {
    if (tam.x() <= 0.0 || tam.y() <= 0.0)
        throw std::invalid_argument("El tamano debe ser positivo.");
}

ElementoDeBatalla& ElementoDeBatalla::operator=(const ElementoDeBatalla& otro) {
    if (this == &otro) return *this;
    m_posicion = otro.m_posicion;
    m_velocidad = otro.m_velocidad;
    m_tamano = otro.m_tamano;
    m_integridad = otro.m_integridad;
    m_activo = otro.m_activo;
    return *this;
}

bool ElementoDeBatalla::operator==(const ElementoDeBatalla& otro) const {
    return typeid(*this) == typeid(otro) && m_posicion == otro.m_posicion &&
           m_velocidad == otro.m_velocidad && m_tamano == otro.m_tamano &&
           casiIgual(m_integridad, otro.m_integridad) && m_activo == otro.m_activo;
}

QPointF ElementoDeBatalla::posicion() const { return m_posicion; }
void ElementoDeBatalla::setPosicion(const QPointF& pos) { m_posicion = pos; }
QPointF ElementoDeBatalla::velocidad() const { return m_velocidad; }
void ElementoDeBatalla::setVelocidad(const QPointF& vel) { m_velocidad = vel; }
QPointF ElementoDeBatalla::tamano() const { return m_tamano; }

void ElementoDeBatalla::setTamano(const QPointF& tam) {
    if (tam.x() <= 0.0 || tam.y() <= 0.0)
        throw std::invalid_argument("El tamano debe ser positivo.");
    m_tamano = tam;
}

double ElementoDeBatalla::integridad() const { return m_integridad; }

void ElementoDeBatalla::setIntegridad(double vida) {
    m_integridad = std::max(0.0, vida);
    if (m_integridad == 0.0) m_activo = false;
}

bool ElementoDeBatalla::estaActivo() const { return m_activo; }
void ElementoDeBatalla::setActivo(bool activo) { m_activo = activo; }

QRectF ElementoDeBatalla::limites() const {
    return QRectF(m_posicion.x() - m_tamano.x() / 2.0, m_posicion.y() - m_tamano.y() / 2.0,
                  m_tamano.x(), m_tamano.y());
}

void ElementoDeBatalla::recibirImpacto(double dano) {
    if (!m_activo) return;
    m_integridad -= dano;
    if (m_integridad <= 0.0) {
        m_integridad = 0.0;
        m_activo = false;
    }
}

void ElementoDeBatalla::desactivar() { m_activo = false; }