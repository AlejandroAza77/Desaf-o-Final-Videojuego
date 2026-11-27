#ifndef ENEMIGOREALISTA_H
#define ENEMIGOREALISTA_H

#include "agenteinteligente.h"
#include "armas.h"
#include "elementodebatalla.h"
#include <stdexcept>

enum class TipoEnemigo { Carreta, Canon, Centinela, Comandante, EnemigoFinal };  // EnemigoFinal = Abelardo

// Centinela = canon patrullero; Comandante = soldado; EnemigoFinal = Abelardo (bazuca).
// Solo el centinela, el comandante y el enemigo final (Abelardo) llevan agente; la clase es duenia de ese agente.
class EnemigoRealista : public ElementoDeBatalla {
private:
    TipoEnemigo m_tipo;
    AgenteInteligente* m_agente;
    QPointF m_meta;
    double m_rapidez;
    bool m_cruzo;
    double m_intervaloDisparo;
    double m_tiempoDisparo;
    double m_saludMaxima;
    QPointF m_objetivo;
    bool m_quiereDisparar;
    double m_tAnim;  // reloj de la animacion de caminata
    int m_variante;  // 0 = version a, 1 = version b del sprite (carretas y canones)
    double m_tMuerte;  // segundos desde que fue destruido (restos)
    bool m_muerteIniciada;
    double m_danoDisparo;     // dano de cada disparo (Abelardo: 2 vidas solo en dificultad dificil)
    bool m_persiguiendo;      // Abelardo: una vez que ve a German lo persigue hasta matarlo
    double m_suelo;           // Abelardo: y del piso (o plataforma) bajo sus pies
    bool m_enSuelo;

    void avanzar(double dt);
    void pensar(double dt);

public:
    // Si lanza, el llamador sigue siendo duenio del agente.
    EnemigoRealista(TipoEnemigo tipo, const QPointF& pos, AgenteInteligente* agente = nullptr,
                    const QPointF& meta = QPointF(0, 0), double rapidez = 0.0, double intervaloDisparo = 0.0);
    EnemigoRealista(const EnemigoRealista& otro);
    ~EnemigoRealista() override;

    EnemigoRealista& operator=(const EnemigoRealista& otro);
    bool operator==(const EnemigoRealista& otro) const;
    bool operator!=(const EnemigoRealista& otro) const { return !(*this == otro); }

    void actualizar(double dt) override;
    QString rutaSprite() const override;
    bool espejar() const override;
    bool esVisible() const override;  // un enemigo destruido deja restos un tiempo
    double opacidad() const override;
    bool libre() const;
    void saltar();  // solo Abelardo: salta si esta en el suelo
    bool persiguiendo() const;
    void setPersiguiendo(bool p);
    double suelo() const;
    void setSuelo(double sueloY);
    bool enSuelo() const;
    double danoDisparo() const;
    void setDanoDisparo(double dano);  // no se ve y se puede reutilizar (carretas)
    int variante() const;
    void setVariante(int variante);
    bool esHumanoide() const;  // comandante y Abelardo: sprites de varias poses
    bool puedeDisparar() const;
    bool disparar(const QPointF& objetivo, Armas& proyectil);
    void impactoRecibido(double dano, const QPointF& origen);
    void reiniciar(const QPointF& pos, const QPointF& meta, double rapidez, double vida);
    double saludRelativa() const;
    bool esMovil() const;
    bool esComandante() const;
    bool esJefeFinal() const;

    TipoEnemigo tipo() const;
    void setTipo(TipoEnemigo tipo);
    AgenteInteligente* agente() const;
    void setAgente(AgenteInteligente* agente);
    QPointF meta() const;
    void setMeta(const QPointF& meta);
    double rapidez() const;
    void setRapidez(double rapidez);
    bool cruzoElPuente() const;
    void setCruzo(bool cruzo);
    double intervaloDisparo() const;
    void setIntervaloDisparo(double t);
    double tiempoDisparo() const;
    void setTiempoDisparo(double t);
    double saludMaxima() const;
    void setSaludMaxima(double salud);
    QPointF objetivo() const;
    void setObjetivo(const QPointF& objetivo);
    bool quiereDisparar() const;
    void setQuiereDisparar(bool quiere);
};

#endif // ENEMIGOREALISTA_H
