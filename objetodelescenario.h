#ifndef OBJETODELESCENARIO_H
#define OBJETODELESCENARIO_H

#include "elementodebatalla.h"
#include <stdexcept>

enum class TipoObjeto {
    TroncoFlotante, TrampaPendulo, PlataformaMovil, BarrilExplosivo, PastelDeYuca,
    Roca, CocoEnSuelo, AguaPanela,
    Efecto  // decoracion temporal: crater, impacto, explosion, salpicadura (ver TipoEfecto)
};

// Cual de los sprites de efecto se muestra (se guarda en la "variante" del objeto).
enum class TipoEfecto {
    BalaImpacto, Explosion, CocoRompe, CocoAgua, Humo, Polvo,
    CraterPasto, CraterTierra, CraterPiedra, CoheteExplosion, Chispa  // Chispa: golpe de la demoledora
};

class ObjetoDelEscenario : public ElementoDeBatalla {
private:
    TipoObjeto m_tipoObjeto;
    bool m_recogible;
    bool m_estatico;
    bool m_oscilaEnX;
    double m_amplitud;  // pendulo: angulo en radianes; resto: distancia en px
    double m_frecuencia;
    double m_fase;
    double m_longitudCuerda;
    QPointF m_posicionBase;
    double m_tiempoAcumulado;
    int m_variante;  // que version del sprite usa (plataformas, cajas, rocas, troncos, efectos)
    double m_duracion;  // efectos: segundos que viven (0 = para siempre)

    void configurarSegunTipo();

public:
    ObjetoDelEscenario(const QPointF& posBase, TipoObjeto tipo, double amplitud = 0.0,
                       double frecuencia = 1.0, double fase = 0.0, double longitud = 50.0,
                       const QPointF& tam = QPointF(50, 50), bool oscilaEnX = true);
    ObjetoDelEscenario(const ObjetoDelEscenario&) = default;
    ~ObjetoDelEscenario() override = default;

    ObjetoDelEscenario& operator=(const ObjetoDelEscenario& otro);
    bool operator==(const ObjetoDelEscenario& otro) const;
    bool operator!=(const ObjetoDelEscenario& otro) const { return !(*this == otro); }

    void actualizar(double dt) override;
    QString rutaSprite() const override;
    double opacidad() const override;
    double superficieY() const;  // plataformas: y de la superficie donde se pisa (el sprite tiene plantas encima)
    void reiniciarEfecto(TipoEfecto efecto, const QPointF& pos, const QPointF& tam, double duracion);
    void explotar();
    void recoger();

    TipoObjeto tipo() const;
    void setTipo(TipoObjeto tipo);
    bool esRecogible() const;
    void setRecogible(bool recogible);
    bool esEstatico() const;
    void setEstatico(bool estatico);
    bool oscilaEnX() const;
    void setOscilaEnX(bool oscilaEnX);
    double amplitud() const;
    void setAmplitud(double amplitud);
    double frecuencia() const;
    void setFrecuencia(double frecuencia);
    double fase() const;
    void setFase(double fase);
    double longitudCuerda() const;
    void setLongitudCuerda(double longitud);
    QPointF posicionBase() const;
    void setPosicionBase(const QPointF& pos);
    double tiempoAcumulado() const;
    void setTiempoAcumulado(double t);
    int variante() const;
    void setVariante(int variante);
    double duracion() const;
};

#endif // OBJETODELESCENARIO_H