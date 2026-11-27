#include "nivelrioteatinos.h"
#include "armas.h"
#include "enemigorealista.h"
#include "germanquintero.h"
#include "objetodelescenario.h"
#include <QRandomGenerator>

namespace {
constexpr int kCarretasMoviles = 8;  // algunas estan quemandose o hechas escombros mientras otras avanzan
constexpr int kReservaEfectos = 30;
constexpr int kReservaArmas = 24;
constexpr double kAlcanceRebote = 110.0;
constexpr double kTiempoRebote = 0.45;
constexpr double kRadioImpacto = 24.0;
constexpr double kIntervaloCoco = 2.0;  // s entre cocos nuevos (infinitos)
constexpr int kMaxCocosSuelo = 6;       // maximo de cocos en la orilla a la vez
constexpr int kReservaCocos = 16;
constexpr double kDuracionCrater = 10.0;  // s que dura un crater antes de borrarse
constexpr double kEscalonCanon = 0.7;
// El puente de piedra cruza el rio (sobre la zona de agua): lo que cae ahi no cae al agua.
const QRectF kZonaPuente(280.0, 135.0, 640.0, 150.0);  // cada canon dispara 0.7 s mas lento que el anterior
}

bool NivelRioTeatinos::Parametros::operator==(const Parametros& o) const {
    return casiIgual(rapidezCarreta, o.rapidezCarreta) && casiIgual(intervaloAparicion, o.intervaloAparicion) &&
           casiIgual(intervaloCanon, o.intervaloCanon) && casiIgual(anticipo, o.anticipo) &&
           casiIgual(corriente, o.corriente) && maxCruzadas == o.maxCruzadas;
}

NivelRioTeatinos::Parametros NivelRioTeatinos::parametrosPara(Dificultad d) {
    switch (d) {
    case Dificultad::Facil: return {28.0, 3.5, 4.5, 0.3, 70.0, 5};
    case Dificultad::Normal: return {35.0, 2.5, 3.5, 1.0, 120.0, 3};
    case Dificultad::Dificil: return {45.0, 1.9, 2.5, 1.7, 170.0, 2};
    }
    throw std::invalid_argument("Dificultad invalida.");
}

NivelRioTeatinos::NivelRioTeatinos(GermanQuintero* jugador, Dificultad dificultad, double tiempo)
    : NivelDeBatalla(tiempo, jugador), m_dificultad(dificultad), m_parametros(parametrosPara(dificultad)),
      m_carretasObjetivo(10), m_carretasDestruidas(0), m_carretasCruzaron(0),
      m_maxCruzadas(m_parametros.maxCruzadas), m_carril(0), m_intervaloAparicion(m_parametros.intervaloAparicion),
      m_tiempoAparicion(1.0),
      m_zonaJugador(0, 0, 280, 480), m_zonaAgua(280, 0, 585, 480), m_corriente(0, m_parametros.corriente),
      m_inicioCarretas(915, 190), m_metaCarretas(215, 190), m_tiempoCoco(kIntervaloCoco) {
    if (jugador == nullptr) throw std::invalid_argument("El nivel necesita a German.");
    construirEscenario();
}

NivelRioTeatinos& NivelRioTeatinos::operator=(const NivelRioTeatinos& otro) {
    if (this == &otro) return *this;
    NivelDeBatalla::operator=(otro);
    m_dificultad = otro.m_dificultad;
    m_parametros = otro.m_parametros;
    m_carretasObjetivo = otro.m_carretasObjetivo;
    m_carretasDestruidas = otro.m_carretasDestruidas;
    m_carretasCruzaron = otro.m_carretasCruzaron;
    m_maxCruzadas = otro.m_maxCruzadas;
    m_carril = otro.m_carril;
    m_intervaloAparicion = otro.m_intervaloAparicion;
    m_tiempoAparicion = otro.m_tiempoAparicion;
    m_zonaJugador = otro.m_zonaJugador;
    m_zonaAgua = otro.m_zonaAgua;
    m_corriente = otro.m_corriente;
    m_inicioCarretas = otro.m_inicioCarretas;
    m_metaCarretas = otro.m_metaCarretas;
    m_tiempoCoco = otro.m_tiempoCoco;
    return *this;
}

