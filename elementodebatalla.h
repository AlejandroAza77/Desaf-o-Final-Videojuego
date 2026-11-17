#ifndef ELEMENTODEBATALLA_H
#define ELEMENTODEBATALLA_H

#include <QPointF>
#include <QRectF>
#include <QString>
#include <cmath>
#include <typeinfo>

inline bool casiIgual(double a, double b) { return std::fabs(a - b) < 1e-9; }

class ElementoDeBatalla {
protected:
    QPointF m_posicion;
    QPointF m_velocidad;
    QPointF m_tamano;
    double m_integridad;
    bool m_activo;

    // protegidos: la clase es abstracta (evita slicing)
    ElementoDeBatalla(const ElementoDeBatalla&) = default;
    ElementoDeBatalla& operator=(const ElementoDeBatalla& otro);

public:
    ElementoDeBatalla(const QPointF& pos = QPointF(0, 0), const QPointF& vel = QPointF(0, 0),
                      const QPointF& tam = QPointF(50, 50), double vida = 100.0);
    virtual ~ElementoDeBatalla() = default;

    bool operator==(const ElementoDeBatalla& otro) const;
    bool operator!=(const ElementoDeBatalla& otro) const { return !(*this == otro); }

    virtual void actualizar(double dt) = 0;
    virtual QString rutaSprite() const = 0;
    virtual double altura() const { return 0.0; }

    QPointF posicion() const;
    void setPosicion(const QPointF& pos);
    QPointF velocidad() const;
    void setVelocidad(const QPointF& vel);
    QPointF tamano() const;
    void setTamano(const QPointF& tam);
    double integridad() const;
    void setIntegridad(double vida);
    bool estaActivo() const;
    void setActivo(bool activo);

    QRectF limites() const;
    void recibirImpacto(double dano);
    void desactivar();
};

#endif // ELEMENTODEBATALLA_H