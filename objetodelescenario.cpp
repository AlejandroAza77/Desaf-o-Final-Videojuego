#include "objetodelescenario.h"
#include <cmath>

ObjetoDelEscenario::ObjetoDelEscenario(const QPointF& posBase, TipoObjeto tipo, double amplitud,
                                       double frecuencia, double fase, double longitud,
                                       const QPointF& tam, bool oscilaEnX)
    : ElementoDeBatalla(posBase, QPointF(0, 0), tam, 100.0), m_tipoObjeto(tipo), m_recogible(false),
    m_estatico(false), m_oscilaEnX(oscilaEnX), m_amplitud(amplitud), m_frecuencia(frecuencia),
    m_fase(fase), m_longitudCuerda(longitud), m_posicionBase(posBase), m_tiempoAcumulado(0.0) {
    if (m_amplitud < 0.0) throw std::invalid_argument("La amplitud debe ser mayor o igual a cero.");
    if (m_frecuencia <= 0.0) throw std::invalid_argument("La frecuencia debe ser mayor que cero.");
    if (m_longitudCuerda <= 0.0) throw std::invalid_argument("La longitud debe ser mayor que cero.");
    configurarSegunTipo();
}

void ObjetoDelEscenario::configurarSegunTipo() {
    m_recogible = (m_tipoObjeto == TipoObjeto::PastelDeYuca || m_tipoObjeto == TipoObjeto::CocoEnSuelo);
    m_estatico = !(m_tipoObjeto == TipoObjeto::TroncoFlotante ||
                   m_tipoObjeto == TipoObjeto::TrampaPendulo ||
                   m_tipoObjeto == TipoObjeto::PlataformaMovil);
}

ObjetoDelEscenario& ObjetoDelEscenario::operator=(const ObjetoDelEscenario& otro) {
    if (this == &otro) return *this;
    ElementoDeBatalla::operator=(otro);
    m_tipoObjeto = otro.m_tipoObjeto;
    m_recogible = otro.m_recogible;
    m_estatico = otro.m_estatico;
    m_oscilaEnX = otro.m_oscilaEnX;
    m_amplitud = otro.m_amplitud;
    m_frecuencia = otro.m_frecuencia;
    m_fase = otro.m_fase;
    m_longitudCuerda = otro.m_longitudCuerda;
    m_posicionBase = otro.m_posicionBase;
    m_tiempoAcumulado = otro.m_tiempoAcumulado;
    return *this;
}

bool ObjetoDelEscenario::operator==(const ObjetoDelEscenario& o) const {
    return ElementoDeBatalla::operator==(o) && m_tipoObjeto == o.m_tipoObjeto &&
           m_recogible == o.m_recogible && m_estatico == o.m_estatico &&
           m_oscilaEnX == o.m_oscilaEnX && casiIgual(m_amplitud, o.m_amplitud) &&
           casiIgual(m_frecuencia, o.m_frecuencia) && casiIgual(m_fase, o.m_fase) &&
           casiIgual(m_longitudCuerda, o.m_longitudCuerda) && m_posicionBase == o.m_posicionBase &&
           casiIgual(m_tiempoAcumulado, o.m_tiempoAcumulado);
}

// Sobrecarga del operador de asignacion
ObjetoDelEscenario& ObjetoDelEscenario::operator=(const ObjetoDelEscenario& otro) {
    if (this != &otro) {
        ElementoDeBatalla::operator=(otro); // Asignar la parte heredada
        m_tipoObjeto = otro.m_tipoObjeto;
        m_recogible = otro.m_recogible;
        m_amplitud = otro.m_amplitud;
        m_frecuencia = otro.m_frecuencia;
        m_fase = otro.m_fase;
        m_longitudCuerda = otro.m_longitudCuerda;
        m_posicionBase = otro.m_posicionBase;
        m_tiempoAcumulado = otro.m_tiempoAcumulado;
    }
    return *this;
}

