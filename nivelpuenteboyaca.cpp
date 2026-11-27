#include "nivelpuenteboyaca.h"
#include "armas.h"
#include "enemigorealista.h"
#include "germanquintero.h"
#include "objetodelescenario.h"
#include <QRandomGenerator>
#include <algorithm>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr int kReservaArmas = 30;
constexpr int kReservaCocos = 18;
constexpr int kReservaEfectos = 30;
constexpr double kIntervaloCoco = 1.0;  // s entre cocos nuevos (infinitos)
constexpr int kMaxCocosCerca = 6;          // cocos que debe haber siempre alrededor de German
constexpr double kRadioCocosCerca = 900.0; // "alrededor" = 900 px a cada lado
constexpr double kRadioCocosLejos = 1600.0; // los que quedan mas lejos se reciclan para volver a aparecer cerca
constexpr double kDanoPendulo = 15.0;
constexpr double kDanoBarril = 20.0;  // ahora es una caja de madera
constexpr double kInvulnerabilidad = 1.0;
constexpr double kRadioExplosion = 110.0;
constexpr double kRapidezCentinela = 0.0;    // los canones son estaticos: solo giran a la izquierda o a la derecha
constexpr double kRapidezComandante = 65.0;
constexpr double kRapidezAbelardo = 90.0;   // rapido para alcanzar a German
constexpr double kFactorRadioJefe = 3.0;     // Abelardo ve desde 3 veces mas lejos
constexpr double kEsc = NivelPuenteBoyaca::kEscala;
}

bool NivelPuenteBoyaca::Parametros::operator==(const Parametros& o) const {
    return casiIgual(radioVision, o.radioVision) && casiIgual(retardo, o.retardo) && umbral == o.umbral &&
           casiIgual(muBarro, o.muBarro) && casiIgual(anguloPendulo, o.anguloPendulo);
}

NivelPuenteBoyaca::Parametros NivelPuenteBoyaca::parametrosPara(Dificultad d) {
    switch (d) {
    case Dificultad::Facil: return {150.0, 0.80, 4, 0.5, 20.0};
    case Dificultad::Normal: return {250.0, 0.50, 3, 0.7, 35.0};
    case Dificultad::Dificil: return {350.0, 0.25, 2, 0.9, 50.0};
    }
    throw std::invalid_argument("Dificultad invalida.");
}

NivelPuenteBoyaca::NivelPuenteBoyaca(GermanQuintero* jugador, Dificultad dificultad, double tiempo)
    : NivelDeBatalla(tiempo, jugador), m_dificultad(dificultad), m_parametros(parametrosPara(dificultad)),
      m_sueloY(400.0), m_metaX(9850.0), m_anchoMundo(10000.0), m_muPrado(0.3), m_invulnerable(0.0), m_tiempoCoco(kIntervaloCoco),
      m_indiceApoyo(-1), m_xApoyoPrev(0.0) {
    if (jugador == nullptr) throw std::invalid_argument("El nivel necesita a German.");
    construirEscenario();
}

NivelPuenteBoyaca& NivelPuenteBoyaca::operator=(const NivelPuenteBoyaca& otro) {
    if (this == &otro) return *this;
    NivelDeBatalla::operator=(otro);
    m_dificultad = otro.m_dificultad;
    m_parametros = otro.m_parametros;
    m_zonasBarro = otro.m_zonasBarro;
    m_sueloY = otro.m_sueloY;
    m_metaX = otro.m_metaX;
    m_anchoMundo = otro.m_anchoMundo;
    m_muPrado = otro.m_muPrado;
    m_invulnerable = otro.m_invulnerable;
    m_tiempoCoco = otro.m_tiempoCoco;
    m_indiceApoyo = otro.m_indiceApoyo;
    m_xApoyoPrev = otro.m_xApoyoPrev;
    return *this;
}

