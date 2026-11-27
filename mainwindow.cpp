#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "enemigorealista.h"
#include "escenadebatalla.h"
#include "germanquintero.h"
#include "nivelpuenteboyaca.h"
#include "nivelrioteatinos.h"
#include "objetodelescenario.h"
#include <QApplication>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSoundEffect>
#include <QUrl>
#include <QButtonGroup>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTransform>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <exception>

namespace {
constexpr int kAnchoVista = 960;
constexpr int kAltoVista = 480;
// Volumenes (0..1, lineal). Los fondos se bajan para que suenen de ambiente; el del Nivel 2 viene ~7 dB mas bajo
// que el del Nivel 1, por eso lleva un volumen mayor para que ambos se escuchen igual de fuertes.
constexpr double kVolMenu = 0.40;
constexpr double kVolFondoNivel1 = 0.24;
constexpr double kVolFondoNivel2 = 0.56;
constexpr double kVolInicioNivel = 1.0;
constexpr double kVolDerrota = 0.85;
constexpr double kIntervaloPaso = 0.18;  // s entre pasos de German mientras camina
constexpr int kAnchoImagen = 1376;  // las imagenes de las pantallas miden 1376x768
constexpr int kAltoImagen = 768;
constexpr int kPasoMs = 16;
constexpr double kDtMaximo = 0.05;
constexpr double kPausaVictoria = 0.8;  // segundos que se sigue viendo la escena tras ganar
constexpr double kPausaDerrota = 1.6;   // ...y tras perder (se ve el fantasma)
const char* const kClaveNivel2 = "progreso/nivel2Desbloqueado";

// Botones transparentes sobre los botones dibujados en la imagen: brillan al pasar el mouse y se oscurecen al presionar.
const char* const kEstiloBotonImagen =
    "QPushButton { background: transparent; border: none; padding: 0; min-width: 0; }"
    "QPushButton:hover { background: rgba(255,255,255,55); border-radius: 7px; }"
    "QPushButton:pressed { background: rgba(0,0,0,80); border-radius: 7px; }"
    "QPushButton:disabled { background: rgba(0,0,0,165); border-radius: 7px; }";

const char* const kEstilo =
    "QWidget { background: #1b1f2a; color: #f2e9d8; font-family: 'Segoe UI', sans-serif; }"
    "QLabel#titulo { font-size: 38px; font-weight: bold; color: #f4c542; }"
    "QLabel#subtitulo { font-size: 20px; color: #f2e9d8; }"
    "QLabel#ayuda { font-size: 13px; color: #9aa3b8; }"
    "QPushButton { background: #3b4a6b; border: 2px solid #f4c542; border-radius: 8px; padding: 10px 28px;"
    "  font-size: 18px; color: #ffffff; min-width: 260px; }"
    "QPushButton:hover { background: #4d6090; }"
    "QPushButton:disabled { background: #2a2f3d; border-color: #555b6b; color: #777d8c; }"
    "QPushButton:checked { background: #f4c542; color: #1b1f2a; font-weight: bold; }";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_paginas(nullptr), m_escenaIntro(nullptr),
    m_escenaJuego(nullptr), m_vistaIntro(nullptr), m_vistaJuego(nullptr), m_btnNivel1(nullptr),
    m_btnNivel2(nullptr), m_pantMenu(nullptr), m_pantDificultad(nullptr), m_pantResultado(nullptr), m_lblDesbloqueo(nullptr),
    m_grupoDificultad(nullptr), m_reloj(new QTimer(this)),
    m_nivelElegido(1), m_nivel2Desbloqueado(false), m_introAcercando(true), m_victoria(false), m_musica(nullptr), m_salidaMusica(nullptr), m_jingle(nullptr),
    m_salidaJingle(nullptr), m_faseMusica(0), m_tPaso(0.0), m_pasoAlterno(0), m_tiempoFin(-1.0)
{
    ui->setupUi(this);
    menuBar()->hide();
    statusBar()->hide();
    setWindowTitle(QStringLiteral("El Macho Alfa en Boyacá"));
    setFocusPolicy(Qt::StrongFocus);  // la ventana recibe WASD y espacio
    setStyleSheet(QString::fromLatin1(kEstilo));

    m_nivel2Desbloqueado = QSettings().value(QString::fromLatin1(kClaveNivel2), false).toBool();

    m_paginas = new QStackedWidget(this);
    m_paginas->addWidget(crearPaginaInicio());
    m_paginas->addWidget(crearPaginaMenu());
    m_paginas->addWidget(crearPaginaDificultad());
    m_paginas->addWidget(crearPaginaJuego());
    m_paginas->addWidget(crearPaginaResultado());
    setCentralWidget(m_paginas);

    m_reloj->setInterval(kPasoMs);
    connect(m_reloj, &QTimer::timeout, this, &MainWindow::tick);

    actualizarMenu();
    m_paginas->setCurrentIndex(static_cast<int>(Pagina::Inicio));
    lanzarIntro();
    iniciarSonido();
    musicaMenu();  // al abrir el juego suena la musica del menu
    resize(1200, 720);
    setFocus();
}

MainWindow::~MainWindow()
{
    m_reloj->stop();
    delete ui;
}

bool MainWindow::operator==(const MainWindow& o) const {
    return m_nivelElegido == o.m_nivelElegido && m_nivel2Desbloqueado == o.m_nivel2Desbloqueado &&
           paginaActual() == o.paginaActual() && dificultadElegida() == o.dificultadElegida() &&
           m_reloj->isActive() == o.m_reloj->isActive();
}

MainWindow::Pagina MainWindow::paginaActual() const { return static_cast<Pagina>(m_paginas->currentIndex()); }

// ---------- construccion de pantallas ----------

QGraphicsView* MainWindow::crearVista(EscenaDeBatalla* escena) {
    QGraphicsView* v = new QGraphicsView(escena);
    v->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // llena toda la ventana
    v->setMinimumSize(kAnchoVista / 2, kAltoVista / 2);
    v->setBackgroundBrush(QColor(13, 15, 20));
    v->setRenderHint(QPainter::SmoothPixmapTransform);
    v->installEventFilter(this);  // para reajustar la escala cuando cambia el tamano
    v->setFrameShape(QFrame::NoFrame);
    v->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v->setFocusPolicy(Qt::NoFocus);  // las teclas las recibe la ventana
    return v;
}

QWidget* MainWindow::crearPaginaInicio() {
    QWidget* p = new QWidget;
    QVBoxLayout* l = new QVBoxLayout(p);
    l->setContentsMargins(0, 0, 0, 12);
    QLabel* titulo = new QLabel(QStringLiteral("El Macho Alfa en Boyacá"));
    titulo->setObjectName(QStringLiteral("titulo"));
    titulo->setAlignment(Qt::AlignCenter);
    m_escenaIntro = new EscenaDeBatalla(QSizeF(kAnchoVista, kAltoVista), this);
    m_escenaIntro->mostrarPresentacion();
    m_vistaIntro = crearVista(m_escenaIntro);
    QPushButton* iniciar = new QPushButton(QStringLiteral("Iniciar"));
    iniciar->setFocusPolicy(Qt::NoFocus);
    connect(iniciar, &QPushButton::clicked, this, [this]() { reproducirEfecto(QStringLiteral("click")); });
    connect(iniciar, &QPushButton::clicked, this, &MainWindow::mostrarMenu);
    l->addWidget(titulo);
    l->addWidget(m_vistaIntro, 1);
    l->addWidget(iniciar, 0, Qt::AlignCenter);

    // La animacion se repite: se acerca, espera, se aleja, espera...
    connect(m_escenaIntro, &EscenaDeBatalla::transicionTerminada, this, [this]() {
        m_introAcercando = !m_introAcercando;
        QTimer::singleShot(900, this, [this]() {
            if (paginaActual() == Pagina::Inicio) lanzarIntro();
        });
    });
    return p;
}

void MainWindow::lanzarIntro() {
    m_escenaIntro->iniciarTransicion(m_introAcercando ? EscenaDeBatalla::Direccion::Acercar : EscenaDeBatalla::Direccion::Alejar, 3500,
                                     false);
}

// ---------- pantallas con imagen (menu, dificultad, resultado) ----------
// Las imagenes traen los botones dibujados: encima se ponen botones transparentes con efecto al pasar el mouse.

QWidget* MainWindow::crearPaginaImagen(const QString& fondo) {
    QWidget* p = new QWidget;
    p->installEventFilter(this);  // aqui se dibuja el fondo y se reubican los botones (ver eventFilter)
    ponerFondo(p, fondo);
    return p;
}

void MainWindow::ponerFondo(QWidget* pagina, const QString& ruta) {
    pagina->setProperty("fondo", QPixmap(ruta));
    pagina->update();
}

QPushButton* MainWindow::agregarBotonImagen(QWidget* pagina, const QRect& referencia) {
    QPushButton* b = new QPushButton(pagina);
    b->setFocusPolicy(Qt::NoFocus);  // las teclas del juego las recibe la ventana
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QString::fromLatin1(kEstiloBotonImagen));
    b->setProperty("referencia", referencia);
    connect(b, &QPushButton::clicked, this, [this]() { reproducirEfecto(QStringLiteral("click")); });
    ubicarHijos(pagina);
    return b;
}