void ObjetoDelEscenario::actualizar(double dt) {
    if (m_estatico || !estaActivo()) return;

    m_tiempoAcumulado += dt;
    switch (m_tipoObjeto) {
    case TipoObjeto::TroncoFlotante: {
        const double d = m_amplitud * std::cos(m_frecuencia * m_tiempoAcumulado + m_fase);
        if (m_oscilaEnX) m_posicion.setX(m_posicionBase.x() + d);
        else m_posicion.setY(m_posicionBase.y() + d);
        break;
    }
    case TipoObjeto::TrampaPendulo: {
        const double theta = m_amplitud * std::cos(m_frecuencia * m_tiempoAcumulado);
        m_posicion = QPointF(m_posicionBase.x() + m_longitudCuerda * std::sin(theta),
                             m_posicionBase.y() + m_longitudCuerda * std::cos(theta));
        break;
    }
    case TipoObjeto::PlataformaMovil: {
        const double d = m_amplitud * std::sin(m_frecuencia * m_tiempoAcumulado);
        if (m_oscilaEnX) m_posicion.setX(m_posicionBase.x() + d);
        else m_posicion.setY(m_posicionBase.y() + d);
        break;
    }
    default:
        break;
    }
}

QString ObjetoDelEscenario::rutaSprite() const {
    switch (m_tipoObjeto) {
    case TipoObjeto::TroncoFlotante:  return QStringLiteral(":/sprites/nivel1/tronco.png");
    case TipoObjeto::Roca:            return QStringLiteral(":/sprites/nivel1/roca.png");
    case TipoObjeto::CocoEnSuelo:     return QStringLiteral(":/sprites/nivel1/coco.png");
    case TipoObjeto::TrampaPendulo:   return QStringLiteral(":/sprites/nivel2/boledora.png");
    case TipoObjeto::PlataformaMovil: return QStringLiteral(":/sprites/nivel2/plataforma.png");
    case TipoObjeto::BarrilExplosivo: return QStringLiteral(":/sprites/nivel2/barril.png");
    case TipoObjeto::PastelDeYuca:    return QStringLiteral(":/sprites/nivel2/pastelyuca.png");
    }
    return QStringLiteral(":/sprites/nivel1/tronco.png");
}

void ObjetoDelEscenario::explotar() {
    if (m_tipoObjeto == TipoObjeto::BarrilExplosivo && estaActivo()) recibirImpacto(100.0);
}

void ObjetoDelEscenario::recoger() {
    if (m_recogible && estaActivo()) desactivar();
}

TipoObjeto ObjetoDelEscenario::tipo() const { return m_tipoObjeto; }
void ObjetoDelEscenario::setTipo(TipoObjeto tipo) { m_tipoObjeto = tipo; configurarSegunTipo(); }
bool ObjetoDelEscenario::esRecogible() const { return m_recogible; }
void ObjetoDelEscenario::setRecogible(bool recogible) { m_recogible = recogible; }
bool ObjetoDelEscenario::esEstatico() const { return m_estatico; }
void ObjetoDelEscenario::setEstatico(bool estatico) { m_estatico = estatico; }
bool ObjetoDelEscenario::oscilaEnX() const { return m_oscilaEnX; }
void ObjetoDelEscenario::setOscilaEnX(bool oscilaEnX) { m_oscilaEnX = oscilaEnX; }
double ObjetoDelEscenario::amplitud() const { return m_amplitud; }
void ObjetoDelEscenario::setAmplitud(double a) {
    if (a < 0.0) throw std::invalid_argument("La amplitud debe ser mayor o igual a cero.");
    m_amplitud = a;
}
double ObjetoDelEscenario::frecuencia() const { return m_frecuencia; }
void ObjetoDelEscenario::setFrecuencia(double f) {
    if (f <= 0.0) throw std::invalid_argument("La frecuencia debe ser mayor que cero.");
    m_frecuencia = f;
}
double ObjetoDelEscenario::fase() const { return m_fase; }
void ObjetoDelEscenario::setFase(double fase) { m_fase = fase; }
double ObjetoDelEscenario::longitudCuerda() const { return m_longitudCuerda; }
void ObjetoDelEscenario::setLongitudCuerda(double l) {
    if (l <= 0.0) throw std::invalid_argument("La longitud debe ser mayor que cero.");
    m_longitudCuerda = l;
}
QPointF ObjetoDelEscenario::posicionBase() const { return m_posicionBase; }
void ObjetoDelEscenario::setPosicionBase(const QPointF& pos) { m_posicionBase = pos; }
double ObjetoDelEscenario::tiempoAcumulado() const { return m_tiempoAcumulado; }
void ObjetoDelEscenario::setTiempoAcumulado(double t) {
    if (t < 0.0) throw std::invalid_argument("El tiempo acumulado no puede ser negativo.");
    m_tiempoAcumulado = t;
}