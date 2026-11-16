#include "armas.h"
#include <cmath>

Armas::Armas(QPointF pos, QPointF vel, double dano, int rebotes, double tiempoVida, double g, double e, TipoProyectil tipo, QPointF tam)
    : ElementoDeBatalla(pos, vel, tam, 1.0),
    m_dano(dano),
    m_rebotesRestantes(rebotes),
    m_tiempoVida(tiempoVida),
    m_g(g),
    m_e(e),
    m_tipo(tipo),
    m_tiempoAcumulado(0.0) {

    // Validacion estricta requerida: g > 0 y 0 <= e <= 1
    if (m_g <= 0.0) {
        throw std::invalid_argument("Error: La gravedad g debe ser mayor que cero.");
    }
    if (m_e < 0.0 || m_e > 1.0) {
        throw std::invalid_argument("Error: El coeficiente de restitucion e debe estar entre 0 y 1.");
    }
}

void Armas::actualizar(double dt) {
    if (!estaActivo()) return;

    // Reducir el tiempo de vida restante
    m_tiempoVida -= dt;
    if (m_tiempoVida <= 0.0) {
        desactivar();
        return;
    }

    m_tiempoAcumulado += dt;

    // Movimiento parabolico: calculo de posicion y velocidad bajo la gravedad
    double nuevaVx = m_velocidad.x();
    double nuevaVy = m_velocidad.y() + m_g * dt; // Aceleracion vertical acumulada

    m_posicion.setX(m_posicion.x() + nuevaVx * dt);
    m_posicion.setY(m_posicion.y() + nuevaVy * dt);

    m_velocidad.setX(nuevaVx);
    m_velocidad.setY(nuevaVy);
}

QString Armas::rutaSprite() const {
    switch (m_tipo) {
    case TipoProyectil::Coco:
        return ":/sprites/nivel1/coco.png";
    case TipoProyectil::Piedra:
        return ":/sprites/nivel1/piedra.png";
    case TipoProyectil::BalaDeCanon:
        return ":/sprites/nivel1/baladecanon.png";
    default:
        return ":/sprites/nivel1/coco.png";
    }
}

void Armas::rebotar(const QPointF& normal) {
    if (m_rebotesRestantes <= 0) {
        desactivar();
        return;
    }

    m_rebotesRestantes--;

    // Rebote elastico usando QPointF::dotProduct: v' = v - (1 + e) * (v dot n) * n
    double productoPunto = QPointF::dotProduct(m_velocidad, normal);

    double vxNueva = m_velocidad.x() - (1.0 + m_e) * productoPunto * normal.x();
    double vyNueva = m_velocidad.y() - (1.0 + m_e) * productoPunto * normal.y();

    m_velocidad = QPointF(vxNueva, vyNueva);
}

bool Armas::haExpirado() const {
    return !estaActivo() || m_tiempoVida <= 0.0;
}

void Armas::reiniciar(QPointF pos, QPointF vel, double dano, int rebotes, double tiempoVida, double g, double e, TipoProyectil tipo) {
    m_posicion = pos;
    m_velocidad = vel;
    m_dano = dano;
    m_rebotesRestantes = rebotes;
    m_tiempoVida = tiempoVida;
    m_g = g;
    m_e = e;
    m_tipo = tipo;
    m_tiempoAcumulado = 0.0;
    m_activo = true;
}

// Getters y Setters
double Armas::dano() const { return m_dano; }
void Armas::setDano(double dano) { m_dano = dano; }

int Armas::rebotesRestantes() const { return m_rebotesRestantes; }
void Armas::setRebotesRestantes(int rebotes) { m_rebotesRestantes = rebotes; }

double Armas::tiempoVida() const { return m_tiempoVida; }
void Armas::setTiempoVida(double tiempoVida) { m_tiempoVida = tiempoVida; }

double Armas::gravedad() const { return m_g; }
void Armas::setGravedad(double g) {
    if (g <= 0.0) throw std::invalid_argument("La gravedad debe ser mayor que cero.");
    m_g = g;
}

double Armas::coeficienteRestitucion() const { return m_e; }
void Armas::setCoeficienteRestitucion(double e) {
    if (e < 0.0 || e > 1.0) throw std::invalid_argument("El coeficiente debe estar entre 0 y 1.");
    m_e = e;
}

TipoProyectil Armas::tipoProyectil() const { return m_tipo; }
void Armas::setTipoProyectil(TipoProyectil tipo) { m_tipo = tipo; }