bool NivelPuenteBoyaca::operator==(const NivelPuenteBoyaca& o) const {
    return NivelDeBatalla::operator==(o) && m_dificultad == o.m_dificultad && m_parametros == o.m_parametros &&
           m_zonasBarro == o.m_zonasBarro && casiIgual(m_sueloY, o.m_sueloY) && casiIgual(m_metaX, o.m_metaX) &&
           casiIgual(m_anchoMundo, o.m_anchoMundo) && casiIgual(m_muPrado, o.m_muPrado) &&
           casiIgual(m_invulnerable, o.m_invulnerable) && casiIgual(m_tiempoCoco, o.m_tiempoCoco) &&
           m_indiceApoyo == o.m_indiceApoyo && casiIgual(m_xApoyoPrev, o.m_xApoyoPrev);
}

void NivelPuenteBoyaca::agregarPlataforma(double xCentro, double yTope, double ancho, int variante, double amplitud,
                                          double frecuencia) {
    // Alto/ancho de cada sprite (metal, ruinas, pasto con hongos, estalactitas), algo comprimido para no tapar el suelo.
    static const double proporcion[4] = {0.54, 0.435, 0.478, 0.465};
    const double alto = ancho * proporcion[((variante % 4) + 4) % 4] * 0.7;
    ObjetoDelEscenario* p = new ObjetoDelEscenario(QPointF(xCentro, yTope + alto / 2.0), TipoObjeto::PlataformaMovil, amplitud,
                                                   frecuencia, 0, 50, QPointF(ancho, alto));
    p->setVariante(variante);
    // El sprite tiene plantas y hongos arriba: se sube/baja para que su superficie pisable quede justo en yTope.
    const double desfase = p->superficieY() - yTope;
    p->setPosicion(QPointF(xCentro, p->posicion().y() - desfase));
    p->setPosicionBase(QPointF(xCentro, p->posicion().y()));
    try {
        agregarObjetoEscenario(p);
    } catch (...) {
        delete p;
        throw;
    }
}

double NivelPuenteBoyaca::radioParaAgente(TipoEnemigo tipo) const {
    return m_parametros.radioVision * kEsc * (tipo == TipoEnemigo::EnemigoFinal ? kFactorRadioJefe : 1.0);
}

// Facil e intermedio: el cohete de Abelardo hace lo mismo que una bala de canon (1 vida). Dificil: 2 vidas por impacto.
double NivelPuenteBoyaca::danoJefe() const { return m_dificultad == Dificultad::Dificil ? 50.0 : 25.0; }

void NivelPuenteBoyaca::agregarEnemigoAgente(TipoEnemigo tipo, double x, double piesY, double xA, double xB, double rapidez) {
    AgenteInteligente* ag = new AgenteInteligente(radioParaAgente(tipo), m_parametros.retardo, m_parametros.umbral);
    EnemigoRealista* e = nullptr;
    try {
        e = new EnemigoRealista(tipo, QPointF(x, piesY), ag, QPointF(0, 0), rapidez * kEsc);
    } catch (...) {
        delete ag;
        throw;
    }
    e->setTamano(e->tamano() * kEsc);
    e->setSuelo(piesY);
    if (tipo == TipoEnemigo::EnemigoFinal) e->setDanoDisparo(danoJefe());
    const double y = piesY - e->tamano().y() / 2.0;  // los pies quedan sobre el suelo o la plataforma
    e->setPosicion(QPointF(x, y));
    e->setObjetivo(QPointF(x, y));
    ag->setPuntosPatrulla(QPointF(xA, y), QPointF(xB, y));
    try {
        agregarEnemigo(e);  // si falla, 'e' (duenio del agente) se libera aqui
    } catch (...) {
        delete e;
        throw;
    }
}

// Un coco nuevo en el suelo, un poco por delante de German (nunca se acaban).
void NivelPuenteBoyaca::aparecerCocoAleatorio() {
    QRandomGenerator* azar = QRandomGenerator::global();
    // Si German se quedo sin cocos, el nuevo aparece cerca; si no, en un tramo amplio alrededor de el.
    const bool sinCocos = m_jugador->cocos() == 0;
    double x = m_jugador->posicion().x() + (sinCocos ? -250.0 + azar->generateDouble() * 600.0 : -300.0 + azar->generateDouble() * 1000.0)  /* siempre dentro del radio "cerca" */;
    x = std::max(60.0, std::min(x, m_anchoMundo - 60.0));
    colocarCocoEnSuelo(QPointF(x, m_sueloY - 14.0));
}

