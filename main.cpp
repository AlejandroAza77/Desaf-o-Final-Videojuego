#include <QApplication>
#include <QCoreApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("DesafioFinal"));
    QCoreApplication::setApplicationName(QStringLiteral("ElMachoAlfaEnBoyaca"));  // el progreso se guarda con estos nombres
    MainWindow ventana;
    ventana.showMaximized();  // la ventana ocupa toda la pantalla (F11: pantalla completa)
    return app.exec();
}