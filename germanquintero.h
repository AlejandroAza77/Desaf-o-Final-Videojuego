#ifndef GERMANQUINTERO_H
#define GERMANQUINTERO_H

#include "elementodebatalla.h"
#include "armas.h"
#include <stdexcept>

class GermanQuintero : public ElementoDeBatalla {
public:
    enum class Modo { Cenital, Plataforma };
    enum class Especialidad { Ninguna, MachoAlfa, CrestaVeloz };
    // Estado visual segun los disparos recibidos (un disparo enemigo = 25 de 100): elige el sprite.
    enum class Desgaste { Normal, Leve, Medio, Fuerte, Fantasma };

    explicit GermanQuintero(const QPointF& pos, Modo modo, const QPointF& tam = QPointF(40, 60));
    GermanQuintero(const GermanQuintero&) = default;
    ~GermanQuintero() override = default;

    GermanQuintero& operator=(const GermanQuintero& otro);
    bool operator==(const GermanQuintero& otro) const;
    bool operator!=(const GermanQuintero& otro) const { return !(*this == otro); }

    void actualizar(double dt) override;
    QString rutaSprite() const override;

    void saltar();
    bool recogerCoco();
    bool lanzarCoco(const QPointF& destino, Armas& proyectil);
    void activarEspecialidad(Especialidad esp);
    double alcance() const;
    // Agranda a German y toda su fisica (velocidad, salto, gravedad, alcance) en la misma proporcion.
    double escalaMundo() const;
    void setEscalaMundo(double escala);
    Desgaste desgaste() const;

    Modo modo() const;
    void setModo(Modo modo);
    QPointF tamanoBase() const;
    void setTamanoBase(const QPointF& tam);
    QPointF direccion() const;
    void setDireccion(const QPointF& dir);
    QRectF zona() const;
    void setZonaMovimiento(const QRectF& zona);
    bool tieneZona() const;
    void setTieneZona(bool tiene);
    QPointF corriente() const;
    void setCorriente(const QPointF& vel);
    double suelo() const;
    void setSuelo(double sueloY);
    bool enSuelo() const;
    void setEnSuelo(bool enSuelo);
    double friccion() const;
    void setFriccion(double mu);
    int cocos() const;
    void setCocos(int cocos);
    int capacidadCocos() const;
    Especialidad especialidad() const;
    void setEspecialidad(Especialidad esp);
    double tiempoEspecialidad() const;
    void setTiempoEspecialidad(double t);
    double factorVelocidad() const;
    void setFactorVelocidad(double f);
    double factorSalto() const;
    void setFactorSalto(double f);
    double factorFriccion() const;
    void setFactorFriccion(double f);
    double factorDano() const;
    void setFactorDano(double f);
    double factorAlcance() const;
    void setFactorAlcance(double f);

private:
    static constexpr double kVelCenital = 180.0;
    static constexpr double kVelMaxPlat = 220.0;
    static constexpr double kAceleracion = 1500.0;
    static constexpr double kG = 1400.0;
    static constexpr double kSalto = 560.0;
    static constexpr double kAlcanceBase = 420.0;
    static constexpr double kVelVuelo = 380.0;
    static constexpr double kTVueloMin = 0.35;
    static constexpr double kGravedadCoco = 900.0;
    static constexpr double kDanoBase = 25.0;
    static constexpr double kMargenCorriente = 60.0;
    static constexpr double kDuracionMacho = 10.0;
    static constexpr double kDuracionCresta = 8.0;
    static constexpr int kCapacidad = 6;
    static constexpr double kVida = 100.0;

    Modo m_modo;
    QPointF m_tamanoBase;
    QPointF m_direccion;
    QRectF m_zona;
    bool m_tieneZona;
    QPointF m_corriente;
    double m_suelo;
    bool m_enSuelo;
    double m_mu;
    int m_cocos;
    Especialidad m_especialidad;
    double m_tiempoEspecialidad;
    double m_factorVelocidad, m_factorSalto, m_factorFriccion, m_factorDano, m_factorAlcance;
    double m_escala;

    void moverCenital(double dt);
    void moverPlataforma(double dt);
    void terminarEspecialidad();
    void cambiarTamano(const QPointF& nuevo);
};

#endif // GERMANQUINTERO_H