// Cocos en el suelo a menos de 'radio' px de German.
int NivelPuenteBoyaca::cocosCercaDeGerman(double radio) const {
    int n = 0;
    for (const ObjetoDelEscenario* o : m_objetosEscenario)
        if (o->tipo() == TipoObjeto::CocoEnSuelo && o->estaActivo() && std::fabs(o->posicion().x() - m_jugador->posicion().x()) <= radio) ++n;
    return n;
}

void NivelPuenteBoyaca::construirEscenario() {
    m_jugador->setModo(GermanQuintero::Modo::Plataforma);
    m_jugador->setEscalaMundo(kEsc);  // German mas grande y con la fisica a su escala
    m_jugador->setPosicion(QPointF(120, m_sueloY - m_jugador->tamano().y() / 2.0));
    m_jugador->setSuelo(m_sueloY);
    crearReservaArmas(kReservaArmas);
    crearReservaCocos(kReservaCocos);
    crearReservaEfectos(kReservaEfectos);
    for (double x : {250.0, 450.0, 650.0, 850.0, 1050.0, 1250.0}) colocarCocoEnSuelo(QPointF(x, m_sueloY - 14.0));
    m_zonasBarro = {QRectF(900, m_sueloY, 380, 18), QRectF(3300, m_sueloY, 400, 18), QRectF(7550, m_sueloY, 400, 18)};

    // Power-ups: aparecen en un punto al azar de cada tramo (lejos de los pendulos).
    QRandomGenerator* azar = QRandomGenerator::global();
    auto enTramo = [azar](double a, double b) { return a + azar->generateDouble() * (b - a); };
    for (double x : {enTramo(400, 1200), enTramo(6000, 7300)})
        agregarObjetoEscenario(new ObjetoDelEscenario(QPointF(x, m_sueloY - 20), TipoObjeto::PastelDeYuca, 0, 1, 0, 50, QPointF(80, 40)));
    for (double x : {enTramo(3800, 5300), enTramo(7900, 9000)})
        agregarObjetoEscenario(new ObjetoDelEscenario(QPointF(x, m_sueloY - 37), TipoObjeto::AguaPanela, 0, 1, 0, 50, QPointF(46, 73)));
    // Pendulos colgando del borde superior. El sprite de la boleadora todavia no existe: se dibuja un cuadro de color.
    for (double x : {2850.0, 5750.0})
        agregarObjetoEscenario(new ObjetoDelEscenario(QPointF(x, m_sueloY - 360), TipoObjeto::TrampaPendulo,
                                                      m_parametros.anguloPendulo * kPi / 180.0, 1.6, 0, 300, QPointF(80, 80)));  // demoledora: cadena + bola

    // Plataformas flotando sobre la capa frontal, en cuatro cadenas separadas por tramos de suelo abierto.
    // El salto de German llega a ~180 px (300 con Cresta Veloz) y cubre ~280 px en horizontal: se sube en escalones
    // de ~95 y ~170 px, con huecos de 20 a 140 px entre plataformas.
    agregarPlataforma(1300, m_sueloY - 95, 260, 2);
    agregarPlataforma(1700, m_sueloY - 100, 240, 0, 120.0, 1.0);  // movil: arrastra a German
    agregarPlataforma(2150, m_sueloY - 170, 280, 3);
    agregarPlataforma(2550, m_sueloY - 95, 260, 1);

    agregarPlataforma(4000, m_sueloY - 95, 260, 1);
    agregarPlataforma(4400, m_sueloY - 170, 300, 3, 100.0, 0.9);  // movil
    agregarPlataforma(4850, m_sueloY - 100, 240, 0);
    agregarPlataforma(5250, m_sueloY - 175, 280, 2);

    agregarPlataforma(6000, m_sueloY - 95, 260, 1);
    agregarPlataforma(6400, m_sueloY - 170, 300, 3);
    agregarPlataforma(6850, m_sueloY - 95, 260, 0, 110.0, 1.0);   // movil
    agregarPlataforma(7250, m_sueloY - 170, 280, 2);

    agregarPlataforma(8100, m_sueloY - 100, 240, 1);
    agregarPlataforma(8500, m_sueloY - 170, 300, 3);
    agregarPlataforma(8900, m_sueloY - 95, 260, 2);

    // Canones patrulleros en el suelo (pocos y muy separados).
    for (double x : {3100.0, 5600.0, 7750.0, 9250.0})
        agregarEnemigoAgente(TipoEnemigo::Centinela, x, m_sueloY, x, x, kRapidezCentinela);
    // Dos soldados a mitad de camino y Abelardo (bazuca) muy lejos, cerca de la meta.
    agregarEnemigoAgente(TipoEnemigo::Comandante, 3600, m_sueloY, 3500, 3720, kRapidezComandante);
    agregarEnemigoAgente(TipoEnemigo::Comandante, 7450, m_sueloY, 7380, 7560, kRapidezComandante);
    agregarEnemigoAgente(TipoEnemigo::EnemigoFinal, 9650, m_sueloY, 9550, 9750, kRapidezAbelardo);
    int v = 0;
    for (EnemigoRealista* e : m_enemigos)
        if (e->tipo() == TipoEnemigo::Centinela) e->setVariante(v++);  // canones "a" y "b" alternados
}