void MainWindow::ubicarHijos(QWidget* pagina) {
    const double escala = std::min(pagina->width() / static_cast<double>(kAnchoImagen), pagina->height() / static_cast<double>(kAltoImagen));
    if (escala <= 0.0) return;
    const double ox = (pagina->width() - kAnchoImagen * escala) / 2.0;
    const double oy = (pagina->height() - kAltoImagen * escala) / 2.0;
    for (QObject* o : pagina->children()) {
        QWidget* w = qobject_cast<QWidget*>(o);
        if (w == nullptr || !w->property("referencia").isValid()) continue;
        const QRect r = w->property("referencia").toRect();
        w->setGeometry(QRect(static_cast<int>(ox + r.x() * escala), static_cast<int>(oy + r.y() * escala),
                             static_cast<int>(r.width() * escala), static_cast<int>(r.height() * escala)));
    }
}

void MainWindow::dibujarFondo(QWidget* pagina) {
    QPainter p(pagina);
    p.fillRect(pagina->rect(), QColor(13, 15, 20));
    const QPixmap fondo = pagina->property("fondo").value<QPixmap>();
    if (fondo.isNull()) return;
    const double escala = std::min(pagina->width() / static_cast<double>(kAnchoImagen), pagina->height() / static_cast<double>(kAltoImagen));
    const double ox = (pagina->width() - kAnchoImagen * escala) / 2.0;
    const double oy = (pagina->height() - kAltoImagen * escala) / 2.0;
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawPixmap(QRectF(ox, oy, kAnchoImagen * escala, kAltoImagen * escala), fondo, QRectF(fondo.rect()));
}