bool NivelRioTeatinos::operator==(const NivelRioTeatinos& o) const {
    return NivelDeBatalla::operator==(o) && m_dificultad == o.m_dificultad && m_parametros == o.m_parametros &&
           m_carretasObjetivo == o.m_carretasObjetivo &&
           m_carretasDestruidas == o.m_carretasDestruidas && m_carretasCruzaron == o.m_carretasCruzaron &&
           m_maxCruzadas == o.m_maxCruzadas && m_carril == o.m_carril &&
           casiIgual(m_intervaloAparicion, o.m_intervaloAparicion) &&
           casiIgual(m_tiempoAparicion, o.m_tiempoAparicion) && m_zonaJugador == o.m_zonaJugador &&
           m_zonaAgua == o.m_zonaAgua && m_corriente == o.m_corriente &&
           m_inicioCarretas == o.m_inicioCarretas && m_metaCarretas == o.m_metaCarretas &&
           casiIgual(m_tiempoCoco, o.m_tiempoCoco);
}

void NivelRioTeatinos::construirEscenario() {
    m_jugador->setModo(GermanQuintero::Modo::Cenital);
    m_jugador->setZonaMovimiento(m_zonaJugador);
    m_jugador->setCorriente(m_corriente);
    crearReservaArmas(kReservaArmas);
    crearReservaEfectos(kReservaEfectos);

    m_enemigos.reserve(kCarretasMoviles + 2);
    for (int i = 0; i < kCarretasMoviles; ++i) {
        EnemigoRealista* c = new EnemigoRealista(TipoEnemigo::Carreta, m_inicioCarretas, nullptr, m_metaCarretas, m_parametros.rapidezCarreta);
        c->desactivar();
        agregarEnemigo(c);
    }
    // Dos canones (se quito el del centro: el nivel era demasiado dificil).
    const double alturasCanon[] = {90.0, 390.0};
    for (int i = 0; i < 2; ++i) {
        const QPointF p(930, alturasCanon[i]);
        EnemigoRealista* canon = new EnemigoRealista(TipoEnemigo::Canon, p, nullptr, p, 0.0, m_parametros.intervaloCanon + kEscalonCanon * i);
        canon->setVariante(i);  // un canon "a" y otro "b"
        try {
            agregarEnemigo(canon);
        } catch (...) {
            delete canon;
            throw;
        }
    }

    const QPointF cocos[] = {{60, 100}, {150, 200}, {40, 330}, {170, 400}, {100, 450}, {200, 260}};
    for (const QPointF& p : cocos)
        agregarObjetoEscenario(new ObjetoDelEscenario(p, TipoObjeto::CocoEnSuelo, 0, 1, 0, 50, QPointF(44, 28)));
    crearReservaCocos(kReservaCocos);  // cocos que iran apareciendo (nunca se acaban)
    const QPointF rocas[] = {{413, 54}, {597, 108}, {799, 111}, {510, 346}, {821, 385}};
    int vr = 0;
    for (const QPointF& p : rocas) {
        ObjetoDelEscenario* roca = new ObjetoDelEscenario(p, TipoObjeto::Roca, 0, 1, 0, 50, QPointF(46, 36));
        roca->setVariante(vr++);
        agregarObjetoEscenario(roca);
    }
    // (sin troncos flotantes en el rio)
}

void NivelRioTeatinos::generarCarreta() {
    for (EnemigoRealista* c : m_enemigos) {
        if (!c->esMovil() || !c->libre()) continue;  // libre: ya no se ve (tampoco sus restos)
        c->setVariante(m_carril);                    // alterna carreta "a" y "b"
        const double y = m_inicioCarretas.y() + 40.0 * (m_carril % 3 - 1);
        c->reiniciar(QPointF(m_inicioCarretas.x(), y), QPointF(m_metaCarretas.x(), y), m_parametros.rapidezCarreta, 50.0);
        ++m_carril;
        return;
    }
}

void NivelRioTeatinos::dejarCocoEnSuelo(const QPointF& pos) { colocarCocoEnSuelo(pos); }

// Un coco nuevo en un punto al azar de la orilla del jugador (lejos de la corriente del rio).
void NivelRioTeatinos::aparecerCocoAleatorio() {
    QRandomGenerator* azar = QRandomGenerator::global();
    const double x = m_zonaJugador.left() + 30.0 + azar->generateDouble() * (m_zonaJugador.width() - 120.0);
    const double y = m_zonaJugador.top() + 40.0 + azar->generateDouble() * (m_zonaJugador.height() - 80.0);
    colocarCocoEnSuelo(QPointF(x, y));
}

