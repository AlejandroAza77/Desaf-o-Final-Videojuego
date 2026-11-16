#ifndef ARMAS_H
#define ARMAS_H

#include "elementodebatalla.h"
#include <stdexcept>

enum class TipoProyectil {
    Coco,
    Piedra,
    BalaDeCanon
};

class Armas : public ElementoDeBatalla {
private:
    double m_dano;
    int m_rebotesRestantes;     // Numero de rebotes permitidos antes de destruirse
    double m_tiempoVida;
    double m_g;                 // Aceleracion de la gravedad (g > 0)
    double m_e;                 // Coeficiente de restitucion (0 <= e <= 1)
    TipoProyectil m_tipo;
    double m_tiempoAcumulado;

public:
    // Constructor con validacion de parametros fisicos
    Armas(QPointF pos, QPointF vel, double dano, int rebotes, double tiempoVida, double g, double e, TipoProyectil tipo, QPointF tam = QPointF(25, 25));

    // Destructor virtual por defecto
    virtual ~Armas() = default;

    // Sobrescritura de metodos virtuales de elementodebatalla
    void actualizar(double dt) override;
    QString rutaSprite() const override;

    // Metodos especificos de Armas
    void rebotar(const QPointF& normal);
    bool haExpirado() const;
    void reiniciar(QPointF pos, QPointF vel, double dano, int rebotes, double tiempoVida, double g, double e, TipoProyectil tipo);

    // Getters y Setters c
    double dano() const;
    void setDano(double dano);

    int rebotesRestantes() const;
    void setRebotesRestantes(int rebotes);

    double tiempoVida() const;
    void setTiempoVida(double tiempoVida);

    double gravedad() const;
    void setGravedad(double g);

    double coeficienteRestitucion() const;
    void setCoeficienteRestitucion(double e);

    TipoProyectil tipoProyectil() const;
    void setTipoProyectil(TipoProyectil tipo);
};

#endif // ARMAS_H