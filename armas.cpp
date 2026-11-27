#include "armas.h"

namespace {
// Tamano en pantalla de cada proyectil (respeta la proporcion de su sprite).
QPointF tamanoPorTipo(TipoProyectil t) {
    switch (t) {
    case TipoProyectil::Coco: return QPointF(44, 31);
    case TipoProyectil::Piedra: return QPointF(36, 20);
    case TipoProyectil::BalaDeCanon: return QPointF(58, 22);
    case TipoProyectil::Cohete: return QPointF(86, 17);
    }
    return QPointF(25, 25);
}

void validar(double g, double e) {
    if (g <= 0.0) throw std::invalid_argument("La gravedad g debe ser mayor que cero.");
    if (e < 0.0 || e > 1.0) throw std::invalid_argument("El coeficiente e debe estar entre 0 y 1.");
}
}

Armas::Armas(const QPointF& pos, const QPointF& vel, double dano, int rebotes, double tiempoVida,
             double g, double e, TipoProyectil tipo, const QPointF& tam, ModoVista modo)
    : ElementoDeBatalla(pos, vel, tam, 1.0), m_dano(dano), m_rebotesRestantes(rebotes),
    m_tiempoVida(tiempoVida), m_g(g), m_e(e), m_tipo(tipo), m_modo(modo), m_altura(0.0),
    m_velAltura(0.0), m_aterrizo(false), m_destino(0, 0), m_tieneDestino(false), m_recto(false) {
    validar(m_g, m_e);
}

Armas& Armas::operator=(const Armas& otro) {
    if (this == &otro) return *this;
    ElementoDeBatalla::operator=(otro);
    m_dano = otro.m_dano;
    m_rebotesRestantes = otro.m_rebotesRestantes;
    m_tiempoVida = otro.m_tiempoVida;
    m_g = otro.m_g;
    m_e = otro.m_e;
    m_tipo = otro.m_tipo;
    m_modo = otro.m_modo;
    m_altura = otro.m_altura;
    m_velAltura = otro.m_velAltura;
    m_aterrizo = otro.m_aterrizo;
    m_destino = otro.m_destino;
    m_tieneDestino = otro.m_tieneDestino;
    m_recto = otro.m_recto;
    return *this;
}

bool Armas::operator==(const Armas& o) const {
    return ElementoDeBatalla::operator==(o) && casiIgual(m_dano, o.m_dano) &&
           m_rebotesRestantes == o.m_rebotesRestantes && casiIgual(m_tiempoVida, o.m_tiempoVida) &&
           casiIgual(m_g, o.m_g) && casiIgual(m_e, o.m_e) && m_tipo == o.m_tipo &&
           m_modo == o.m_modo && casiIgual(m_altura, o.m_altura) &&
           casiIgual(m_velAltura, o.m_velAltura) && m_aterrizo == o.m_aterrizo &&
           m_tieneDestino == o.m_tieneDestino && m_destino == o.m_destino && m_recto == o.m_recto;
}

void Armas::apuntar(const QPointF& origen, const QPointF& destino, double tiempoVuelo) {
    if (tiempoVuelo <= 0.0) throw std::invalid_argument("El tiempo de vuelo debe ser mayor que cero.");
    const double T = tiempoVuelo;
    const QPointF d = destino - origen;
    m_posicion = origen;
    if (m_modo == ModoVista::Cenital) {
        m_velocidad = QPointF(d.x() / T, d.y() / T);
        m_velAltura = 0.5 * m_g * T;  // vuelve a altura 0 justo en t = T
    } else {
        m_velocidad = QPointF(d.x() / T, (d.y() - 0.5 * m_g * T * T) / T);
        m_velAltura = 0.0;
    }
    m_altura = 0.0;
    m_destino = destino;
    m_tieneDestino = true;
    m_aterrizo = false;
    m_recto = false;
    m_tiempoVida = T + 0.5;
}

void Armas::apuntarDirecto(const QPointF& origen, const QPointF& destino, double tiempoHastaDestino) {
    if (tiempoHastaDestino <= 0.0) throw std::invalid_argument("El tiempo debe ser mayor que cero.");
    m_posicion = origen;
    m_velocidad = (destino - origen) / tiempoHastaDestino;
    m_altura = 0.0;
    m_velAltura = 0.0;
    m_destino = destino;
    m_tieneDestino = false;
    m_aterrizo = false;
    m_recto = true;
    m_tiempoVida = tiempoHastaDestino + 1.5;  // si falla, sigue de largo antes de desaparecer
}

void Armas::actualizar(double dt) {
    if (!estaActivo() || m_aterrizo) return;

    m_tiempoVida -= dt;
    if (m_tiempoVida <= 0.0) {
        desactivar();
        return;
    }

    if (m_modo == ModoVista::Cenital) {  // la gravedad actua sobre la altura
        m_posicion += m_velocidad * dt;
        m_altura += m_velAltura * dt;
        m_velAltura -= m_g * dt;
        if (m_altura <= 0.0 && m_velAltura < 0.0) {
            m_altura = 0.0;
            m_velAltura = 0.0;
            m_velocidad = QPointF(0, 0);
            m_aterrizo = true;
            if (m_tieneDestino) m_posicion = m_destino;
        }
    } else {                             // la gravedad actua sobre y
        if (!m_recto) m_velocidad.setY(m_velocidad.y() + m_g * dt);
        m_posicion += m_velocidad * dt;
    }
}