void NivelRioTeatinos::resolverProyectil(Armas& a) {
    const QPointF p = a.posicion();
    const QRectF impacto(p.x() - kRadioImpacto, p.y() - kRadioImpacto, 2 * kRadioImpacto, 2 * kRadioImpacto);

    if (a.esEnemiga()) {  // bala de canon: golpea a German y deja un crater donde cae
        if (m_jugador->limites().intersects(impacto)) m_jugador->recibirImpacto(a.dano());
        if (kZonaPuente.contains(p))
            lanzarEfecto(TipoEfecto::CraterPiedra, p, kDuracionCrater, 1.3);  // en el puente: crater de piedra
        else if (!m_zonaAgua.contains(p))
            lanzarEfecto(m_zonaJugador.contains(p) ? TipoEfecto::CraterPasto : TipoEfecto::CraterTierra, p, kDuracionCrater, 1.3);
        lanzarEfecto(TipoEfecto::BalaImpacto, p, 0.4, 1.2);
        a.desactivar();
        return;
    }

    for (EnemigoRealista* c : m_enemigos) {
        if (!c->esMovil() || !c->estaActivo() || !c->limites().intersects(impacto)) continue;
        c->recibirImpacto(a.dano());
        lanzarEfecto(TipoEfecto::CocoRompe, p, 0.4);
        if (!c->estaActivo()) {
            ++m_carretasDestruidas;
            lanzarEfecto(TipoEfecto::Explosion, c->posicion(), 0.5);  // luego quedan los escombros y se hacen cenizas
        }
        a.desactivar();
        return;
    }
    for (ObjetoDelEscenario* o : m_objetosEscenario) {
        const bool rebota = o->tipo() == TipoObjeto::Roca || o->tipo() == TipoObjeto::TroncoFlotante;
        if (!rebota || !o->estaActivo() || !o->limites().contains(p) || a.rebotesRestantes() <= 0) continue;
        const QPointF normal = normalizado(p - o->posicion());
        a.setRebotesRestantes(a.rebotesRestantes() - 1);
        a.apuntar(p, p + normal * (kAlcanceRebote * a.coeficienteRestitucion()), kTiempoRebote);
        return;
    }
    if (kZonaPuente.contains(p)) {  // el coco pega en el puente: pequeno crater de piedra y se rompe
        lanzarEfecto(TipoEfecto::CraterPiedra, p, 4.0, 0.35);
        lanzarEfecto(TipoEfecto::CocoRompe, p, 0.4);
    } else if (!m_zonaAgua.contains(p)) {
        dejarCocoEnSuelo(p);
        lanzarEfecto(TipoEfecto::CraterPasto, p, 4.0, 0.35);  // crater pequenito donde cae el coco
    } else {
        lanzarEfecto(TipoEfecto::CocoAgua, p, 0.7);  // cae al rio: salpicadura
    }
    a.desactivar();
}

void NivelRioTeatinos::recogerCocos() {
    for (ObjetoDelEscenario* o : m_objetosEscenario) {
        if (o->tipo() != TipoObjeto::CocoEnSuelo || !o->estaActivo()) continue;
        if (m_jugador->limites().intersects(o->limites()) && m_jugador->recogerCoco()) o->recoger();
    }
}

void NivelRioTeatinos::actualizar(double dt) {
    if (dt <= 0.0 || victoria() || derrota()) return;
    actualizarComun(dt);

    m_tiempoAparicion -= dt;
    if (m_tiempoAparicion <= 0.0) {
        generarCarreta();
        m_tiempoAparicion = m_intervaloAparicion;
    }

    const QPointF objetivo = m_jugador->posicion() + m_jugador->velocidad() * m_parametros.anticipo;
    for (EnemigoRealista* c : m_enemigos) {
        if (!c->estaActivo()) continue;
        if (c->esMovil() && c->cruzoElPuente()) {
            ++m_carretasCruzaron;
            c->desactivar();
        } else if (c->puedeDisparar()) {
            if (Armas* a = armaLibre()) c->disparar(objetivo, *a);
        }
    }
    for (Armas* a : m_armas)
        if (a->estaActivo() && a->haAterrizado()) resolverProyectil(*a);
    recogerCocos();

    m_tiempoCoco -= dt;
    if (m_tiempoCoco <= 0.0) {
        m_tiempoCoco = kIntervaloCoco;
        if (cocosEnSuelo() < kMaxCocosSuelo) aparecerCocoAleatorio();
    }
}

