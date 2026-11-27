#ifndef NIVELPUENTEBOYACA_H
#define NIVELPUENTEBOYACA_H

#include <vector>
#include "enemigorealista.h"
#include "niveldebatalla.h"

// Nivel 2 (vista lateral con scroll, recorrido largo de ~7000 px). La dificultad es un enum con sus parametros,
// no una clase: solo cambia numeros del agente, del barro y del pendulo.
class NivelPuenteBoyaca : public NivelDeBatalla {
public:
    struct Parametros {
        double radioVision;
        double retardo;
        int umbral;
        double muBarro;
        double anguloPendulo;
        bool operator==(const Parametros& o) const;
        bool operator!=(const Parametros& o) const { return !(*this == o); }
    };

private:
    Dificultad m_dificultad;
    Parametros m_parametros;
    std::vector<QRectF> m_zonasBarro;
    double m_sueloY;
    double m_metaX;
    double m_anchoMundo;
    double m_muPrado;
    double m_invulnerable;
    double m_tiempoCoco;  // cuenta atras para que aparezca otro coco
    int m_indiceApoyo;    // plataforma (indice en los objetos) sobre la que esta German; -1 = ninguna
    double m_xApoyoPrev;  // x de esa plataforma en el cuadro anterior: sirve para arrastrar a German

    void construirEscenario();
    void agregarPlataforma(double xCentro, double yTope, double ancho, int variante, double amplitud = 0.0,
                           double frecuencia = 1.0);
    void agregarEnemigoAgente(TipoEnemigo tipo, double x, double piesY, double xA, double xB, double rapidez);
    void aparecerCocoAleatorio();
    int cocosCercaDeGerman(double radio) const;
    double sueloPara(const QRectF& cuerpo, int* indicePlataforma) const;  // piso bajo un cuerpo: suelo o plataforma
    void actualizarJefes();                                                // gravedad, saltos y limites de Abelardo
    double radioParaAgente(TipoEnemigo tipo) const;
    double danoJefe() const;                                               // dano del cohete segun la dificultad
    void aplicarDificultad();
    void actualizarTerreno();
    void resolverProyectiles();
    void resolverObjetos();
    bool jefeDerrotado() const;

public:
    explicit NivelPuenteBoyaca(GermanQuintero* jugador, Dificultad dificultad = Dificultad::Normal,
                               double tiempo = 150.0);
    NivelPuenteBoyaca(const NivelPuenteBoyaca& otro) = default;
    ~NivelPuenteBoyaca() override = default;

    NivelPuenteBoyaca& operator=(const NivelPuenteBoyaca& otro);
    bool operator==(const NivelPuenteBoyaca& otro) const;
    bool operator!=(const NivelPuenteBoyaca& otro) const { return !(*this == otro); }

    void actualizar(double dt) override;
    bool victoria() const override;
    static Parametros parametrosPara(Dificultad dificultad);
    static constexpr double kEscala = 1.6;  // todo el nivel (personajes, cajas, plataformas, fisica) es 1.6 veces mas grande

    Dificultad dificultad() const;
    void setDificultad(Dificultad dificultad);
    Parametros parametros() const;
    const std::vector<QRectF>& zonasBarro() const;
    void setZonasBarro(const std::vector<QRectF>& zonas);
    double sueloY() const;
    void setSueloY(double y);
    double metaX() const;
    void setMetaX(double x);
    double anchoMundo() const;
    void setAnchoMundo(double ancho);
    double muPrado() const;
    void setMuPrado(double mu);
    double invulnerable() const;
    void setInvulnerable(double t);
};

#endif // NIVELPUENTEBOYACA_H
