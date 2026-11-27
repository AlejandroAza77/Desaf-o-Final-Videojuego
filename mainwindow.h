#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QElapsedTimer>
#include <QHash>
#include <QMainWindow>
#include <QSet>
#include <memory>
#include "niveldebatalla.h"

class EnemigoRealista;
class EscenaDeBatalla;
class ObjetoDelEscenario;
class QAudioOutput;
class QMediaPlayer;
class QSoundEffect;
class QButtonGroup;
class QEvent;
class QGraphicsView;
class QKeyEvent;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Ventana principal (Ventana Principal del diagrama): pantallas de inicio, menu, dificultad y juego,
// bucle del juego, entrada de teclado y mouse, y progreso guardado en disco. Es duena del nivel en curso.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Una ventana es un QObject: no se copia. == y != comparan su estado de juego.
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    bool operator==(const MainWindow& otro) const;
    bool operator!=(const MainWindow& otro) const { return !(*this == otro); }

protected:
    void keyPressEvent(QKeyEvent* e) override;  // F11: pantalla completa
    void keyReleaseEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private slots:
    void mostrarMenu();
    void elegirNivel(int numero);
    void jugar();
    void reiniciarDeFabrica();
    void tick();

private:
    enum class Pagina { Inicio = 0, Menu = 1, Dificultad = 2, Juego = 3, Resultado = 4 };

    Ui::MainWindow *ui;
    QStackedWidget* m_paginas;
    EscenaDeBatalla* m_escenaIntro;
    EscenaDeBatalla* m_escenaJuego;
    QGraphicsView* m_vistaIntro;
    QGraphicsView* m_vistaJuego;
    QPushButton* m_btnNivel1;
    QPushButton* m_btnNivel2;
    QWidget* m_pantMenu;       // pantallas cuyo fondo es una imagen de 1376x768 con los botones ya dibujados
    QWidget* m_pantDificultad;
    QWidget* m_pantResultado;
    QLabel* m_lblDesbloqueo;  // "Nivel 2 desbloqueado" en la pantalla de victoria
    QButtonGroup* m_grupoDificultad;
    QTimer* m_reloj;
    QElapsedTimer m_cronometro;
    QSet<int> m_teclas;
    std::unique_ptr<NivelDeBatalla> m_nivel;
    int m_nivelElegido;
    bool m_nivel2Desbloqueado;
    bool m_introAcercando;
    bool m_victoria;
    // Sonido: musica de fondo (menu y niveles), sonidos largos de una sola vez (inicio de nivel, derrota) y efectos cortos.
    QMediaPlayer* m_musica;
    QAudioOutput* m_salidaMusica;
    QMediaPlayer* m_jingle;
    QAudioOutput* m_salidaJingle;
    QHash<QString, QSoundEffect*> m_efectos;
    int m_faseMusica;  // 0 ninguna, 1 menu, 2 sonido de inicio de nivel, 3 fondo del nivel
    double m_tPaso;
    int m_pasoAlterno;
    QHash<const EnemigoRealista*, double> m_disparoPrevio;  // para detectar cuando un enemigo dispara
    QHash<const ObjetoDelEscenario*, bool> m_efectoPrevio;  // para detectar explosiones nuevas
    double m_tiempoFin;  // < 0 mientras se juega; despues, segundos desde que termino la partida

    QWidget* crearPaginaInicio();
    QWidget* crearPaginaMenu();
    QWidget* crearPaginaDificultad();
    QWidget* crearPaginaJuego();
    QWidget* crearPaginaResultado();
    // Pantallas con imagen de fondo: la imagen se ajusta a la ventana sin deformarse y los botones (transparentes,
    // con efecto al pasar el mouse) se ubican encima con coordenadas de la imagen original.
    QWidget* crearPaginaImagen(const QString& fondo);
    void ponerFondo(QWidget* pagina, const QString& ruta);
    QPushButton* agregarBotonImagen(QWidget* pagina, const QRect& referencia);
    void ubicarHijos(QWidget* pagina);
    void dibujarFondo(QWidget* pagina);
    void fundirPagina(QWidget* pagina, int ms);  // la pantalla aparece poco a poco
    void ajustarVista(QGraphicsView* vista, const QSize& tamano);  // la escena (960x480) llena toda la vista
    QGraphicsView* crearVista(EscenaDeBatalla* escena);
    void lanzarIntro();
    void iniciarSonido();
    void reproducirEfecto(const QString& nombre, double volumen = 1.0);
    void musicaMenu();
    void musicaNivel(int numero);
    void sonidoDerrota();
    void detenerMusica();
    void sonidosDelJuego(double dt);  // pasos de German, disparos y explosiones
    void actualizarMenu();
    void aplicarEntrada();
    void terminarPartida();
    void abandonarPartida();
    NivelDeBatalla::Dificultad dificultadElegida() const;
    Pagina paginaActual() const;
};
#endif // MAINWINDOW_H