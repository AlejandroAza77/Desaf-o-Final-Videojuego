#ifndef SOLDADOCENTINELA_H
#define SOLDADOCENTINELA_H

#include "elementodebatalla.h"
#include "agenteinteligente.h"
#include "armas.h"
#include <stdexcept>

// Rol del centinela en el nivel (permite usar centinela normal, comandante o enemigo final tipo Abelardo)
enum class RolCentinela {
    Centinela,
    Comandante,
    EnemigoFinal
};


class SoldadoCentinela : public ElementoDeBatalla {
private:
    AgenteInteligente* m_cerebro;
    bool m_duenoCerebro;            // Indica si este objeto creo o posee m_cerebro
    RolCentinela m_rol;
    double m_velocidadMovimiento;
    double m_cooldownDisparo;       // Tiempo minimo entre disparos en segundos
    double m_temporizadorDisparo;   // Tiempo acumulado desde el ultimo disparo
    double m_retardoAcumulado;
    double m_danoProyectil;
    double m_gravedadProyectil;
    QPointF m_objetivo;             // Posicion actual conocida del objetivo
    bool m_tieneObjetivo;
    double m_saludMaxima;

    void copiarDesde(const SoldadoCentinela& otro);
    void liberarCerebro();

public:
    SoldadoCentinela(const QPointF& pos = QPointF(0.0, 0.0),
                     double vida = 100.0,
                     RolCentinela rol = RolCentinela::Centinela,
                     double velocidadMovimiento = 135.0,
                     double cooldownDisparo = 1.6,
                     double danoProyectil = 15.0,
                     double gravedadProyectil = 900.0,
                     const QPointF& tam = QPointF(42.0, 60.0),
                     AgenteInteligente* cerebroExterno = nullptr,
                     bool tomarPropiedadCerebro = true);

    SoldadoCentinela(const SoldadoCentinela& otro);
    ~SoldadoCentinela() override;

    SoldadoCentinela& operator=(const SoldadoCentinela& otro);
    bool operator==(const SoldadoCentinela& otro) const;
    bool operator!=(const SoldadoCentinela& otro) const { return !(*this == otro); }

    // Sobrescritura de ElementoDeBatalla
    void actualizar(double dt) override;
    QString rutaSprite() const override;
    void recibirImpacto(double dano);

    // Metodos principales del centinela
    void setObjetivo(const QPointF& posObjetivo);
    Armas* disparar();
    void registrarImpactoRecibido(const QPointF& origenImpacto, double dano);

    // Getters y setters
    AgenteInteligente* cerebro() const;
    void setCerebro(AgenteInteligente* nuevoCerebro, bool tomarPropiedad = true);

    RolCentinela rol() const;
    void setRol(RolCentinela rol);

    double velocidadMovimiento() const;
    void setVelocidadMovimiento(double velocidad);

    double cooldownDisparo() const;
    void setCooldownDisparo(double cooldown);

    double temporizadorDisparo() const;
    void setTemporizadorDisparo(double tiempo);

    double danoProyectil() const;
    void setDanoProyectil(double dano);

    double gravedadProyectil() const;
    void setGravedadProyectil(double gravedad);

    QPointF objetivo() const;
    bool tieneObjetivo() const;
    void setTieneObjetivo(bool tiene);

    double saludMaxima() const;
    void setSaludMaxima(double vidaMax);
};

#endif // SOLDADOCENTINELA_H