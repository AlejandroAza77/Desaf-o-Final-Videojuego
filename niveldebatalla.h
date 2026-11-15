#ifndef NIVELDEBATALLA_H
#define NIVELDEBATALLA_H

#include <list>
#include <vector>
#include "elementodebatalla.h"

// Declaraciones adelantadas para evitar dependencias circulares
class GermanQuintero;
class Arma;
class ObjetoDelEscenario;

//Clase abstracta NivelDeBatalla

class NivelDeBatalla {
protected:
    double m_tiempoRestante;
    GermanQuintero* m_jugador;
    std::list<Arma*> m_armas;
    std::vector<ObjetoDelEscenario*> m_objetosEscenario;

public:
    NivelDeBatalla(double tiempoInicial = 90.0, GermanQuintero* jugador = nullptr);

    // Destructor virtual para liberar toda la memoria dinamica
    virtual ~NivelDeBatalla();

    virtual void actualizar(double dt) = 0;
    virtual bool victoria() const = 0;
    virtual bool derrota() const; // Se acaba el tiempo o muere German

    double tiempoRestante() const;

    // Retorna todos los elementos activos
    std::vector<ElementoDeBatalla*> elementos() const;

    GermanQuintero* jugador() const;
    void agregarArma(Arma* nuevaArma);
    void agregarObjetoEscenario(ObjetoDelEscenario* nuevoObjeto);
};

#endif // NIVELDEBATALLA_H