void MainWindow::fundirPagina(QWidget* pagina, int ms) {
    QGraphicsOpacityEffect* efecto = new QGraphicsOpacityEffect(pagina);
    efecto->setOpacity(0.0);
    pagina->setGraphicsEffect(efecto);  // reemplaza (y borra) cualquier efecto anterior
    QPropertyAnimation* anim = new QPropertyAnimation(efecto, "opacity", efecto);
    anim->setDuration(ms);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QPropertyAnimation::finished, pagina, [pagina, efecto]() {
        QTimer::singleShot(0, pagina, [pagina, efecto]() {
            if (pagina->graphicsEffect() == efecto) pagina->setGraphicsEffect(nullptr);  // ya termino: se dibuja normal
        });
    });
    anim->start();
}

void MainWindow::ajustarVista(QGraphicsView* vista, const QSize& tamano) {
    const double escala = std::min(tamano.width() / static_cast<double>(kAnchoVista), tamano.height() / static_cast<double>(kAltoVista));
    if (escala > 0.0) vista->setTransform(QTransform::fromScale(escala, escala));
}

QWidget* MainWindow::crearPaginaMenu() {
    m_pantMenu = crearPaginaImagen(QStringLiteral(":/sprites/ui/menu.jpg"));
    m_btnNivel1 = agregarBotonImagen(m_pantMenu, QRect(460, 238, 475, 88));
    m_btnNivel2 = agregarBotonImagen(m_pantMenu, QRect(460, 342, 475, 88));
    QPushButton* reiniciar = agregarBotonImagen(m_pantMenu, QRect(460, 447, 475, 88));
    QPushButton* salir = agregarBotonImagen(m_pantMenu, QRect(460, 552, 475, 88));
    connect(m_btnNivel1, &QPushButton::clicked, this, [this]() { elegirNivel(1); });
    connect(m_btnNivel2, &QPushButton::clicked, this, [this]() { elegirNivel(2); });
    connect(reiniciar, &QPushButton::clicked, this, &MainWindow::reiniciarDeFabrica);
    connect(salir, &QPushButton::clicked, qApp, &QApplication::quit);
    return m_pantMenu;
}

