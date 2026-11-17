#ifndef ARMAS_H
#define ARMAS_H

#include "elementodebatalla.h"
#include <stdexcept>

enum class TipoProyectil { Coco, Piedra, BalaDeCanon };
enum class ModoVista { Cenital, Lateral };

class Armas : public ElementoDeBatalla {
private:
    double m_dano;
    int m_rebotesRestantes;
    double m_tiempoVida;
    double m_g;
    double m_e;
    TipoProyectil m_tipo;
    ModoVista m_modo;
    double m_altura;
    double m_velAltura;
    bool m_aterrizo;
    QPointF m_destino;
    bool m_tieneDestino;

public:
    Armas(const QPointF& pos, const QPointF& vel, double dano, int rebotes, double tiempoVida,
          double g, double e, TipoProyectil tipo, const QPointF& tam = QPointF(25, 25),
          ModoVista modo = ModoVista::Lateral);
    Armas(const Armas&) = default;
    ~Armas() override = default;

    Armas& operator=(const Armas& otro);
    bool operator==(const Armas& otro) const;
    bool operator!=(const Armas& otro) const { return !(*this == otro); }

    void actualizar(double dt) override;
    QString rutaSprite() const override;

    void apuntar(const QPointF& origen, const QPointF& destino, double tiempoVuelo);
    void rebotar(const QPointF& normal);
    bool haExpirado() const;
    void reiniciar(const QPointF& pos, const QPointF& vel, double dano, int rebotes,
                   double tiempoVida, double g, double e, TipoProyectil tipo,
                   ModoVista modo = ModoVista::Lateral);

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
    ModoVista modo() const;
    void setModo(ModoVista modo);
    double altura() const override;
    void setAltura(double altura);
    double velocidadAltura() const;
    void setVelocidadAltura(double v);
    bool haAterrizado() const;
    void setAterrizo(bool aterrizo);
    QPointF destino() const;
    void setDestino(const QPointF& destino);
    bool tieneDestino() const;
    void setTieneDestino(bool tiene);
};

#endif // ARMAS_H