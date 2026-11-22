#ifndef CARRETADECANON_H
#define CARRETADECANON_H

#include "elementodebatalla.h"
#include "armas.h"
#include <stdexcept>

// Enemigo del Nivel 1 sin inteligencia artificial.
class CarretaDeCanon : public ElementoDeBatalla {
private:
    QPointF m_puntoInicio;          // Extremo inicial del puente
    QPointF m_puntoFin;             // Extremo final del puente
    double m_velocidadPuente;
    int m_golpesMaximos;
    int m_golpesRecibidos;
    double m_danoPorGolpe;
    double m_intervaloDisparo;
    double m_temporizadorDisparo;   // Tiempo acumulado desde el ultimo disparo
    double m_danoBala;
    double m_gravedadBala;
    bool m_cruzoPuente;

    static void validarConfiguracion(double velocidadPuente,
                                     int golpesMaximos,
                                     double intervaloDisparo,
                                     double gravedadBala);
    void sincronizarIntegridad();

public:
    CarretaDeCanon(const QPointF& puntoInicio,
                   const QPointF& puntoFin,
                   double velocidadPuente = 65.0,
                   int golpesMaximos = 3,
                   double intervaloDisparo = 2.5,
                   double danoPorGolpe = 1.0,
                   double danoBala = 20.0,
                   double gravedadBala = 900.0,
                   const QPointF& tam = QPointF(70, 50));

    CarretaDeCanon(const CarretaDeCanon& otro) = default;
    ~CarretaDeCanon() override = default;


    CarretaDeCanon& operator=(const CarretaDeCanon& otro);
    bool operator==(const CarretaDeCanon& otro) const;
    bool operator!=(const CarretaDeCanon& otro) const { return !(*this == otro); }

    // Sobrescritura de ElementoDeBatalla
    void actualizar(double dt) override;
    QString rutaSprite() const override;

    // Firmas fijas requeridas
    Armas* disparar();
    bool cruzoElPuente() const;

    // Registro de impacto adicional para contabilizar golpes (N golpes)
    void registrarGolpe(double dano = -1.0);

    // Getters y setters
    QPointF puntoInicio() const;
    void setPuntoInicio(const QPointF& inicio);

    QPointF puntoFin() const;
    void setPuntoFin(const QPointF& fin);

    double velocidadPuente() const;
    void setVelocidadPuente(double velocidad);

    int golpesMaximos() const;
    void setGolpesMaximos(int golpes);

    int golpesRecibidos() const;
    void setGolpesRecibidos(int golpes);

    double intervaloDisparo() const;
    void setIntervaloDisparo(double intervalo);

    double temporizadorDisparo() const;
    void setTemporizadorDisparo(double tiempo);

    double danoPorGolpe() const;
    void setDanoPorGolpe(double dano);

    double danoBala() const;
    void setDanoBala(double dano);

    double gravedadBala() const;
    void setGravedadBala(double gravedad);

    void setCruzoPuente(bool cruzo);
};

#endif // CARRETADECANON_H