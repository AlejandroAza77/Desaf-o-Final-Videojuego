#include "germanquintero.h"
#include <algorithm>
#include <cmath>

GermanQuintero::GermanQuintero(const QPointF& pos, Modo modo, const QPointF& tam)
    : ElementoDeBatalla(pos, QPointF(0, 0), tam, kVida),
    m_modo(modo), m_tamanoBase(tam), m_direccion(0, 0), m_zona(), m_tieneZona(false),
    m_corriente(0, 0), m_suelo(pos.y() + tam.y() / 2.0), m_enSuelo(true), m_mu(0.3),
    m_cocos(kCapacidad), m_especialidad(Especialidad::Ninguna),
    m_tiempoEspecialidad(0.0), m_factorVelocidad(1.0), m_factorSalto(1.0),
    m_factorFriccion(1.0), m_factorDano(1.0), m_factorAlcance(1.0), m_escala(1.0) {
    if (tam.x() <= 0.0 || tam.y() <= 0.0)
        throw std::invalid_argument("Error: el tamano de German debe ser positivo.");
}

QString GermanQuintero::rutaSprite() const {
    // [Qt 6.x] QStringLiteral: sin reservar memoria. Se llama una sola vez.
    return m_modo == Modo::Cenital ? QStringLiteral(":/sprites/nivel1/german.png")
                                   : QStringLiteral(":/sprites/nivel2/german.png");
}

void GermanQuintero::setDireccion(const QPointF& dir) { m_direccion = dir; }
void GermanQuintero::setZonaMovimiento(const QRectF& zona) { m_zona = zona; m_tieneZona = true; }
void GermanQuintero::setCorriente(const QPointF& vel) { m_corriente = vel; }
void GermanQuintero::setSuelo(double sueloY) { m_suelo = sueloY; }
void GermanQuintero::setFriccion(double mu) {
    if (mu < 0.0) throw std::invalid_argument("Error: la friccion mu debe ser mayor o igual a cero.");
    m_mu = mu;
}
bool GermanQuintero::enSuelo() const { return m_enSuelo; }

void GermanQuintero::saltar() {
    if (m_modo == Modo::Plataforma && m_enSuelo) {
        m_velocidad.setY(-kSalto * m_factorSalto * m_escala);  // [Qt 6.x] y crece hacia abajo: arriba es negativo
        m_enSuelo = false;
    }
}

void GermanQuintero::actualizar(double dt) {
    if (dt <= 0.0 || !estaActivo()) return;

    if (m_tiempoEspecialidad > 0.0) {
        m_tiempoEspecialidad -= dt;
        if (m_tiempoEspecialidad <= 0.0) terminarEspecialidad();
    }
    if (m_modo == Modo::Cenital) moverCenital(dt);
    else moverPlataforma(dt);
}

void GermanQuintero::moverCenital(double dt) {
    // [Qt 6.x] Se normaliza para que ir en diagonal no sea mas rapido.
    QPointF dir = m_direccion;
    const double len = std::hypot(dir.x(), dir.y());
    if (len > 1.0) dir = dir * (1.0 / len);

    m_velocidad = dir * (kVelCenital * m_factorVelocidad);
    m_posicion += m_velocidad * dt;

    if (!m_tieneZona) return;

    // [Qt 6.x] Cerca del rio (borde derecho de la orilla) la corriente lo arrastra.
    const double distRio = m_zona.right() - (m_posicion.x() + m_tamano.x() / 2.0);
    if (distRio < kMargenCorriente) {
        const double k = std::min(1.0, (kMargenCorriente - distRio) / kMargenCorriente);
        m_posicion += m_corriente * (k * dt);
    }

    // [Qt 6.x] No sale de su orilla: el rio es el limite.
    const double hx = m_tamano.x() / 2.0, hy = m_tamano.y() / 2.0;
    m_posicion.setX(std::max(m_zona.left() + hx, std::min(m_posicion.x(), m_zona.right() - hx)));
    m_posicion.setY(std::max(m_zona.top() + hy, std::min(m_posicion.y(), m_zona.bottom() - hy)));
}

void GermanQuintero::moverPlataforma(double dt) {
    // [Qt 6.x] Friccion dinamica: aFric = mu * g. Cresta Veloz la reduce a la mitad.
    const double aFric = m_mu * m_factorFriccion * kG * m_escala;
    const double vmax = kVelMaxPlat * m_factorVelocidad * m_escala;
    double vx = m_velocidad.x();

    if (m_direccion.x() != 0.0) {
        // [Qt 6.x] Con impulso: aceleracion neta = impulso - friccion (en barro cuesta mas).
        const double neta = std::max(0.0, kAceleracion * m_escala - aFric);
        vx += (m_direccion.x() > 0.0 ? 1.0 : -1.0) * neta * dt;
    } else {
        // [Qt 6.x] Sin impulso: la friccion lo frena hasta detenerlo.
        const double dec = aFric * dt;
        if (std::abs(vx) <= dec) vx = 0.0;
        else vx -= (vx > 0.0 ? 1.0 : -1.0) * dec;
    }
    vx = std::max(-vmax, std::min(vx, vmax));

    // [Qt 6.x] Caida libre: v += g dt (y crece hacia abajo).
    const double vy = m_velocidad.y() + kG * m_escala * dt;
    m_velocidad = QPointF(vx, vy);
    m_posicion += m_velocidad * dt;

    const double pies = m_posicion.y() + m_tamano.y() / 2.0;
    if (pies >= m_suelo && vy >= 0.0) {
        m_posicion.setY(m_suelo - m_tamano.y() / 2.0);
        m_velocidad.setY(0.0);
        m_enSuelo = true;
    } else {
        m_enSuelo = false;
    }
}