void NivelPuenteBoyaca::aplicarDificultad() {
    for (EnemigoRealista* e : m_enemigos) {
        AgenteInteligente* a = e->agente();
        if (a == nullptr) continue;
        a->setRadioVision(radioParaAgente(e->tipo()));
        if (e->esJefeFinal()) e->setDanoDisparo(danoJefe());
        a->setRetardoReaccion(m_parametros.retardo);
        a->setUmbralAprendizaje(m_parametros.umbral);
    }
    for (ObjetoDelEscenario* o : m_objetosEscenario)
        if (o->tipo() == TipoObjeto::TrampaPendulo) o->setAmplitud(m_parametros.anguloPendulo * kPi / 180.0);
}

// Piso bajo un cuerpo: la superficie de la plataforma mas alta que tiene justo debajo, o el suelo.
double NivelPuenteBoyaca::sueloPara(const QRectF& cuerpo, int* indicePlataforma) const {
    double suelo = m_sueloY;
    int apoyo = -1;
    for (std::size_t i = 0; i < m_objetosEscenario.size(); ++i) {
        const ObjetoDelEscenario* o = m_objetosEscenario[i];
        if (o->tipo() != TipoObjeto::PlataformaMovil || !o->estaActivo()) continue;
        const QRectF p = o->limites();
        const double superficie = o->superficieY();
        if (cuerpo.right() > p.left() && cuerpo.left() < p.right() && cuerpo.bottom() <= superficie + 10.0 && superficie < suelo) {
            suelo = superficie;
            apoyo = static_cast<int>(i);
        }
    }
    if (indicePlataforma != nullptr) *indicePlataforma = apoyo;
    return suelo;
}

void NivelPuenteBoyaca::actualizarTerreno() {
    // Si German iba sobre una plataforma movil, se mueve con ella.
    if (m_indiceApoyo >= 0 && m_indiceApoyo < static_cast<int>(m_objetosEscenario.size()) && m_jugador->enSuelo()) {
        const double dx = m_objetosEscenario[static_cast<std::size_t>(m_indiceApoyo)]->posicion().x() - m_xApoyoPrev;
        m_jugador->setPosicion(QPointF(m_jugador->posicion().x() + dx, m_jugador->posicion().y()));
    }
    const QRectF lim = m_jugador->limites();
    int apoyo = -1;
    m_jugador->setSuelo(sueloPara(lim, &apoyo));
    m_indiceApoyo = apoyo;
    m_xApoyoPrev = apoyo >= 0 ? m_objetosEscenario[static_cast<std::size_t>(apoyo)]->posicion().x() : 0.0;

    const QPointF pies(m_jugador->posicion().x(), lim.bottom());
    bool barro = false;
    for (const QRectF& z : m_zonasBarro)
        if (pies.x() >= z.left() && pies.x() <= z.right() && pies.y() >= z.top() - 4.0) barro = true;
    m_jugador->setFriccion(barro ? m_parametros.muBarro : m_muPrado);

    const double x = std::max(40.0, std::min(m_jugador->posicion().x(), m_anchoMundo - 40.0));
    m_jugador->setPosicion(QPointF(x, m_jugador->posicion().y()));
}