QWidget* MainWindow::crearPaginaDificultad() {
    m_pantDificultad = crearPaginaImagen(QStringLiteral(":/sprites/ui/dificultad_1.jpg"));
    m_grupoDificultad = new QButtonGroup(this);
    m_grupoDificultad->setExclusive(true);
    const char* nombres[] = {"facil", "intermedio", "dificil"};
    for (int i = 0; i < 3; ++i) {
        QPushButton* b = agregarBotonImagen(m_pantDificultad, QRect(460, 238 + 104 * i, 475, 88));
        const QString n = QString::fromLatin1(nombres[i]);
        // Cada opcion se dibuja con su imagen: normal, brillante al pasar el mouse y dorada cuando esta elegida.
        b->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: none; padding: 0; min-width: 0;"
                                        " border-image: url(:/sprites/ui/%1_normal.png) 0 0 0 0 stretch stretch; }"
                                        "QPushButton:hover { border-image: url(:/sprites/ui/%1_hover.png) 0 0 0 0 stretch stretch; }"
                                        "QPushButton:pressed { border-image: url(:/sprites/ui/%1_normal.png) 0 0 0 0 stretch stretch; }"
                                        "QPushButton:checked { border-image: url(:/sprites/ui/%1_dorado.png) 0 0 0 0 stretch stretch; }").arg(n));
        b->setCheckable(true);
        m_grupoDificultad->addButton(b, i);
        if (i == 0) b->setChecked(true);  // Facil seleccionado al inicio, como en la imagen
    }
    QPushButton* jugarBtn = agregarBotonImagen(m_pantDificultad, QRect(460, 552, 475, 88));
    QPushButton* volver = agregarBotonImagen(m_pantDificultad, QRect(460, 657, 475, 88));
    connect(jugarBtn, &QPushButton::clicked, this, &MainWindow::jugar);
    connect(volver, &QPushButton::clicked, this, &MainWindow::mostrarMenu);
    return m_pantDificultad;
}

QWidget* MainWindow::crearPaginaResultado() {
    m_pantResultado = crearPaginaImagen(QStringLiteral(":/sprites/ui/perdiste.jpg"));
    QPushButton* regresar = agregarBotonImagen(m_pantResultado, QRect(500, 608, 385, 99));
    connect(regresar, &QPushButton::clicked, this, &MainWindow::mostrarMenu);
    m_lblDesbloqueo = new QLabel(QStringLiteral("¡Nivel 2 desbloqueado!"), m_pantResultado);
    m_lblDesbloqueo->setAlignment(Qt::AlignCenter);
    m_lblDesbloqueo->setStyleSheet(QStringLiteral("QLabel { background: transparent; color: #f4c542; font-size: 22px; font-weight: bold; }"));
    m_lblDesbloqueo->setProperty("referencia", QRect(388, 535, 600, 50));
    ubicarHijos(m_pantResultado);
    m_lblDesbloqueo->hide();
    return m_pantResultado;
}

