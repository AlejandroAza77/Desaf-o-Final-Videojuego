#ifndef ELEMENTODEBATALLA_H
#define ELEMENTODEBATALLA_H

#include <QPointF>
#include <QRectF>
#include <QString>
#include <cmath>
#include <typeinfo>

inline bool casiIgual(double a, double b) { return std::fabs(a - b) < 1e-9; }

// Ayudas geometricas compartidas por los niveles y el agente.
inline double distancia(const QPointF& a, const QPointF& b) { return std::hypot(a.x() - b.x(), a.y() - b.y()); }
inline QPointF normalizado(const QPointF& v) {
    const double len = std::hypot(v.x(), v.y());
    return len > 0.0 ? QPointF(v.x() / len, v.y() / len) : QPointF(0.0, 0.0);
}

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
    virtual bool esVisible() const { return estaActivo(); }  // un enemigo destruido puede seguir viendose (restos)
    virtual double opacidad() const { return 1.0; }
    virtual bool espejar() const { return false; }     // true: el sprite se dibuja espejado en horizontal
    virtual double rotacion() const { return 0.0; }    // grados con que se dibuja el sprite

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