int GermanQuintero::cocos() const { return m_cocos; }
int GermanQuintero::capacidadCocos() const { return kCapacidad; }

bool GermanQuintero::recogerCoco() {
    if (m_cocos >= kCapacidad) return false;
    ++m_cocos;
    return true;
}

double GermanQuintero::alcance() const { return kAlcanceBase * m_factorAlcance * m_escala; }

double GermanQuintero::escalaMundo() const { return m_escala; }
void GermanQuintero::setEscalaMundo(double escala) {
    if (escala <= 0.0) throw std::invalid_argument("Error: la escala del mundo debe ser mayor que cero.");
    const double k = escala / m_escala;
    m_escala = escala;
    m_tamanoBase = m_tamanoBase * k;
    m_tamano = m_tamano * k;
}

bool GermanQuintero::lanzarCoco(const QPointF& destino, Armas& proyectil) {
    if (m_cocos <= 0) return false;

    const QPointF origen = m_posicion;
    const QPointF d = destino - origen;
    double dist = std::hypot(d.x(), d.y());
    QPointF fin = destino;
    if (dist > alcance() && dist > 0.0) {
        // [Qt 6.x] Mas lejos que el alcance: cae en el limite, en la misma direccion.
        fin = origen + d * (alcance() / dist);
        dist = alcance();
    }
    // [Qt 6.x] Tiempo de vuelo proporcional a la distancia (tiro lejano tarda mas).
    const double T = std::max(kTVueloMin, dist / (kVelVuelo * m_escala));
    const ModoVista mv = (m_modo == Modo::Cenital) ? ModoVista::Cenital : ModoVista::Lateral;

    proyectil.reiniciar(origen, QPointF(0, 0), kDanoBase * m_factorDano, 2, T + 0.5,
                        kGravedadCoco * m_escala, 0.6, TipoProyectil::Coco, mv);
    proyectil.apuntar(origen, fin, T);
    --m_cocos;
    return true;
}

void GermanQuintero::cambiarTamano(const QPointF& nuevo) {
    // [Qt 6.x] En Plataforma se mantienen los pies en su sitio al crecer o encoger.
    if (m_modo == Modo::Plataforma)
        m_posicion.setY(m_posicion.y() - (nuevo.y() - m_tamano.y()) / 2.0);
    m_tamano = nuevo;
}

void GermanQuintero::terminarEspecialidad() {
    m_factorVelocidad = m_factorSalto = m_factorFriccion = m_factorDano = m_factorAlcance = 1.0;
    cambiarTamano(m_tamanoBase);
    m_especialidad = Especialidad::Ninguna;
    m_tiempoEspecialidad = 0.0;
}

void GermanQuintero::activarEspecialidad(Especialidad esp) {
    terminarEspecialidad();  // [Qt 6.x] primero se restaura lo anterior
    m_especialidad = esp;
    switch (esp) {
    case Especialidad::MachoAlfa:
        // [Qt 6.x] Mas grande, lanza con el doble de fuerza y alcance, pero -20% de velocidad.
        m_tiempoEspecialidad = kDuracionMacho;
        m_factorDano = 2.0;
        m_factorAlcance = 2.0;
        m_factorVelocidad = 0.8;
        cambiarTamano(m_tamanoBase * 1.3);
        break;
    case Especialidad::CrestaVeloz:
        // [Qt 6.x] Mas veloz, salta mas alto y la friccion se reduce a la mitad.
        m_tiempoEspecialidad = kDuracionCresta;
        m_factorVelocidad = 1.5;
        m_factorSalto = 1.3;
        m_factorFriccion = 0.5;
        break;
    case Especialidad::Ninguna:
        break;
    }
}

GermanQuintero::Desgaste GermanQuintero::desgaste() const {
    // Umbrales sobre la vida: con disparos de 25 sobre 100, cada impacto sube un nivel.
    if (m_integridad > 0.75 * kVida) return Desgaste::Normal;
    if (m_integridad > 0.50 * kVida) return Desgaste::Leve;
    if (m_integridad > 0.25 * kVida) return Desgaste::Medio;
    if (m_integridad > 0.0) return Desgaste::Fuerte;
    return Desgaste::Fantasma;
}

GermanQuintero::Especialidad GermanQuintero::especialidad() const { return m_especialidad; }
double GermanQuintero::tiempoEspecialidad() const { return m_tiempoEspecialidad; }
GermanQuintero::Modo GermanQuintero::modo() const { return m_modo; }