QWidget* MainWindow::crearPaginaJuego() {
    QWidget* p = new QWidget;
    QVBoxLayout* l = new QVBoxLayout(p);
    l->setContentsMargins(0, 0, 0, 4);
    l->setSpacing(2);
    m_escenaJuego = new EscenaDeBatalla(QSizeF(kAnchoVista, kAltoVista), this);
    m_vistaJuego = crearVista(m_escenaJuego);
    m_vistaJuego->viewport()->installEventFilter(this);  // clics del mouse = lanzar coco
    QLabel* ayuda = new QLabel(QStringLiteral("WASD: moverse   ·   Espacio: saltar   ·   Clic: lanzar coco   ·   Esc: volver al menú"));
    ayuda->setObjectName(QStringLiteral("ayuda"));
    ayuda->setAlignment(Qt::AlignCenter);
    l->addWidget(m_vistaJuego, 1);
    l->addWidget(ayuda, 0, Qt::AlignCenter);
    return p;
}

// ---------- sonido ----------

void MainWindow::iniciarSonido() {
    m_salidaMusica = new QAudioOutput(this);
    m_musica = new QMediaPlayer(this);
    m_musica->setAudioOutput(m_salidaMusica);
    m_musica->setLoops(QMediaPlayer::Infinite);
    m_salidaJingle = new QAudioOutput(this);
    m_jingle = new QMediaPlayer(this);
    m_jingle->setAudioOutput(m_salidaJingle);
    // Al terminar el sonido de inicio del nivel empieza la musica de fondo de ese nivel.
    connect(m_jingle, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus estado) {
        if (estado != QMediaPlayer::EndOfMedia || m_faseMusica != 2) return;
        m_faseMusica = 3;
        m_musica->setSource(QUrl(m_nivelElegido == 1 ? QStringLiteral("qrc:/sonidos/fondo_nivel1.mp3") : QStringLiteral("qrc:/sonidos/fondo_nivel2.mp3")));
        m_salidaMusica->setVolume(static_cast<float>(m_nivelElegido == 1 ? kVolFondoNivel1 : kVolFondoNivel2));
        m_musica->play();
    });
    const char* nombres[] = {"click", "paso_0", "paso_1", "disparo_canon", "disparo_fusil", "cohete", "explosion", "explosion_grande"};
    for (const char* n : nombres) {
        QSoundEffect* e = new QSoundEffect(this);
        e->setSource(QUrl(QStringLiteral("qrc:/sonidos/") + QString::fromLatin1(n) + QStringLiteral(".wav")));
        m_efectos.insert(QString::fromLatin1(n), e);
    }
}

void MainWindow::reproducirEfecto(const QString& nombre, double volumen) {
    QSoundEffect* e = m_efectos.value(nombre, nullptr);
    if (e == nullptr) return;
    e->setVolume(static_cast<float>(std::max(0.0, std::min(1.0, volumen))));
    e->play();
}

void MainWindow::musicaMenu() {
    if (m_faseMusica == 1 && m_musica->playbackState() == QMediaPlayer::PlayingState) return;  // ya suena: no se reinicia
    m_jingle->stop();
    m_faseMusica = 1;
    m_musica->setSource(QUrl(QStringLiteral("qrc:/sonidos/fondo_menu.mp3")));
    m_salidaMusica->setVolume(static_cast<float>(kVolMenu));
    m_musica->play();
}

// Primero suena el sonido de inicio (solo); cuando termina empieza el fondo del nivel (ver iniciarSonido).
void MainWindow::musicaNivel(int numero) {
    Q_UNUSED(numero);
    m_musica->stop();
    m_faseMusica = 2;
    m_salidaJingle->setVolume(static_cast<float>(kVolInicioNivel));
    m_jingle->setSource(QUrl(QStringLiteral("qrc:/sonidos/inicio_nivel.mp3")));
    m_jingle->play();
}

