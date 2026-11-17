#include "niveldebatalla.h"
#include "armas.h"
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
    copiarProfundo(otro, m_jugador, m_armas, m_objetosEscenario);
}

NivelDeBatalla::~NivelDeBatalla() { liberarTodo(); }

void NivelDeBatalla::liberarTodo() {
    liberar(m_armas);
    liberar(m_objetosEscenario);
    delete m_jugador;
    m_jugador = nullptr;
}

void NivelDeBatalla::copiarProfundo(const NivelDeBatalla& origen, GermanQuintero*& jugador,
                                    std::list<Armas*>& armas,
                                    std::vector<ObjetoDelEscenario*>& objetos) {
    // Todo en temporales: si un new falla, no se pierde ni se filtra nada.
    GermanQuintero* j = nullptr;
    std::list<Armas*> a;
    std::vector<ObjetoDelEscenario*> o;
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
    } catch (...) {
        delete j;
        liberar(a);
        liberar(o);
        throw;
    }
    jugador = j;
    armas.swap(a);
    objetos.swap(o);
}

NivelDeBatalla& NivelDeBatalla::operator=(const NivelDeBatalla& otro) {
    if (this == &otro) return *this;
    GermanQuintero* j = nullptr;
    std::list<Armas*> a;
    std::vector<ObjetoDelEscenario*> o;
    copiarProfundo(otro, j, a, o);
    liberarTodo();
    m_tiempoRestante = otro.m_tiempoRestante;
    m_jugador = j;
    m_armas.swap(a);
    m_objetosEscenario.swap(o);
    return *this;
}

bool NivelDeBatalla::operator==(const NivelDeBatalla& otro) const {
    if (typeid(*this) != typeid(otro)) return false;
    if (!casiIgual(m_tiempoRestante, otro.m_tiempoRestante)) return false;
    if ((m_jugador == nullptr) != (otro.m_jugador == nullptr)) return false;
    if (m_jugador && !(*m_jugador == *otro.m_jugador)) return false;
    return iguales(m_armas, otro.m_armas) && iguales(m_objetosEscenario, otro.m_objetosEscenario);
}

void NivelDeBatalla::actualizarComun(double dt) {
    if (dt <= 0.0) return;
    m_tiempoRestante = std::max(0.0, m_tiempoRestante - dt);
    if (m_jugador) m_jugador->actualizar(dt);
    for (Armas* a : m_armas) a->actualizar(dt);
    for (ObjetoDelEscenario* o : m_objetosEscenario) o->actualizar(dt);
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

void NivelDeBatalla::agregarArma(Armas* nuevaArma) {
    if (nuevaArma == nullptr) throw std::invalid_argument("No se puede agregar un arma nula.");
    m_armas.push_back(nuevaArma);
}

void NivelDeBatalla::agregarObjetoEscenario(ObjetoDelEscenario* nuevoObjeto) {
    if (nuevoObjeto == nullptr) throw std::invalid_argument("No se puede agregar un objeto nulo.");
    m_objetosEscenario.push_back(nuevoObjeto);
}

Armas* NivelDeBatalla::armaLibre() {
    for (Armas* a : m_armas)
        if (!a->estaActivo()) return a;
    return nullptr;
}

void NivelDeBatalla::llenarElementos(std::vector<ElementoDeBatalla*>& salida) const {
    salida.clear();
    salida.reserve(1 + m_armas.size() + m_objetosEscenario.size());
    if (m_jugador && m_jugador->estaActivo()) salida.push_back(m_jugador);
    for (Armas* a : m_armas)
        if (a->estaActivo()) salida.push_back(a);
    for (ObjetoDelEscenario* o : m_objetosEscenario)
        if (o->estaActivo()) salida.push_back(o);
}

std::vector<ElementoDeBatalla*> NivelDeBatalla::elementos() const {
    std::vector<ElementoDeBatalla*> lista;
    llenarElementos(lista);
    return lista;
}