GermanQuintero& GermanQuintero::operator=(const GermanQuintero& otro) {
    if (this == &otro) return *this;
    ElementoDeBatalla::operator=(otro);
    m_modo = otro.m_modo;
    m_tamanoBase = otro.m_tamanoBase;
    m_direccion = otro.m_direccion;
    m_zona = otro.m_zona;
    m_tieneZona = otro.m_tieneZona;
    m_corriente = otro.m_corriente;
    m_suelo = otro.m_suelo;
    m_enSuelo = otro.m_enSuelo;
    m_mu = otro.m_mu;
    m_cocos = otro.m_cocos;
    m_especialidad = otro.m_especialidad;
    m_tiempoEspecialidad = otro.m_tiempoEspecialidad;
    m_factorVelocidad = otro.m_factorVelocidad;
    m_factorSalto = otro.m_factorSalto;
    m_factorFriccion = otro.m_factorFriccion;
    m_factorDano = otro.m_factorDano;
    m_factorAlcance = otro.m_factorAlcance;
    m_escala = otro.m_escala;
    return *this;
}

bool GermanQuintero::operator==(const GermanQuintero& o) const {
    return ElementoDeBatalla::operator==(o) && m_modo == o.m_modo && m_tamanoBase == o.m_tamanoBase &&
           m_direccion == o.m_direccion && m_tieneZona == o.m_tieneZona && m_zona == o.m_zona &&
           m_corriente == o.m_corriente && casiIgual(m_suelo, o.m_suelo) && m_enSuelo == o.m_enSuelo &&
           casiIgual(m_mu, o.m_mu) && m_cocos == o.m_cocos &&
           m_especialidad == o.m_especialidad && casiIgual(m_tiempoEspecialidad, o.m_tiempoEspecialidad) &&
           casiIgual(m_factorVelocidad, o.m_factorVelocidad) && casiIgual(m_factorSalto, o.m_factorSalto) &&
           casiIgual(m_factorFriccion, o.m_factorFriccion) && casiIgual(m_factorDano, o.m_factorDano) &&
           casiIgual(m_factorAlcance, o.m_factorAlcance) && casiIgual(m_escala, o.m_escala);
}

// ---- Accesores declarados en el .h que faltaba definir ----
void GermanQuintero::setModo(Modo modo) { m_modo = modo; }
QPointF GermanQuintero::tamanoBase() const { return m_tamanoBase; }
void GermanQuintero::setTamanoBase(const QPointF& tam) {
    if (tam.x() <= 0.0 || tam.y() <= 0.0)
        throw std::invalid_argument("Error: el tamano base debe ser positivo.");
    m_tamanoBase = tam;
}
QPointF GermanQuintero::direccion() const { return m_direccion; }
QRectF GermanQuintero::zona() const { return m_zona; }
bool GermanQuintero::tieneZona() const { return m_tieneZona; }
void GermanQuintero::setTieneZona(bool tiene) { m_tieneZona = tiene; }
QPointF GermanQuintero::corriente() const { return m_corriente; }
double GermanQuintero::suelo() const { return m_suelo; }
void GermanQuintero::setEnSuelo(bool enSuelo) { m_enSuelo = enSuelo; }
double GermanQuintero::friccion() const { return m_mu; }
void GermanQuintero::setCocos(int cocos) {
    if (cocos < 0 || cocos > kCapacidad)
        throw std::invalid_argument("Error: los cocos deben estar entre 0 y la capacidad.");
    m_cocos = cocos;
}
void GermanQuintero::setEspecialidad(Especialidad esp) { m_especialidad = esp; }
void GermanQuintero::setTiempoEspecialidad(double t) {
    if (t < 0.0) throw std::invalid_argument("Error: el tiempo de especialidad no puede ser negativo.");
    m_tiempoEspecialidad = t;
}
double GermanQuintero::factorVelocidad() const { return m_factorVelocidad; }
void GermanQuintero::setFactorVelocidad(double f) {
    if (f <= 0.0) throw std::invalid_argument("Error: el factor de velocidad debe ser mayor que cero.");
    m_factorVelocidad = f;
}
double GermanQuintero::factorSalto() const { return m_factorSalto; }
void GermanQuintero::setFactorSalto(double f) {
    if (f <= 0.0) throw std::invalid_argument("Error: el factor de salto debe ser mayor que cero.");
    m_factorSalto = f;
}
double GermanQuintero::factorFriccion() const { return m_factorFriccion; }
void GermanQuintero::setFactorFriccion(double f) {
    if (f < 0.0) throw std::invalid_argument("Error: el factor de friccion no puede ser negativo.");
    m_factorFriccion = f;
}
double GermanQuintero::factorDano() const { return m_factorDano; }
void GermanQuintero::setFactorDano(double f) {
    if (f <= 0.0) throw std::invalid_argument("Error: el factor de dano debe ser mayor que cero.");
    m_factorDano = f;
}
double GermanQuintero::factorAlcance() const { return m_factorAlcance; }
void GermanQuintero::setFactorAlcance(double f) {
    if (f <= 0.0) throw std::invalid_argument("Error: el factor de alcance debe ser mayor que cero.");
    m_factorAlcance = f;
}
