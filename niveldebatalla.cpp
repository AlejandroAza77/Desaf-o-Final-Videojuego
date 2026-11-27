#include "niveldebatalla.h"
#include "armas.h"
#include "enemigorealista.h"
#include "germanquintero.h"
#include "objetodelescenario.h"
#include <algorithm>
#include <memory>
#include <typeinfo>

namespace {
template <typename Contenedor>
void liberar(Contenedor& c) {
    for (auto* p : c) delete p;
    c.clear();
}

template <typename Contenedor>
bool iguales(const Contenedor& a, const Contenedor& b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                      [](const auto* x, const auto* y) { return *x == *y; });
}
}

NivelDeBatalla::NivelDeBatalla(double tiempoInicial, GermanQuintero* jugador)
    : m_tiempoRestante(tiempoInicial), m_jugador(nullptr) {
    if (tiempoInicial <= 0.0)
        throw std::invalid_argument("El tiempo inicial del nivel debe ser mayor que cero.");
    m_jugador = jugador;
}

NivelDeBatalla::NivelDeBatalla(const NivelDeBatalla& otro)
    : m_tiempoRestante(otro.m_tiempoRestante), m_jugador(nullptr) {
    copiarProfundo(otro, m_jugador, m_armas, m_objetosEscenario, m_enemigos);
}

NivelDeBatalla::~NivelDeBatalla() { liberarTodo(); }

void NivelDeBatalla::liberarTodo() {
    liberar(m_armas);
    liberar(m_objetosEscenario);
    liberar(m_enemigos);
    delete m_jugador;
    m_jugador = nullptr;
}

void NivelDeBatalla::copiarProfundo(const NivelDeBatalla& origen, GermanQuintero*& jugador,
                                    std::list<Armas*>& armas,
                                    std::vector<ObjetoDelEscenario*>& objetos,
                                    std::vector<EnemigoRealista*>& enemigos) {
    // Todo en temporales: si un new falla, no se pierde ni se filtra nada.
    GermanQuintero* j = nullptr;
    std::list<Armas*> a;
    std::vector<ObjetoDelEscenario*> o;
    std::vector<EnemigoRealista*> en;
    try {
        if (origen.m_jugador) j = new GermanQuintero(*origen.m_jugador);
        for (const Armas* x : origen.m_armas) {
            std::unique_ptr<Armas> copia(new Armas(*x));
            a.push_back(copia.get());
            copia.release();
        }
        o.reserve(origen.m_objetosEscenario.size());
        for (const ObjetoDelEscenario* x : origen.m_objetosEscenario) {
            std::unique_ptr<ObjetoDelEscenario> copia(new ObjetoDelEscenario(*x));
            o.push_back(copia.get());
            copia.release();
        }
        en.reserve(origen.m_enemigos.size());
        for (const EnemigoRealista* x : origen.m_enemigos) {
            std::unique_ptr<EnemigoRealista> copia(new EnemigoRealista(*x));
            en.push_back(copia.get());
            copia.release();
        }
    } catch (...) {
        delete j;
        liberar(a);
        liberar(o);
        liberar(en);
        throw;
    }
    jugador = j;
    armas.swap(a);
    objetos.swap(o);
    enemigos.swap(en);
}

NivelDeBatalla& NivelDeBatalla::operator=(const NivelDeBatalla& otro) {
    if (this == &otro) return *this;
    GermanQuintero* j = nullptr;
    std::list<Armas*> a;
    std::vector<ObjetoDelEscenario*> o;
    std::vector<EnemigoRealista*> en;
    copiarProfundo(otro, j, a, o, en);
    liberarTodo();
    m_tiempoRestante = otro.m_tiempoRestante;
    m_jugador = j;
    m_armas.swap(a);
    m_objetosEscenario.swap(o);
    m_enemigos.swap(en);
    return *this;
}

bool NivelDeBatalla::operator==(const NivelDeBatalla& otro) const {
    if (typeid(*this) != typeid(otro)) return false;
    if (!casiIgual(m_tiempoRestante, otro.m_tiempoRestante)) return false;
    if ((m_jugador == nullptr) != (otro.m_jugador == nullptr)) return false;
    if (m_jugador && !(*m_jugador == *otro.m_jugador)) return false;
    return iguales(m_armas, otro.m_armas) && iguales(m_objetosEscenario, otro.m_objetosEscenario) &&
           iguales(m_enemigos, otro.m_enemigos);
}

void NivelDeBatalla::actualizarComun(double dt) {
    if (dt <= 0.0) return;
    m_tiempoRestante = std::max(0.0, m_tiempoRestante - dt);
    if (m_jugador) m_jugador->actualizar(dt);
    for (Armas* a : m_armas) a->actualizar(dt);
    for (ObjetoDelEscenario* o : m_objetosEscenario) o->actualizar(dt);
    for (EnemigoRealista* e : m_enemigos) {
        if (m_jugador) e->setObjetivo(m_jugador->posicion());
        e->actualizar(dt);
    }
}

bool NivelDeBatalla::derrota() const {
    return m_tiempoRestante <= 0.0 || (m_jugador != nullptr && !m_jugador->estaActivo());
}

double NivelDeBatalla::tiempoRestante() const { return m_tiempoRestante; }

void NivelDeBatalla::setTiempoRestante(double t) {
    if (t < 0.0) throw std::invalid_argument("El tiempo restante no puede ser negativo.");
    m_tiempoRestante = t;
}

GermanQuintero* NivelDeBatalla::jugador() const { return m_jugador; }

void NivelDeBatalla::setJugador(GermanQuintero* jugador) {
    if (jugador == m_jugador) return;
    delete m_jugador;
    m_jugador = jugador;
}