// Abelardo: sigue el piso (suelo o plataforma), salta cuando German esta mas arriba y no sale del mapa.
void NivelPuenteBoyaca::actualizarJefes() {
    const QRectF g = m_jugador->limites();
    for (EnemigoRealista* e : m_enemigos) {
        if (!e->esJefeFinal() || !e->estaActivo()) continue;
        const QRectF b = e->limites();
        e->setSuelo(sueloPara(b, nullptr));
        const bool germanMasAlto = g.bottom() < b.bottom() - 60.0;
        const double dx = std::fabs((g.left() + g.right()) / 2.0 - (b.left() + b.right()) / 2.0);
        if (e->persiguiendo() && e->enSuelo() && germanMasAlto && dx < 450.0) e->saltar();
        const double x = std::max(60.0, std::min(e->posicion().x(), m_anchoMundo - 60.0));
        if (x != e->posicion().x()) e->setPosicion(QPointF(x, e->posicion().y()));
    }
}

void NivelPuenteBoyaca::resolverProyectiles() {
    for (Armas* a : m_armas) {
        if (!a->estaActivo()) continue;
        const QPointF p = a->posicion();
        const QRectF zona = a->areaImpacto();
        if (a->esEnemiga()) {  // bala de canon o cohete de Abelardo
            const bool golpea = zona.intersects(m_jugador->limites());
            if (golpea) m_jugador->recibirImpacto(a->dano());
            if (golpea || p.y() >= m_sueloY) {
                const bool cohete = a->tipoProyectil() == TipoProyectil::Cohete;
                lanzarEfecto(cohete ? TipoEfecto::CoheteExplosion : TipoEfecto::BalaImpacto,
                             QPointF(p.x(), std::min(p.y(), m_sueloY - 20.0)), 0.45, cohete ? 0.9 : 1.2);
                a->desactivar();
            }
            continue;
        }
        for (EnemigoRealista* e : m_enemigos) {
            if (!e->estaActivo() || !zona.intersects(e->limites())) continue;
            e->impactoRecibido(a->dano(), p);
            lanzarEfecto(TipoEfecto::CocoRompe, p, 0.4, 1.4);
            if (!e->estaActivo())
                lanzarEfecto(e->esHumanoide() ? TipoEfecto::Polvo : TipoEfecto::Explosion, e->posicion(), 0.6, kEsc);
            a->desactivar();
            break;
        }
        if (a->estaActivo() && p.y() >= m_sueloY) a->desactivar();
    }
}

void NivelPuenteBoyaca::resolverObjetos() {
    const QRectF lim = m_jugador->limites();
    for (ObjetoDelEscenario* o : m_objetosEscenario) {
        if (!o->estaActivo()) continue;
        const bool toca = lim.intersects(o->limites());
        switch (o->tipo()) {
        case TipoObjeto::TrampaPendulo:
            if (toca && m_invulnerable <= 0.0) {
                m_jugador->recibirImpacto(kDanoPendulo);
                m_invulnerable = kInvulnerabilidad;
                lanzarEfecto(TipoEfecto::Chispa, m_jugador->posicion(), 0.4, 1.4);  // chispas del golpe
            }
            break;
        case TipoObjeto::BarrilExplosivo:
            if (toca) {
                o->explotar();
                if (distancia(o->posicion(), m_jugador->posicion()) < kRadioExplosion && m_invulnerable <= 0.0) {
                    m_jugador->recibirImpacto(kDanoBarril);
                    m_invulnerable = kInvulnerabilidad;
                }
            }
            break;
        case TipoObjeto::PastelDeYuca:
            if (toca) {
                m_jugador->activarEspecialidad(GermanQuintero::Especialidad::MachoAlfa);
                o->recoger();
            }
            break;
        case TipoObjeto::AguaPanela:
            if (toca) {
                m_jugador->activarEspecialidad(GermanQuintero::Especialidad::CrestaVeloz);
                o->recoger();
            }
            break;
        case TipoObjeto::CocoEnSuelo:
            if (toca && m_jugador->recogerCoco()) o->recoger();
            break;
        default:
            break;
        }
    }
}

