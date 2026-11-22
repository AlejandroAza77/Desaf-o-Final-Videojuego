#ifndef NIVELRIOTEATINOS_H
#define NIVELRIOTEATINOS_H

#include "niveldebatalla.h"
#include "carretadecanon.h"
#include <vector>


class NivelRioTeatinos : public NivelDeBatalla {
private:
    std::vector<CarretaDeCanon*> m_carretas; // Coleccion de carretas del nivel (hasta 10)
    int m_carretasTotales;
    int m_carretasDestruidas;
    int m_carretasGeneradas;
    double m_intervaloAparicion;
    double m_temporizadorAparicion;         // Tiempo acumulado para generar la siguiente carreta
    QPointF m_corrienteRio;                 // Vector de velocidad de la corriente del rio
    QPointF m_inicioPuente;
    QPointF m_finPuente;
    bool m_algunaCruzoPuente;               // Marca si alguna carreta logro cruzar el puente

    void liberarCarretas();
    void copiarCarretas(const std::vector<CarretaDeCanon*>& origen,
                        std::vector<CarretaDeCanon*>& destino);
    void generarCarretaSiCorresponde(double dt);
    void configurarCorrienteJugador();

public:
    explicit NivelRioTeatinos(double tiempoInicial = 90.0,
                              GermanQuintero* jugador = nullptr,
                              int carretasTotales = 10,
                              double intervaloAparicion = 6.0,
                              const QPointF& corrienteRio = QPointF(45.0, 0.0),
                              const QPointF& inicioPuente = QPointF(180.0, 140.0),
                              const QPointF& finPuente = QPointF(760.0, 140.0));

    NivelRioTeatinos(const NivelRioTeatinos& otro);
    ~NivelRioTeatinos() override;

    NivelRioTeatinos& operator=(const NivelRioTeatinos& otro);
    bool operator==(const NivelRioTeatinos& otro) const;
    bool operator!=(const NivelRioTeatinos& otro) const { return !(*this == otro); }

    // Sobrescritura de NivelDeBatalla
    void actualizar(double dt) override;
    bool victoria() const override;
    bool derrota() const override;

    // Gestion de carretas y registro de destruccion
    void agregarCarreta(CarretaDeCanon* nuevaCarreta);
    void registrarCarretaDestruida(CarretaDeCanon* carreta = nullptr);
    const std::vector<CarretaDeCanon*>& carretas() const;

    // Getters y setters
    int carretasTotales() const;
    void setCarretasTotales(int total);

    int carretasDestruidas() const;
    void setCarretasDestruidas(int destruidas);

    int carretasGeneradas() const;
    double intervaloAparicion() const;
    void setIntervaloAparicion(double intervalo);

    QPointF corrienteRio() const;
    void setCorrienteRio(const QPointF& corriente);

    QPointF inicioPuente() const;
    void setInicioPuente(const QPointF& inicio);

    QPointF finPuente() const;
    void setFinPuente(const QPointF& fin);

    bool algunaCruzoPuente() const;
};

#endif // NIVELRIOTEATINOS_H