void MainWindow::sonidoDerrota() {
    m_musica->stop();
    m_faseMusica = 0;
    m_salidaJingle->setVolume(static_cast<float>(kVolDerrota));
    m_jingle->setSource(QUrl(QStringLiteral("qrc:/sonidos/perder_nivel.mp3")));
    m_jingle->play();
}

void MainWindow::detenerMusica() {
    m_musica->stop();
    m_jingle->stop();
    m_faseMusica = 0;
}

// Pasos de German, disparos de los enemigos y explosiones. Se detectan mirando el estado del nivel en cada cuadro.
// Mientras mas lejos ocurre algo, mas bajito suena.
void MainWindow::sonidosDelJuego(double dt) {
    if (!m_nivel) return;
    const GermanQuintero* g = m_nivel->jugador();
    const QPointF pg = g != nullptr ? g->posicion() : QPointF(0, 0);
    auto volumenPorDistancia = [&](const QPointF& p) { return std::max(0.18, 1.0 / (1.0 + distancia(p, pg) / 700.0)); };

    // Pasos: al ritmo de la caminata, solo si German se mueve (y en el Nivel 2, solo si pisa el suelo o una plataforma).
    if (g != nullptr && g->estaActivo()) {
        const bool camina = std::hypot(g->velocidad().x(), g->velocidad().y()) > 5.0 &&
                            (g->modo() == GermanQuintero::Modo::Cenital || g->enSuelo());
        if (camina) {
            m_tPaso += dt;
            if (m_tPaso >= kIntervaloPaso) {
                m_tPaso = 0.0;
                m_pasoAlterno ^= 1;
                reproducirEfecto(m_pasoAlterno == 0 ? QStringLiteral("paso_0") : QStringLiteral("paso_1"), 0.6);
            }
        } else {
            m_tPaso = kIntervaloPaso * 0.5;  // al arrancar a caminar el primer paso suena pronto
        }
    }
    // Disparos: cuando el reloj de disparo de un enemigo se reinicia (sube de golpe), acaba de disparar.
    for (const EnemigoRealista* e : m_nivel->enemigos()) {
        if (e->tipo() == TipoEnemigo::Carreta) continue;
        const double previo = m_disparoPrevio.value(e, e->tiempoDisparo());
        if (e->estaActivo() && e->tiempoDisparo() > previo + 0.05) {
            const double v = volumenPorDistancia(e->posicion());
            if (e->tipo() == TipoEnemigo::EnemigoFinal) reproducirEfecto(QStringLiteral("cohete"), v);
            else if (e->tipo() == TipoEnemigo::Comandante) reproducirEfecto(QStringLiteral("disparo_fusil"), v);
            else reproducirEfecto(QStringLiteral("disparo_canon"), v);  // canones del Nivel 1 y 2
        }
        m_disparoPrevio.insert(e, e->tiempoDisparo());
    }
    // Explosiones: efectos de impacto que acaban de aparecer (bala de canon, carreta o canon destruido, cohete).
    for (const ObjetoDelEscenario* o : m_nivel->objetosEscenario()) {
        if (o->tipo() != TipoObjeto::Efecto) continue;
        const bool activo = o->estaActivo();
        if (activo && !m_efectoPrevio.value(o, false)) {
            const int tipo = o->variante();
            if (tipo == static_cast<int>(TipoEfecto::BalaImpacto) || tipo == static_cast<int>(TipoEfecto::Explosion))
                reproducirEfecto(QStringLiteral("explosion"), volumenPorDistancia(o->posicion()));
            else if (tipo == static_cast<int>(TipoEfecto::CoheteExplosion))
                reproducirEfecto(QStringLiteral("explosion_grande"), volumenPorDistancia(o->posicion()));
        }
        m_efectoPrevio.insert(o, activo);
    }
}

// ---------- navegacion y progreso ----------

void MainWindow::actualizarMenu() {
    m_btnNivel2->setEnabled(m_nivel2Desbloqueado);  // bloqueado: se ve oscurecido
    m_btnNivel2->setToolTip(m_nivel2Desbloqueado ? QString() : QStringLiteral("Bloqueado: gana el Nivel 1 para desbloquearlo"));
}