bool NivelPuenteBoyaca::jefeDerrotado() const {
    for (const EnemigoRealista* e : m_enemigos)
        if (e->esJefeFinal() && e->estaActivo()) return false;
    return true;
}

void NivelPuenteBoyaca::actualizar(double dt) {
    if (dt <= 0.0 || victoria() || derrota()) return;
    m_invulnerable = std::max(0.0, m_invulnerable - dt);
    actualizarTerreno();
    actualizarJefes();
    actualizarComun(dt);

    const QPointF objetivo = m_jugador->posicion();
    for (EnemigoRealista* e : m_enemigos) {
        if (!e->puedeDisparar()) continue;
        if (Armas* a = armaLibre()) e->disparar(objetivo, *a);
    }
    resolverProyectiles();
    resolverObjetos();

    // Los cocos que quedaron muy atras se reciclan: asi siempre hay cocos disponibles en todo el recorrido.
    for (ObjetoDelEscenario* o : m_objetosEscenario)
        if (o->tipo() == TipoObjeto::CocoEnSuelo && o->estaActivo() &&
            std::fabs(o->posicion().x() - m_jugador->posicion().x()) > kRadioCocosLejos)
            o->desactivar();
    // Sin cocos en la mano y casi ninguno cerca: el siguiente aparece enseguida.
    if (m_jugador->cocos() == 0 && cocosCercaDeGerman(kRadioCocosCerca) < 3) m_tiempoCoco = std::min(m_tiempoCoco, 0.25);
    m_tiempoCoco -= dt;
    if (m_tiempoCoco <= 0.0) {
        m_tiempoCoco = kIntervaloCoco;
        if (cocosCercaDeGerman(kRadioCocosCerca) < kMaxCocosCerca) aparecerCocoAleatorio();
    }
}

bool NivelPuenteBoyaca::victoria() const {
    return m_jugador->posicion().x() >= m_metaX && jefeDerrotado();  // llegar a la meta y vencer a Abelardo
}

NivelPuenteBoyaca::Dificultad NivelPuenteBoyaca::dificultad() const { return m_dificultad; }
void NivelPuenteBoyaca::setDificultad(Dificultad d) {
    m_parametros = parametrosPara(d);
    m_dificultad = d;
    aplicarDificultad();
}
NivelPuenteBoyaca::Parametros NivelPuenteBoyaca::parametros() const { return m_parametros; }
const std::vector<QRectF>& NivelPuenteBoyaca::zonasBarro() const { return m_zonasBarro; }
void NivelPuenteBoyaca::setZonasBarro(const std::vector<QRectF>& zonas) { m_zonasBarro = zonas; }
double NivelPuenteBoyaca::sueloY() const { return m_sueloY; }
void NivelPuenteBoyaca::setSueloY(double y) { m_sueloY = y; }
double NivelPuenteBoyaca::metaX() const { return m_metaX; }
void NivelPuenteBoyaca::setMetaX(double x) { m_metaX = x; }
double NivelPuenteBoyaca::anchoMundo() const { return m_anchoMundo; }
void NivelPuenteBoyaca::setAnchoMundo(double ancho) {
    if (ancho <= 0.0) throw std::invalid_argument("El ancho del mundo debe ser mayor que cero.");
    m_anchoMundo = ancho;
}
double NivelPuenteBoyaca::muPrado() const { return m_muPrado; }
void NivelPuenteBoyaca::setMuPrado(double mu) {
    if (mu < 0.0) throw std::invalid_argument("La friccion debe ser mayor o igual a cero.");
    m_muPrado = mu;
}
double NivelPuenteBoyaca::invulnerable() const { return m_invulnerable; }
void NivelPuenteBoyaca::setInvulnerable(double t) {
    if (t < 0.0) throw std::invalid_argument("El tiempo no puede ser negativo.");
    m_invulnerable = t;
}
