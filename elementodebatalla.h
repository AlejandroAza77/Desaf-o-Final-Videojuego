#ifndef ELEMENTODEBATALLA_H
#define ELEMENTODEBATALLA_H

#include <QPointF>
#include <QRectF>
#include <QString>

/*
 * Clase abstracta ElementoDeBatalla
 * Base de todo lo que se mueve en pantalla.
 * Guarda: posicion, velocidad, tamano, integridad (vida) y si esta activo.
 */
class ElementoDeBatalla {
protected:
    QPointF m_posicion;    // Posicion actual en coordenadas del mundo (x, y)
    QPointF m_velocidad;   // Vector de velocidad (vx, vy)
    QPointF m_tamano;      // Ancho y alto para calcular los limites de colision
    double m_integridad;   // Vida o resistencia restante del elemento
    bool m_activo;         // Bandera para saber si sigue activo en el juego o debe eliminarse

public:
    // Constructor por defecto con valores iniciales
    ElementoDeBatalla(QPointF pos = QPointF(0, 0), QPointF vel = QPointF(0, 0), QPointF tam = QPointF(50, 50), double vida = 100.0);

    // Destructor virtual para garantizar la correcta liberacion de memoria en clases derivadas
    virtual ~ElementoDeBatalla() = default;

    // Firmas fijas solicitadas por la arquitectura
    virtual void actualizar(double dt) = 0;
    virtual QString rutaSprite() const = 0; // Daniel la usa para cargar el sprite correspondiente en la GUI

    // Getters y Setters de posicion y velocidad
    QPointF posicion() const;
    void setPosicion(const QPointF& pos);

    QPointF velocidad() const;
    void setVelocidad(const QPointF& vel);

    // Limites de colision basados en QRectF para usar QRectF::intersects
    QRectF limites() const;

    // Gestion de impacto y estado
    void recibirImpacto(double dano);
    bool estaActivo() const;
    void desactivar();
};

#endif // ELEMENTODEBATALLA_H