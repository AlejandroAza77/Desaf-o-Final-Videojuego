#ifndef AGENTEINTELIGENTE_H
#define AGENTEINTELIGENTE_H

#include <QPointF>
#include <stdexcept>
#include <vector>

// Estados posibles del agente segun el razonamiento
enum class EstadoAgente {
    Patrulla,
    Ataque,
    Retirada
};

// Resultado de la percepcion del agente
struct Estimulo {
    double distanciaCuadrada = 0.0; // d^2 entre agente y jugador
    bool oyeJugador = false;        // true si esta dentro del radio auditivo
    double saludRelativa = 1.0;     // Valor entre 0.0 y 1.0
    QPointF posAgente;
    QPointF posJugador;
};

// Registro de impacto recibido por sector angular y banda de distancia
struct RegistroImpacto {
    int sectorAngulo = 0;   // Indice de sector angular
    int bandaDistancia = 0; // Banda de distancia
    int conteoImpactos = 0; // Cantidad de impactos acumulados en este sector y banda
};


 //Cerebro autonomo del soldado centinela.

class AgenteInteligente {
private:
    double m_radioVision;             // Radio de vision R (se compara d^2 con R^2)
    double m_radioAudicion;           // Radio auditivo adicional para detectar sonido
    double m_retardoReaccion;         // Tiempo de retardo de reaccion en segundos (> 0)
    int m_umbralAprendizaje;          // Numero de impactos en un sector para adaptar respuesta (> 0)

    bool m_coberturaActiva;           // Indica si el agente cambio a posicion de cobertura
    int m_ultimoSectorCobertura;      // Sector angular que disparo el ultimo cambio de cobertura
    QPointF m_direccionPatrulla;      // Direccion base de patrulla cuando no hay objetivo activo
    std::vector<RegistroImpacto> m_memoriaImpactos; // Historial de impactos por sector y distancia

    static void validarParametros(double radioVision, double retardoReaccion, int umbralAprendizaje);
    int calcularSectorAngulo(const QPointF& posAgente, const QPointF& origenImpacto) const;
    int calcularBandaDistancia(double distanciaSq) const;

public:
    // Constructor
    AgenteInteligente(double radioVision = 250.0,
                      double retardoReaccion = 0.4,
                      int umbralAprendizaje = 3,
                      double radioAudicion = 350.0);

    AgenteInteligente(const AgenteInteligente& otro) = default;
    ~AgenteInteligente() = default;

    AgenteInteligente& operator=(const AgenteInteligente& otro);

    // 1. Percepcion: calcula d^2 sin raiz cuadrada, audicion y salud relativa
    Estimulo percibir(const QPointF& posAgente,
                      const QPointF& posJugador,
                      double saludRelativa) const;

    // 2. Razonamiento: retirada si salud < 0.20 (20%), ataque si d^2 <= R^2, sino patrulla
    EstadoAgente razonar(const Estimulo& estimulo) const;

    // 3. Accion: retorna direccion normalizada de movimiento segun el estado y el estimulo
    QPointF actuar(EstadoAgente estado, const Estimulo& estimulo) const;

    // 4. Aprendizaje: registra impacto por sector y distancia
    bool aprender(const QPointF& posAgente, const QPointF& origenImpacto);

    // Getters y setters
    double radioVision() const;
    void setRadioVision(double radio);

    double radioAudicion() const;
    void setRadioAudicion(double radio);

    double retardoReaccion() const;
    void setRetardoReaccion(double retardo);

    int umbralAprendizaje() const;
    void setUmbralAprendizaje(int umbral);

    bool coberturaActiva() const;
    void setCoberturaActiva(bool activa);

    int ultimoSectorCobertura() const;

    QPointF direccionPatrulla() const;
    void setDireccionPatrulla(const QPointF& dir);

    const std::vector<RegistroImpacto>& memoriaImpactos() const;
    void limpiarMemoria();
};

#endif // AGENTEINTELIGENTE_H