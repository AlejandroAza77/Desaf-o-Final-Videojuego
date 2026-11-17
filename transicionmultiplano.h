#pragma once

#include <QObject>
#include <QPixmap>
#include <QSizeF>
#include <QString>
#include <QVariant>
#include <vector>

class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QVariantAnimation;

class TransicionMultiplano : public QObject {
    Q_OBJECT
public:
    enum class Direccion { Acercar, Alejar };

    explicit TransicionMultiplano(const QSizeF& tamano, QObject* parent = nullptr);
    TransicionMultiplano(const TransicionMultiplano& otro);  // la copia no tiene padre
    ~TransicionMultiplano() override = default;

    // QObject no se puede copiar: = y == trabajan sobre la configuracion y las capas.
    TransicionMultiplano& operator=(const TransicionMultiplano& otro);
    bool operator==(const TransicionMultiplano& otro) const;
    bool operator!=(const TransicionMultiplano& otro) const { return !(*this == otro); }

    void agregarCapa(const QString& ruta, double zoomFinal, bool desvanecer = false);
    void agregarCapa(const QPixmap& imagen, double zoomFinal, bool desvanecer = false);
    void iniciar(Direccion direccion, int duracionMs = 2000, bool oscurecer = false);
    bool estaActiva() const;

    QGraphicsScene* escena() const { return escena_; }
    QSizeF tamano() const;
    void setTamano(const QSizeF& tamano);
    Direccion direccion() const;
    void setDireccion(Direccion direccion);
    bool oscurecer() const;
    void setOscurecer(bool oscurecer);
    int duracion() const;
    void setDuracion(int duracionMs);
    int cantidadCapas() const;

signals:
    void terminada();

private slots:
    void aplicarProgreso(const QVariant& valor);

private:
    struct Capa {
        QGraphicsPixmapItem* item;
        double zoomFinal;
        bool desvanecer;
    };

    QSizeF tamano_;
    QGraphicsScene* escena_;
    QVariantAnimation* animacion_;
    QGraphicsRectItem* velo_;
    std::vector<Capa> capas_;
    Direccion direccion_ = Direccion::Acercar;
    bool oscurecer_ = false;

    QGraphicsPixmapItem* crearItem(const QPixmap& imagen);
    void borrarCapas();
};