const std::list<Armas*>& NivelDeBatalla::armas() const { return m_armas; }
const std::vector<ObjetoDelEscenario*>& NivelDeBatalla::objetosEscenario() const { return m_objetosEscenario; }
const std::vector<EnemigoRealista*>& NivelDeBatalla::enemigos() const { return m_enemigos; }

void NivelDeBatalla::agregarArma(Armas* nuevaArma) {
    if (nuevaArma == nullptr) throw std::invalid_argument("No se puede agregar un arma nula.");
    m_armas.push_back(nuevaArma);
}

void NivelDeBatalla::agregarObjetoEscenario(ObjetoDelEscenario* nuevoObjeto) {
    if (nuevoObjeto == nullptr) throw std::invalid_argument("No se puede agregar un objeto nulo.");
    m_objetosEscenario.push_back(nuevoObjeto);
}

void NivelDeBatalla::agregarEnemigo(EnemigoRealista* nuevoEnemigo) {
    if (nuevoEnemigo == nullptr) throw std::invalid_argument("No se puede agregar un enemigo nulo.");
    m_enemigos.push_back(nuevoEnemigo);
}

Armas* NivelDeBatalla::armaLibre() {
    for (Armas* a : m_armas)
        if (!a->estaActivo()) return a;
    return nullptr;
}

void NivelDeBatalla::llenarElementos(std::vector<ElementoDeBatalla*>& salida) const {
    salida.clear();
    salida.reserve(1 + m_armas.size() + m_objetosEscenario.size() + m_enemigos.size());
    if (m_jugador && m_jugador->estaActivo()) salida.push_back(m_jugador);
    for (Armas* a : m_armas)
        if (a->estaActivo()) salida.push_back(a);
    for (ObjetoDelEscenario* o : m_objetosEscenario)
        if (o->estaActivo()) salida.push_back(o);
    for (EnemigoRealista* e : m_enemigos)
        if (e->estaActivo()) salida.push_back(e);
}

std::vector<ElementoDeBatalla*> NivelDeBatalla::elementos() const {
    std::vector<ElementoDeBatalla*> lista;
    llenarElementos(lista);
    return lista;
}

void NivelDeBatalla::crearReservaArmas(int cantidad) {
    if (cantidad < 0) throw std::invalid_argument("La reserva de armas no puede ser negativa.");
    for (int i = 0; i < cantidad; ++i) {
        std::unique_ptr<Armas> a(new Armas(QPointF(0, 0), QPointF(0, 0), 0.0, 0, 1.0, 900.0, 0.5, TipoProyectil::Coco));
        a->desactivar();
        m_armas.push_back(a.get());
        a.release();
    }
}

bool NivelDeBatalla::lanzarCocoJugador(const QPointF& destino) {
    if (m_jugador == nullptr) return false;
    Armas* a = armaLibre();
    return a != nullptr && m_jugador->lanzarCoco(destino, *a);
}

void NivelDeBatalla::crearReservaCocos(int cantidad) {
    if (cantidad < 0) throw std::invalid_argument("La reserva de cocos no puede ser negativa.");
    for (int i = 0; i < cantidad; ++i) {
        std::unique_ptr<ObjetoDelEscenario> o(new ObjetoDelEscenario(QPointF(0, 0), TipoObjeto::CocoEnSuelo, 0, 1, 0, 50, QPointF(44, 28)));
        o->desactivar();
        m_objetosEscenario.push_back(o.get());
        o.release();
    }
}

bool NivelDeBatalla::colocarCocoEnSuelo(const QPointF& pos) {
    for (ObjetoDelEscenario* o : m_objetosEscenario) {
        if (o->tipo() != TipoObjeto::CocoEnSuelo || o->estaActivo()) continue;
        o->setActivo(true);
        o->setPosicion(pos);
        o->setPosicionBase(pos);
        return true;
    }
    return false;
}

int NivelDeBatalla::cocosEnSuelo() const {
    int n = 0;
    for (const ObjetoDelEscenario* o : m_objetosEscenario)
        if (o->tipo() == TipoObjeto::CocoEnSuelo && o->estaActivo()) ++n;
    return n;
}

void NivelDeBatalla::crearReservaEfectos(int cantidad) {
    if (cantidad < 0) throw std::invalid_argument("La reserva de efectos no puede ser negativa.");
    for (int i = 0; i < cantidad; ++i) {
        std::unique_ptr<ObjetoDelEscenario> o(new ObjetoDelEscenario(QPointF(0, 0), TipoObjeto::Efecto, 0, 1, 0, 50, QPointF(60, 40)));
        o->desactivar();
        m_objetosEscenario.push_back(o.get());
        o.release();
    }
}

bool NivelDeBatalla::lanzarEfecto(TipoEfecto efecto, const QPointF& pos, double duracion, double escala) {
    // Ancho y alto en pantalla de cada sprite (respeta su proporcion).
    static const QPointF tamanos[] = {QPointF(84, 42), QPointF(80, 48), QPointF(52, 30), QPointF(52, 29), QPointF(60, 39),
                                      QPointF(70, 35), QPointF(76, 33), QPointF(76, 35), QPointF(76, 33), QPointF(150, 148), QPointF(80, 76)};
    for (ObjetoDelEscenario* o : m_objetosEscenario) {
        if (o->tipo() != TipoObjeto::Efecto || o->estaActivo()) continue;
        o->reiniciarEfecto(efecto, pos, tamanos[static_cast<int>(efecto)] * escala, duracion);
        return true;
    }
    return false;
}