QString Armas::rutaSprite() const {
    switch (m_tipo) {
    case TipoProyectil::Coco: return QStringLiteral(":/sprites/nivel1/coco_vuelo.png");
    case TipoProyectil::Piedra: return QStringLiteral(":/sprites/nivel1/piedra.png");
    case TipoProyectil::BalaDeCanon: return QStringLiteral(":/sprites/nivel1/bala_vuelo.png");
    case TipoProyectil::Cohete: return QStringLiteral(":/sprites/nivel2/cohete_vuelo.png");
    }
    return QStringLiteral(":/sprites/nivel1/coco_vuelo.png");
}

// Los sprites de coco y bala llevan la estela a la izquierda (viajan hacia la derecha):
// si el proyectil va hacia la izquierda se espejan.
bool Armas::espejar() const {
    return m_tipo != TipoProyectil::Cohete && m_tipo != TipoProyectil::Piedra && m_velocidad.x() < 0.0;
}

// El cohete tiene la punta a la izquierda: se gira para que apunte hacia donde viaja.
double Armas::rotacion() const {
    if (m_tipo != TipoProyectil::Cohete) return 0.0;
    const double kPi = 3.14159265358979323846;
    return std::atan2(m_velocidad.y(), m_velocidad.x()) * 180.0 / kPi + 180.0;
}

bool Armas::esEnemiga() const { return m_tipo == TipoProyectil::BalaDeCanon || m_tipo == TipoProyectil::Cohete; }

QRectF Armas::areaImpacto() const {
    double r = 12.0;
    if (m_tipo == TipoProyectil::BalaDeCanon) r = 10.0;
    else if (m_tipo == TipoProyectil::Cohete) r = 14.0;
    return QRectF(m_posicion.x() - r, m_posicion.y() - r, 2.0 * r, 2.0 * r);
}

void Armas::rebotar(const QPointF& normal) {
    if (m_rebotesRestantes <= 0) {
        desactivar();
        return;
    }
    --m_rebotesRestantes;
    m_tieneDestino = false;
    const double pp = QPointF::dotProduct(m_velocidad, normal);
    m_velocidad = QPointF(m_velocidad.x() - (1.0 + m_e) * pp * normal.x(),
                          m_velocidad.y() - (1.0 + m_e) * pp * normal.y());
}

bool Armas::haExpirado() const { return !estaActivo() || m_tiempoVida <= 0.0; }

void Armas::reiniciar(const QPointF& pos, const QPointF& vel, double dano, int rebotes,
                      double tiempoVida, double g, double e, TipoProyectil tipo, ModoVista modo) {
    validar(g, e);
    m_posicion = pos;
    m_velocidad = vel;
    m_tamano = tamanoPorTipo(tipo);
    m_dano = dano;
    m_rebotesRestantes = rebotes;
    m_tiempoVida = tiempoVida;
    m_g = g;
    m_e = e;
    m_tipo = tipo;
    m_modo = modo;
    m_altura = 0.0;
    m_velAltura = 0.0;
    m_aterrizo = false;
    m_tieneDestino = false;
    m_recto = false;
    m_integridad = 1.0;
    m_activo = true;
}

double Armas::dano() const { return m_dano; }
void Armas::setDano(double dano) {
    if (dano < 0.0) throw std::invalid_argument("El dano no puede ser negativo.");
    m_dano = dano;
}
int Armas::rebotesRestantes() const { return m_rebotesRestantes; }
void Armas::setRebotesRestantes(int rebotes) {
    if (rebotes < 0) throw std::invalid_argument("Los rebotes no pueden ser negativos.");
    m_rebotesRestantes = rebotes;
}
double Armas::tiempoVida() const { return m_tiempoVida; }
void Armas::setTiempoVida(double tiempoVida) { m_tiempoVida = tiempoVida; }
double Armas::gravedad() const { return m_g; }
void Armas::setGravedad(double g) { validar(g, m_e); m_g = g; }
double Armas::coeficienteRestitucion() const { return m_e; }
void Armas::setCoeficienteRestitucion(double e) { validar(m_g, e); m_e = e; }
TipoProyectil Armas::tipoProyectil() const { return m_tipo; }
void Armas::setTipoProyectil(TipoProyectil tipo) { m_tipo = tipo; }
ModoVista Armas::modo() const { return m_modo; }
void Armas::setModo(ModoVista modo) { m_modo = modo; }
double Armas::altura() const { return m_altura; }
void Armas::setAltura(double altura) {
    if (altura < 0.0) throw std::invalid_argument("La altura no puede ser negativa.");
    m_altura = altura;
}
double Armas::velocidadAltura() const { return m_velAltura; }
void Armas::setVelocidadAltura(double v) { m_velAltura = v; }
bool Armas::haAterrizado() const { return m_aterrizo; }
void Armas::setAterrizo(bool aterrizo) { m_aterrizo = aterrizo; }
QPointF Armas::destino() const { return m_destino; }
void Armas::setDestino(const QPointF& destino) { m_destino = destino; }
bool Armas::esRecto() const { return m_recto; }
bool Armas::tieneDestino() const { return m_tieneDestino; }
void Armas::setTieneDestino(bool tiene) { m_tieneDestino = tiene; }