void MainWindow::mostrarMenu() {
    m_reloj->stop();
    m_escenaIntro->detenerTransicion();
    actualizarMenu();
    m_paginas->setCurrentIndex(static_cast<int>(Pagina::Menu));
    fundirPagina(m_pantMenu, 350);
    musicaMenu();  // sigue sonando en el menu y en la eleccion de dificultad
}

void MainWindow::elegirNivel(int numero) {
    m_nivelElegido = numero;
    ponerFondo(m_pantDificultad, numero == 1 ? QStringLiteral(":/sprites/ui/dificultad_1.jpg") : QStringLiteral(":/sprites/ui/dificultad_2.jpg"));
    m_paginas->setCurrentIndex(static_cast<int>(Pagina::Dificultad));
    fundirPagina(m_pantDificultad, 250);
}

void MainWindow::reiniciarDeFabrica() {
    const auto r = QMessageBox::question(this, QStringLiteral("Reiniciar de fábrica"),
                                         QStringLiteral("Se volverá a bloquear el Nivel 2. ¿Continuar?"),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r != QMessageBox::Yes) return;
    QSettings().remove(QString::fromLatin1(kClaveNivel2));
    m_nivel2Desbloqueado = false;
    actualizarMenu();
}

NivelDeBatalla::Dificultad MainWindow::dificultadElegida() const {
    switch (m_grupoDificultad->checkedId()) {
    case 0: return NivelDeBatalla::Dificultad::Facil;
    case 2: return NivelDeBatalla::Dificultad::Dificil;
    default: return NivelDeBatalla::Dificultad::Normal;
    }
}

// ---------- partida ----------

void MainWindow::jugar() {
    try {
        const NivelDeBatalla::Dificultad d = dificultadElegida();
        if (m_nivelElegido == 1) {
            GermanQuintero* german = new GermanQuintero(QPointF(100, 240), GermanQuintero::Modo::Cenital);
            m_nivel.reset(new NivelRioTeatinos(german, d));  // el nivel pasa a ser dueño de German
        } else {
            GermanQuintero* german = new GermanQuintero(QPointF(80, 320), GermanQuintero::Modo::Plataforma);
            m_nivel.reset(new NivelPuenteBoyaca(german, d));
        }
        m_escenaJuego->mostrarNivel(m_nivel.get());
    } catch (const std::exception& e) {
        QMessageBox::critical(this, QStringLiteral("No se pudo iniciar el nivel"), QString::fromUtf8(e.what()));
        return;
    }
    m_teclas.clear();
    m_disparoPrevio.clear();
    m_efectoPrevio.clear();
    m_tPaso = kIntervaloPaso * 0.5;
    musicaNivel(m_nivelElegido);  // suena el sonido de inicio y, al terminar, el fondo del nivel
    m_tiempoFin = -1.0;
    m_victoria = false;
    m_paginas->setCurrentIndex(static_cast<int>(Pagina::Juego));
    m_cronometro.start();
    m_reloj->start();
    setFocus();
}

void MainWindow::aplicarEntrada() {
    GermanQuintero* g = m_nivel->jugador();
    if (g == nullptr) return;
    const double x = (m_teclas.contains(Qt::Key_D) ? 1.0 : 0.0) - (m_teclas.contains(Qt::Key_A) ? 1.0 : 0.0);
    double y = 0.0;  // en el Nivel 2 W y S no hacen nada: se salta con espacio
    if (g->modo() == GermanQuintero::Modo::Cenital)
        y = (m_teclas.contains(Qt::Key_S) ? 1.0 : 0.0) - (m_teclas.contains(Qt::Key_W) ? 1.0 : 0.0);
    g->setDireccion(QPointF(x, y));
}

