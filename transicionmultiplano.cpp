#include "TransicionMultiplano.h"
#include <QBrush>
#include <QEasingCurve>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QVariantAnimation>
#include <stdexcept>

TransicionMultiplano::TransicionMultiplano(const QSizeF& tamano, QObject* parent)
    : QObject(parent), tamano_(tamano), escena_(new QGraphicsScene(this)),
    animacion_(new QVariantAnimation(this)), velo_(nullptr) {
    if (tamano.width() <= 0 || tamano.height() <= 0)
        throw std::invalid_argument("TransicionMultiplano: tamano invalido");

    capas_.reserve(5);
    escena_->setSceneRect(0, 0, tamano_.width(), tamano_.height());

    velo_ = escena_->addRect(0, 0, tamano_.width(), tamano_.height(), QPen(Qt::NoPen), QBrush(Qt::black));
    velo_->setZValue(1000);
    velo_->setOpacity(0.0);

    animacion_->setDuration(2000);
    animacion_->setEasingCurve(QEasingCurve::InOutQuad);
    connect(animacion_, &QVariantAnimation::valueChanged, this, &TransicionMultiplano::aplicarProgreso);
    connect(animacion_, &QAbstractAnimation::finished, this, &TransicionMultiplano::terminada);
}

TransicionMultiplano::TransicionMultiplano(const TransicionMultiplano& otro)
    : TransicionMultiplano(otro.tamano_, nullptr) {
    *this = otro;
}

TransicionMultiplano& TransicionMultiplano::operator=(const TransicionMultiplano& otro) {
    if (this == &otro) return *this;
    animacion_->stop();
    borrarCapas();
    tamano_ = otro.tamano_;
    escena_->setSceneRect(0, 0, tamano_.width(), tamano_.height());
    velo_->setRect(0, 0, tamano_.width(), tamano_.height());
    direccion_ = otro.direccion_;
    oscurecer_ = otro.oscurecer_;
    animacion_->setDuration(otro.animacion_->duration());

    for (const Capa& c : otro.capas_) {
        QGraphicsPixmapItem* item = crearItem(c.item->pixmap());
        item->setScale(c.item->scale());
        item->setOpacity(c.item->opacity());
        capas_.push_back({item, c.zoomFinal, c.desvanecer});
    }
    velo_->setOpacity(otro.velo_->opacity());
    return *this;
}

bool TransicionMultiplano::operator==(const TransicionMultiplano& o) const {
    if (tamano_ != o.tamano_ || direccion_ != o.direccion_ || oscurecer_ != o.oscurecer_ ||
        duracion() != o.duracion() || capas_.size() != o.capas_.size())
        return false;
    for (std::size_t i = 0; i < capas_.size(); ++i) {
        if (!qFuzzyCompare(capas_[i].zoomFinal, o.capas_[i].zoomFinal) ||
            capas_[i].desvanecer != o.capas_[i].desvanecer ||
            capas_[i].item->pixmap().toImage() != o.capas_[i].item->pixmap().toImage())
            return false;
    }
    return true;
}

QGraphicsPixmapItem* TransicionMultiplano::crearItem(const QPixmap& imagen) {
    QGraphicsPixmapItem* item = escena_->addPixmap(imagen);
    item->setTransformationMode(Qt::SmoothTransformation);
    item->setTransformOriginPoint(imagen.width() / 2.0, imagen.height() / 2.0);
    item->setZValue(static_cast<qreal>(capas_.size()));
    return item;
}

void TransicionMultiplano::borrarCapas() {
    for (const Capa& c : capas_) {
        escena_->removeItem(c.item);
        delete c.item;
    }
    capas_.clear();
}

void TransicionMultiplano::agregarCapa(const QString& ruta, double zoomFinal, bool desvanecer) {
    QPixmap imagen(ruta);
    if (imagen.isNull()) throw std::runtime_error("Recurso no encontrado: " + ruta.toStdString());
    agregarCapa(imagen, zoomFinal, desvanecer);
}

void TransicionMultiplano::agregarCapa(const QPixmap& imagen, double zoomFinal, bool desvanecer) {
    if (imagen.isNull()) throw std::invalid_argument("TransicionMultiplano: imagen vacia");
    if (zoomFinal < 1.0) throw std::invalid_argument("TransicionMultiplano: zoomFinal debe ser >= 1");

    // Se ajusta una sola vez aqui, no en cada cuadro.
    const QPixmap ajustada = imagen.scaled(tamano_.toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    capas_.push_back({crearItem(ajustada), zoomFinal, desvanecer});
}

void TransicionMultiplano::iniciar(Direccion direccion, int duracionMs, bool oscurecer) {
    if (capas_.empty()) throw std::logic_error("TransicionMultiplano: no hay capas");
    setDuracion(duracionMs);
    direccion_ = direccion;
    oscurecer_ = oscurecer;

    animacion_->stop();
    const double inicio = (direccion == Direccion::Acercar) ? 0.0 : 1.0;
    animacion_->setStartValue(inicio);
    animacion_->setEndValue(1.0 - inicio);
    aplicarProgreso(inicio);
    animacion_->start();
}

bool TransicionMultiplano::estaActiva() const {
    return animacion_->state() == QAbstractAnimation::Running;
}

void TransicionMultiplano::aplicarProgreso(const QVariant& valor) {
    const double t = valor.toDouble();
    for (const Capa& c : capas_) {
        c.item->setScale(1.0 + (c.zoomFinal - 1.0) * t);
        c.item->setOpacity(c.desvanecer ? 1.0 - t * t : 1.0);
    }
    double oscuridad = 0.0;
    if (oscurecer_) oscuridad = (direccion_ == Direccion::Alejar) ? 1.0 - t : t;
    velo_->setOpacity(oscuridad);
}

QSizeF TransicionMultiplano::tamano() const { return tamano_; }

void TransicionMultiplano::setTamano(const QSizeF& tamano) {
    if (tamano.width() <= 0 || tamano.height() <= 0)
        throw std::invalid_argument("TransicionMultiplano: tamano invalido");
    tamano_ = tamano;
    escena_->setSceneRect(0, 0, tamano_.width(), tamano_.height());
    velo_->setRect(0, 0, tamano_.width(), tamano_.height());
    for (const Capa& c : capas_) {
        const QPixmap nueva = c.item->pixmap().scaled(tamano_.toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        c.item->setPixmap(nueva);
        c.item->setTransformOriginPoint(nueva.width() / 2.0, nueva.height() / 2.0);
    }
}

TransicionMultiplano::Direccion TransicionMultiplano::direccion() const { return direccion_; }
void TransicionMultiplano::setDireccion(Direccion direccion) { direccion_ = direccion; }
bool TransicionMultiplano::oscurecer() const { return oscurecer_; }
void TransicionMultiplano::setOscurecer(bool oscurecer) { oscurecer_ = oscurecer; }
int TransicionMultiplano::duracion() const { return animacion_->duration(); }

void TransicionMultiplano::setDuracion(int duracionMs) {
    if (duracionMs <= 0) throw std::invalid_argument("TransicionMultiplano: duracion invalida");
    animacion_->setDuration(duracionMs);
}

int TransicionMultiplano::cantidadCapas() const { return static_cast<int>(capas_.size()); }