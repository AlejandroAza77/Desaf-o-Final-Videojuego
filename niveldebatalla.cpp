#include "niveldebatalla.h"
#include "armas.h"
#include "objetodelescenario.h"

NivelDeBatalla::NivelDeBatalla(double tiempoInicial, GermanQuintero* jugador)
    : m_tiempoRestante(tiempoInicial), m_jugador(jugador) {}

NivelDeBatalla::~NivelDeBatalla() {
    // Libera la memoria dinamica de las armas activas
    for (Armas* armas : m_armas) {
        delete armas;
    }
    m_armas.clear();

    // Libera la memoria dinamica de los objetos del escenario
    for (ObjetoDelEscenario* obj : m_objetosEscenario) {
        delete obj;
    }
    m_objetosEscenario.clear();

    // Si el nivel es dueno del jugador, se libera de forma segura
    if (m_jugador != nullptr) {
        delete m_jugador;
        m_jugador = nullptr;
    }
}

bool NivelDeBatalla::derrota() const {
    // Derrota si el tiempo llega a cero o si el jugador ya no esta activo (muerto)
    if (m_tiempoRestante <= 0.0) {
        return true;
    }
    if (m_jugador != nullptr && !m_jugador->estaActivo()) {
        return true;
    }
    return false;
}

double NivelDeBatalla::tiempoRestante() const {
    return m_tiempoRestante;
}

std::vector<ElementoDeBatalla*> NivelDeBatalla::elementos() const {
    std::vector<ElementoDeBatalla*> listaActiva;

    // Anade al jugador si esta activo
    if (m_jugador != nullptr && m_jugador->estaActivo()) {
        listaActiva.push_back(m_jugador);
    }

    // Anade todas las armas activas
    for (Armas* armas: m_armas) {
        if (armas != nullptr && armas->estaActivo()) {
            listaActiva.push_back(armas);
        }
    }

    // Anade todos los objetos del escenario activos
    for (ObjetoDelEscenario* obj : m_objetosEscenario) {
        if (obj != nullptr && obj->estaActivo()) {
            listaActiva.push_back(obj);
        }
    }

    return listaActiva;
}

GermanQuintero* NivelDeBatalla::jugador() const {
    return m_jugador;
}

void NivelDeBatalla::agregarArma(Armas* nuevaArma) {
    if (nuevaArma != nullptr) {
        m_armas.push_back(nuevaArma);
    }
}

void NivelDeBatalla::agregarObjetoEscenario(ObjetoDelEscenario* nuevoObjeto) {
    if (nuevoObjeto != nullptr) {
        m_objetosEscenario.push_back(nuevoObjeto);
    }
}