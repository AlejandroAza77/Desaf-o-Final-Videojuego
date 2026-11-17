#ifndef NIVELDEBATALLA_H
#define NIVELDEBATALLA_H

#include <list>
#include <stdexcept>
#include <vector>
#include "elementodebatalla.h"

class GermanQuintero;
class Armas;
class ObjetoDelEscenario;

// El nivel es dueno de todo lo que contiene y lo libera en su destructor.
class NivelDeBatalla {
protected:
    double m_tiempoRestante;
    GermanQuintero* m_jugador;
    std::list<Armas*> m_armas;
    std::vector<ObjetoDelEscenario*> m_objetosEscenario;

    void actualizarComun(double dt);

    // protegidos: clase abstracta. La copia es profunda.
    NivelDeBatalla(const NivelDeBatalla& otro);
    NivelDeBatalla& operator=(const NivelDeBatalla& otro);

public:
    // Si lanza, el llamador sigue siendo dueno de 'jugador'.
    explicit NivelDeBatalla(double tiempoInicial = 90.0, GermanQuintero* jugador = nullptr);
    virtual ~NivelDeBatalla();

    bool operator==(const NivelDeBatalla& otro) const;
    bool operator!=(const NivelDeBatalla& otro) const { return !(*this == otro); }

    virtual void actualizar(double dt) = 0;
    virtual bool victoria() const = 0;
    virtual bool derrota() const;

    double tiempoRestante() const;
    void setTiempoRestante(double t);
    GermanQuintero* jugador() const;
    void setJugador(GermanQuintero* jugador);
    const std::list<Armas*>& armas() const;
    const std::vector<ObjetoDelEscenario*>& objetosEscenario() const;

    void agregarArma(Armas* nuevaArma);
    void agregarObjetoEscenario(ObjetoDelEscenario* nuevoObjeto);
    Armas* armaLibre();

    // Reutiliza la memoria del vector: sin reservas nuevas en el bucle del juego.
    void llenarElementos(std::vector<ElementoDeBatalla*>& salida) const;
    std::vector<ElementoDeBatalla*> elementos() const;

private:
    static void copiarProfundo(const NivelDeBatalla& origen, GermanQuintero*& jugador,
                               std::list<Armas*>& armas, std::vector<ObjetoDelEscenario*>& objetos);
    void liberarTodo();
};

#endif // NIVELDEBATALLA_H