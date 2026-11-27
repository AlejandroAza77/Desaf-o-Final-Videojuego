#ifndef NIVELDEBATALLA_H
#define NIVELDEBATALLA_H

#include <list>
#include <stdexcept>
#include <vector>
#include "elementodebatalla.h"
#include "objetodelescenario.h"

class GermanQuintero;
class Armas;
class ObjetoDelEscenario;
class EnemigoRealista;

// El nivel es dueno de todo lo que contiene y lo libera en su destructor.
class NivelDeBatalla {
protected:
    double m_tiempoRestante;
    GermanQuintero* m_jugador;
    std::list<Armas*> m_armas;
    std::vector<ObjetoDelEscenario*> m_objetosEscenario;
    std::vector<EnemigoRealista*> m_enemigos;

    void actualizarComun(double dt);
    void crearReservaArmas(int cantidad);
    void crearReservaEfectos(int cantidad);  // objetos Efecto desactivados (craters, impactos, explosiones)
    void crearReservaCocos(int cantidad);  // cocos en el suelo desactivados, listos para aparecer

    // protegidos: clase abstracta. La copia es profunda.
    NivelDeBatalla(const NivelDeBatalla& otro);
    NivelDeBatalla& operator=(const NivelDeBatalla& otro);

public:
    // Dificultad compartida por los dos niveles; cada nivel define que numeros cambia (ver Parametros).
    enum class Dificultad { Facil, Normal, Dificil };

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
    const std::vector<EnemigoRealista*>& enemigos() const;

    void agregarArma(Armas* nuevaArma);
    void agregarObjetoEscenario(ObjetoDelEscenario* nuevoObjeto);
    void agregarEnemigo(EnemigoRealista* nuevoEnemigo);
    Armas* armaLibre();
    bool lanzarCocoJugador(const QPointF& destino);
    bool colocarCocoEnSuelo(const QPointF& pos);  // reactiva un coco de la reserva; false si no queda ninguno
    int cocosEnSuelo() const;
    // Muestra un efecto temporal; si no queda ninguno libre no hace nada. escala agranda el sprite.
    bool lanzarEfecto(TipoEfecto efecto, const QPointF& pos, double duracion, double escala = 1.0);

    // Reutiliza la memoria del vector: sin reservas nuevas en el bucle del juego.
    void llenarElementos(std::vector<ElementoDeBatalla*>& salida) const;
    std::vector<ElementoDeBatalla*> elementos() const;

private:
    static void copiarProfundo(const NivelDeBatalla& origen, GermanQuintero*& jugador, std::list<Armas*>& armas,
                               std::vector<ObjetoDelEscenario*>& objetos, std::vector<EnemigoRealista*>& enemigos);
    void liberarTodo();
};

#endif // NIVELDEBATALLA_H
