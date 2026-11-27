#ifndef NIVELRIOTEATINOS_H
#define NIVELRIOTEATINOS_H

#include "niveldebatalla.h"

// Nivel 1 (vista cenital): German en la orilla izquierda, el rio con el puente en el
// centro y los canones en la orilla derecha.
class NivelRioTeatinos : public NivelDeBatalla {
public:
    // Lo que cambia con la dificultad: ritmo de las carretas, punteria de los canones, corriente y tolerancia.
    struct Parametros {
        double rapidezCarreta;     // px/s de cada carreta
        double intervaloAparicion; // s entre carretas
        double intervaloCanon;     // s entre disparos del primer canon (los otros se escalonan)
        double anticipo;           // s que el canon adelanta su punteria (0 = apunta a donde esta German)
        double corriente;          // px/s con que el rio arrastra a German
        int maxCruzadas;           // carretas que pueden cruzar antes de perder
        bool operator==(const Parametros& o) const;
        bool operator!=(const Parametros& o) const { return !(*this == o); }
    };

private:
    Dificultad m_dificultad;
    Parametros m_parametros;
    int m_carretasObjetivo;
    int m_carretasDestruidas;
    int m_carretasCruzaron;
    int m_maxCruzadas;
    int m_carril;
    double m_intervaloAparicion;
    double m_tiempoAparicion;
    QRectF m_zonaJugador;
    QRectF m_zonaAgua;
    QPointF m_corriente;
    QPointF m_inicioCarretas;
    QPointF m_metaCarretas;
    double m_tiempoCoco;  // cuenta atras para que aparezca otro coco en la orilla

    void construirEscenario();
    void aparecerCocoAleatorio();
    void generarCarreta();
    void resolverProyectil(Armas& arma);
    void dejarCocoEnSuelo(const QPointF& pos);
    void recogerCocos();
    void aplicarDificultad();

public:
    explicit NivelRioTeatinos(GermanQuintero* jugador, Dificultad dificultad = Dificultad::Normal,
                              double tiempo = 90.0);
    NivelRioTeatinos(const NivelRioTeatinos& otro) = default;
    ~NivelRioTeatinos() override = default;

    NivelRioTeatinos& operator=(const NivelRioTeatinos& otro);
    bool operator==(const NivelRioTeatinos& otro) const;
    bool operator!=(const NivelRioTeatinos& otro) const { return !(*this == otro); }

    void actualizar(double dt) override;
    bool victoria() const override;
    bool derrota() const override;
    static Parametros parametrosPara(Dificultad dificultad);

    Dificultad dificultad() const;
    void setDificultad(Dificultad dificultad);
    Parametros parametros() const;

    int carretasObjetivo() const;
    void setCarretasObjetivo(int n);
    int carretasDestruidas() const;
    void setCarretasDestruidas(int n);
    int carretasCruzaron() const;
    void setCarretasCruzaron(int n);
    int maxCruzadas() const;
    void setMaxCruzadas(int n);
    int carril() const;
    void setCarril(int carril);
    double intervaloAparicion() const;
    void setIntervaloAparicion(double t);
    double tiempoAparicion() const;
    void setTiempoAparicion(double t);
    QRectF zonaJugador() const;
    void setZonaJugador(const QRectF& zona);
    QRectF zonaAgua() const;
    void setZonaAgua(const QRectF& zona);
    QPointF corriente() const;
    void setCorriente(const QPointF& vel);
    QPointF inicioCarretas() const;
    void setInicioCarretas(const QPointF& p);
    QPointF metaCarretas() const;
    void setMetaCarretas(const QPointF& p);
};

#endif // NIVELRIOTEATINOS_H