void MainWindow::tick() {
    if (!m_nivel) return;
    const double dt = std::min(kDtMaximo, m_cronometro.restart() / 1000.0);
    if (m_tiempoFin < 0.0) aplicarEntrada();
    m_nivel->actualizar(dt);
    if (m_tiempoFin < 0.0) sonidosDelJuego(dt);
    m_escenaJuego->actualizarVista(dt);

    if (m_tiempoFin < 0.0) {
        if (m_nivel->victoria()) {
            m_victoria = true;
            detenerMusica();
            m_tiempoFin = 0.0;
        } else if (m_nivel->derrota()) {
            m_victoria = false;
            sonidoDerrota();  // suena el sonido de perder, una sola vez
            m_tiempoFin = 0.0;
        }
    } else {
        m_tiempoFin += dt;
        if (m_tiempoFin >= (m_victoria ? kPausaVictoria : kPausaDerrota)) terminarPartida();
    }
}

void MainWindow::terminarPartida() {
    m_reloj->stop();
    const bool desbloquea = m_victoria && m_nivelElegido == 1;
    if (desbloquea) {
        m_nivel2Desbloqueado = true;
        QSettings().setValue(QString::fromLatin1(kClaveNivel2), true);
    }
    m_escenaJuego->soltarNivel();
    m_nivel.reset();  // la escena queda sin nivel: se vuelve a mostrar uno en la proxima partida
    // Pantalla de resultado: la imagen de victoria o derrota, con su boton para volver al menu.
    ponerFondo(m_pantResultado, m_victoria ? QStringLiteral(":/sprites/ui/ganaste.jpg") : QStringLiteral(":/sprites/ui/perdiste.jpg"));
    m_lblDesbloqueo->setVisible(desbloquea);
    actualizarMenu();
    if (m_victoria) musicaMenu();  // no hay sonido de victoria: suena la musica del menu
    m_paginas->setCurrentIndex(static_cast<int>(Pagina::Resultado));
    fundirPagina(m_pantResultado, 800);
}

void MainWindow::abandonarPartida() {
    m_reloj->stop();
    m_escenaJuego->soltarNivel();
    m_nivel.reset();
    mostrarMenu();
}

// ---------- entrada ----------

void MainWindow::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_F11) {  // pantalla completa / ventana maximizada
        if (isFullScreen()) showMaximized();
        else showFullScreen();
        return;
    }
    if (paginaActual() == Pagina::Juego && m_nivel) {
        if (e->key() == Qt::Key_Escape) {
            abandonarPartida();
            return;
        }
        if (!e->isAutoRepeat()) {
            m_teclas.insert(e->key());
            GermanQuintero* g = m_nivel->jugador();
            if (e->key() == Qt::Key_Space && g != nullptr && m_tiempoFin < 0.0) g->saltar();
        }
        return;
    }
    QMainWindow::keyPressEvent(e);
}

void MainWindow::keyReleaseEvent(QKeyEvent* e) {
    if (!e->isAutoRepeat()) m_teclas.remove(e->key());
    QMainWindow::keyReleaseEvent(e);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
    // Pantallas con imagen: dibujar el fondo y ubicar los botones al cambiar el tamano.
    QWidget* pantallas[] = {m_pantMenu, m_pantDificultad, m_pantResultado};
    for (QWidget* pantalla : pantallas) {
        if (pantalla == nullptr || obj != pantalla) continue;
        if (ev->type() == QEvent::Paint) {
            dibujarFondo(pantalla);
            return true;
        }
        if (ev->type() == QEvent::Resize) ubicarHijos(pantalla);
    }
    // Las escenas se escalan para llenar toda la vista.
    if (ev->type() == QEvent::Resize && (obj == m_vistaJuego || obj == m_vistaIntro))
        ajustarVista(static_cast<QGraphicsView*>(obj), static_cast<QResizeEvent*>(ev)->size());
    if (m_vistaJuego != nullptr && obj == m_vistaJuego->viewport() && ev->type() == QEvent::MouseButtonPress && m_nivel &&
        m_reloj->isActive() && m_tiempoFin < 0.0) {
        const QMouseEvent* me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::LeftButton) {
            const QPointF destino = m_vistaJuego->mapToScene(me->position().toPoint());
            if (m_nivel->lanzarCocoJugador(destino)) m_escenaJuego->notificarLanzamiento(destino);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}