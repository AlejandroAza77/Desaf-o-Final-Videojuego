#ifndef AGENTEINTELIGENTE_H
#define AGENTEINTELIGENTE_H

#include "elementodebatalla.h"
#include <stdexcept>
#include <vector>

// Estados posibles del agente segun el razonamiento
enum class EstadoAgente { Patrulla, Ataque, Retirada };

// Resultado de la percepcion del agente
struct Estimulo {
    double distanciaCuadrada = 0.0; // d^2 entre agente y jugador
    bool oyeJugador = false;        // true si esta dentro del radio auditivo
    double saludRelativa = 1.0;     // valor entre 0.0 y 1.0
    QPointF posAgente;
    QPointF posJugador;

    bool operator==(const Estimulo& o) const {
        return casiIgual(distanciaCuadrada, o.distanciaCuadrada) && oyeJugador == o.oyeJugador &&
               casiIgual(saludRelativa, o.saludRelativa) && posAgente == o.posAgente && posJugador == o.posJugador;
    }
    bool operator!=(const Estimulo& o) const { return !(*this == o); }
};

// Impactos recibidos desde un sector angular y una banda de distancia
struct RegistroImpacto {
    int sectorAngulo = 0;
    int bandaDistancia = 0;
    int conteoImpactos = 0;

    bool operator==(const RegistroImpacto& o) const {
        return sectorAngulo == o.sectorAngulo && bandaDistancia == o.bandaDistancia &&
               conteoImpactos == o.conteoImpactos;
    }
    bool operator!=(const RegistroImpacto& o) const { return !(*this == o); }
};

// Cerebro autonomo del centinela y del comandante: percibir, razonar, actuar y aprender.
class AgenteInteligente {
private:
    double m_radioVision;             // radio de vision R (se compara d^2 con R^2)
    double m_radioAudicion;           // campo de proximidad auditiva
    double m_retardoReaccion;         // segundos entre decisiones (> 0)
    int m_umbralAprendizaje;          // impactos desde un mismo sector/distancia para adaptarse (> 0)
    QPointF m_puntoA;                 // extremos de la patrulla
    QPointF m_puntoB;
    double m_tiempoReaccion;          // tiempo acumulado desde la ultima decision

    bool m_coberturaActiva;
    int m_ultimoSectorCobertura;
    QPointF m_origenUltimoImpacto;    // desde donde le dispararon cuando aprendio
    QPointF m_direccionPatrulla;
    std::vector<RegistroImpacto> m_memoriaImpactos;

    static void validarParametros(double radioVision, double retardoReaccion, int umbralAprendizaje);
    int calcularSectorAngulo(const QPointF& posAgente, const QPointF& origenImpacto) const;
    int calcularBandaDistancia(double distanciaSq) const;
    void activarCobertura(int sector, const QPointF& origenImpacto);

public:
    AgenteInteligente(double radioVision = 250.0, double retardoReaccion = 0.4, int umbralAprendizaje = 3,
                      const QPointF& puntoA = QPointF(0.0, 0.0), const QPointF& puntoB = QPointF(0.0, 0.0),
                      double radioAudicion = 350.0);
    AgenteInteligente(const AgenteInteligente& otro) = default;
    ~AgenteInteligente() = default;

    AgenteInteligente& operator=(const AgenteInteligente& otro);
    bool operator==(const AgenteInteligente& otro) const;
    bool operator!=(const AgenteInteligente& otro) const { return !(*this == otro); }

    // 1. Percepcion: d^2 sin raiz cuadrada, audicion y salud relativa
    Estimulo percibir(const QPointF& posAgente, const QPointF& posJugador, double saludRelativa) const;
    // 2. Razonamiento: retirada si salud < 20 %, ataque si d^2 <= R^2, si no patrulla
    EstadoAgente razonar(const Estimulo& estimulo) const;
    // 3. Accion: direccion normalizada segun el estado (patrulla entre A y B, ataque, retirada)
    QPointF actuar(EstadoAgente estado, const Estimulo& estimulo);
    // 4. Aprendizaje: true cuando el umbral se cumple por primera vez en un sector/distancia
    bool aprender(const QPointF& posAgente, const QPointF& origenImpacto);

    // Avanza el reloj de reaccion; true cuando ya puede tomar una nueva decision.
    bool actualizar(double dt);
    // Punto de cobertura: el extremo de la patrulla mas lejano al origen del ultimo impacto.
    QPointF coberturaSugerida(const QPointF& posActual) const;

    double radioVision() const;
    void setRadioVision(double radio);
    double radioAudicion() const;
    void setRadioAudicion(double radio);
    double retardoReaccion() const;
    void setRetardoReaccion(double retardo);
    int umbralAprendizaje() const;
    void setUmbralAprendizaje(int umbral);
    QPointF puntoA() const;
    QPointF puntoB() const;
    void setPuntosPatrulla(const QPointF& a, const QPointF& b);
    bool coberturaActiva() const;
    void setCoberturaActiva(bool activa);
    int ultimoSectorCobertura() const;
    QPointF direccionPatrulla() const;
    void setDireccionPatrulla(const QPointF& dir);
    const std::vector<RegistroImpacto>& memoriaImpactos() const;
    void limpiarMemoria();
};

#endif // AGENTEINTELIGENTE_H