bool NivelRioTeatinos::victoria() const { return m_carretasDestruidas >= m_carretasObjetivo; }
bool NivelRioTeatinos::derrota() const { return NivelDeBatalla::derrota() || m_carretasCruzaron >= m_maxCruzadas; }

NivelRioTeatinos::Dificultad NivelRioTeatinos::dificultad() const { return m_dificultad; }
void NivelRioTeatinos::setDificultad(Dificultad d) {
    m_parametros = parametrosPara(d);
    m_dificultad = d;
    aplicarDificultad();
}
NivelRioTeatinos::Parametros NivelRioTeatinos::parametros() const { return m_parametros; }

void NivelRioTeatinos::aplicarDificultad() {
    int canon = 0;
    for (EnemigoRealista* e : m_enemigos) {
        if (e->esMovil()) e->setRapidez(m_parametros.rapidezCarreta);
        else if (e->tipo() == TipoEnemigo::Canon) e->setIntervaloDisparo(m_parametros.intervaloCanon + kEscalonCanon * canon++);
    }
    m_maxCruzadas = m_parametros.maxCruzadas;
    m_intervaloAparicion = m_parametros.intervaloAparicion;
    m_corriente = QPointF(0, m_parametros.corriente);
    m_jugador->setCorriente(m_corriente);
}

int NivelRioTeatinos::carretasObjetivo() const { return m_carretasObjetivo; }
void NivelRioTeatinos::setCarretasObjetivo(int n) {
    if (n <= 0) throw std::invalid_argument("El objetivo debe ser mayor que cero.");
    m_carretasObjetivo = n;
}
int NivelRioTeatinos::carretasDestruidas() const { return m_carretasDestruidas; }
void NivelRioTeatinos::setCarretasDestruidas(int n) {
    if (n < 0) throw std::invalid_argument("El contador no puede ser negativo.");
    m_carretasDestruidas = n;
}
int NivelRioTeatinos::carretasCruzaron() const { return m_carretasCruzaron; }
void NivelRioTeatinos::setCarretasCruzaron(int n) {
    if (n < 0) throw std::invalid_argument("El contador no puede ser negativo.");
    m_carretasCruzaron = n;
}
int NivelRioTeatinos::maxCruzadas() const { return m_maxCruzadas; }
void NivelRioTeatinos::setMaxCruzadas(int n) {
    if (n <= 0) throw std::invalid_argument("El maximo debe ser mayor que cero.");
    m_maxCruzadas = n;
}
int NivelRioTeatinos::carril() const { return m_carril; }
void NivelRioTeatinos::setCarril(int carril) { m_carril = carril; }
double NivelRioTeatinos::intervaloAparicion() const { return m_intervaloAparicion; }
void NivelRioTeatinos::setIntervaloAparicion(double t) {
    if (t <= 0.0) throw std::invalid_argument("El intervalo debe ser mayor que cero.");
    m_intervaloAparicion = t;
}
double NivelRioTeatinos::tiempoAparicion() const { return m_tiempoAparicion; }
void NivelRioTeatinos::setTiempoAparicion(double t) { m_tiempoAparicion = t; }
QRectF NivelRioTeatinos::zonaJugador() const { return m_zonaJugador; }
void NivelRioTeatinos::setZonaJugador(const QRectF& zona) { m_zonaJugador = zona; m_jugador->setZonaMovimiento(zona); }
QRectF NivelRioTeatinos::zonaAgua() const { return m_zonaAgua; }
void NivelRioTeatinos::setZonaAgua(const QRectF& zona) { m_zonaAgua = zona; }
QPointF NivelRioTeatinos::corriente() const { return m_corriente; }
void NivelRioTeatinos::setCorriente(const QPointF& vel) { m_corriente = vel; m_jugador->setCorriente(vel); }
QPointF NivelRioTeatinos::inicioCarretas() const { return m_inicioCarretas; }
void NivelRioTeatinos::setInicioCarretas(const QPointF& p) { m_inicioCarretas = p; }
QPointF NivelRioTeatinos::metaCarretas() const { return m_metaCarretas; }
void NivelRioTeatinos::setMetaCarretas(const QPointF& p) { m_metaCarretas = p; }
