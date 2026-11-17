#include <QApplication>
#include <QGraphicsView>
#include <QPainter>
#include <QTimer>
#include "TransicionMultiplano.h"

static QPixmap capa(const QSize& t, int tipo) {
    QPixmap pm(t);
    pm.fill(tipo == 0 ? QColor(25, 40, 90) : Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    if (tipo == 0) {
        p.setBrush(QColor(240, 240, 200));
        p.drawEllipse(QPointF(t.width() * 0.5, t.height() * 0.35), 60, 60);
    } else if (tipo == 1) {
        p.setBrush(QColor(40, 70, 60));
        p.drawEllipse(QPointF(t.width() * 0.3, t.height() * 0.85), 260, 150);
        p.drawEllipse(QPointF(t.width() * 0.75, t.height() * 0.9), 300, 170);
    } else if (tipo == 2) {
        p.setBrush(QColor(150, 90, 60));
        p.drawRect(t.width() * 0.42, t.height() * 0.6, 120, 90);
    } else {
        p.setBrush(QColor(15, 90, 40));
        for (int i = 0; i < 6; ++i) p.drawEllipse(QPointF(i * t.width() / 5.0, t.height() * 0.97), 90, 60);
    }
    return pm;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    const QSizeF tam(960, 540);
    TransicionMultiplano trans(tam);
    trans.agregarCapa(capa(tam.toSize(), 0), 1.10);
    trans.agregarCapa(capa(tam.toSize(), 1), 1.40);
    trans.agregarCapa(capa(tam.toSize(), 2), 2.20);
    trans.agregarCapa(capa(tam.toSize(), 3), 4.00, true);

    TransicionMultiplano copia(trans);
    Q_ASSERT(copia == trans);

    QGraphicsView vista(trans.escena());
    vista.setFixedSize(960, 540);
    vista.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    vista.show();

    bool acercar = true;
    QObject::connect(&trans, &TransicionMultiplano::terminada, [&]() {
        acercar = !acercar;
        QTimer::singleShot(800, [&]() {
            trans.iniciar(acercar ? TransicionMultiplano::Direccion::Acercar
                                  : TransicionMultiplano::Direccion::Alejar, 2500, !acercar);
        });
    });
    trans.iniciar(TransicionMultiplano::Direccion::Acercar, 2500);
    return app.exec();
}