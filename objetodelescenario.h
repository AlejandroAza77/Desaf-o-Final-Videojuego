#ifndef OBJETODESCENARIO_H
#define OBJETODESCENARIO_H

#include "elementodebatalla.h"

// Enumeracion para los tipos de objetos del escenario
enum class TipoObjeto {
    TroncoFlotante,
    TrampaPendulo,
    PlataformaMovil,
    BarrilExplosivo,
    PastelDeYuca
};

class ObjetoDelEscenario : public ElementoDeBatalla {
private:
    TipoObjeto m_tipoObjeto;
    bool m_recogible;

    // Parametros fisicos y geometricos
    double m_amplitud;          // Amplitud A para movimiento armonico (A >= 0)
    double m_frecuencia;        // Frecuencia angular omega
    double m_fase;
    double m_longitudCuerda;
    QPointF m_posicionBase;     // Posicion central o pivote de referencia
    double m_tiempoAcumulado;   // Tiempo acumulado para calcular las ecuaciones parametricas

public:
    // Constructor con validacion estricta de parametros fisicos
    ObjetoDelEscenario(QPointF posBase, TipoObjeto tipo, double amplitud = 0.0, double frecuencia = 1.0, double fase = 0.0, double longitud = 50.0, QPointF tam = QPointF(50, 50));

    // Destructor virtual por defecto
    virtual ~ObjetoDelEscenario() = default;

    // Sobrescritura de metodos virtuales de ElementoDeBatalla
    void actualizar(double dt) override;
    QString rutaSprite() const override;

    // Metodos especificos de los objetos del escenario
    TipoObjeto tipo() const;
    void setTipo(TipoObjeto tipo);

    bool esRecogible() const;
    void setRecogible(bool recogible);

    // Acciones particulares segun el tipo de objeto
    void explotar();
    void recoger();

    // Getters y Setters de parametros fisicos
    double amplitud() const;
    void setAmplitud(double amplitud);

    double frecuencia() const;
    void setFrecuencia(double frecuencia);

    double longitudCuerda() const;
    void setLongitudCuerda(double longitud);
};

#endif // OBJETODESCENARIO_H