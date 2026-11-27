#include "escenadebatalla.h"
#include "armas.h"
#include "enemigorealista.h"
#include "germanquintero.h"
#include "niveldebatalla.h"
#include "nivelpuenteboyaca.h"
#include "nivelrioteatinos.h"
#include "objetodelescenario.h"
#include <QBrush>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QImage>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QPen>
#include <QTransform>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>
#include <utility>
#include <stdexcept>

namespace {
constexpr int kFrames = 4;               // cuadros por animacion
constexpr double kFpsQuieto = 4.0;
constexpr double kFpsMovimiento = 12.0;
constexpr double kTiempoLanzamiento = 0.35;
constexpr double kFraccionCamara = 0.33;  // German queda a un tercio de la pantalla
constexpr double kZFondo = -30.0, kZIntermedio = -20.0, kZTerreno = -10.0;
constexpr double kHundimientoSuelo = 16.0;  // Nivel 2: los pies se hunden un poco en el pasto del terreno
constexpr double kZObjeto = 1.0, kZEnemigo = 2.0, kZArma = 3.0, kZJugador = 4.0, kZHud = 100.0;
const char* const kDirs[] = {"der", "izq", "arriba", "abajo"};
}

EscenaDeBatalla::EscenaDeBatalla(const QSizeF& vista, QObject* parent)
    : QGraphicsScene(parent), m_tamano(vista), m_nivel(nullptr), m_animacion(new QVariantAnimation(this)),
      m_velo(nullptr), m_hudPanel(nullptr), m_hudVidaFondo(nullptr), m_hudVida(nullptr), m_hudTexto(nullptr),
      m_direccion(Direccion::Acercar), m_oscurecer(false), m_presentacion(false), m_camara(0.0), m_tLanz(0.0),
      m_dirLanz(0) {
    if (vista.width() <= 0 || vista.height() <= 0)
        throw std::invalid_argument("EscenaDeBatalla: tamano de vista invalido.");
    m_animacion->setDuration(2500);
    m_animacion->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_animacion, &QVariantAnimation::valueChanged, this, &EscenaDeBatalla::aplicarProgreso);
    connect(m_animacion, &QAbstractAnimation::finished, this, &EscenaDeBatalla::transicionTerminada);
    reiniciarEscena();
}

bool EscenaDeBatalla::operator==(const EscenaDeBatalla& o) const {
    return m_tamano == o.m_tamano && m_nivel == o.m_nivel && m_entradas.size() == o.m_entradas.size() &&
           m_capasZoom.size() == o.m_capasZoom.size() && m_capasParallax.size() == o.m_capasParallax.size() &&
           m_presentacion == o.m_presentacion && m_direccion == o.m_direccion && m_oscurecer == o.m_oscurecer &&
           std::fabs(m_camara - o.m_camara) < 1e-9 && m_animacion->duration() == o.m_animacion->duration();
}

void EscenaDeBatalla::reiniciarEscena() {
    m_animacion->stop();
    clear();  // borra todos los items de la escena
    m_entradas.clear();
    m_capasZoom.clear();
    m_capasParallax.clear();
    m_hudPanel = m_hudVidaFondo = m_hudVida = nullptr;
    m_hudTexto = nullptr;
    m_camara = 0.0;
    m_tLanz = 0.0;
    setSceneRect(0, 0, m_tamano.width(), m_tamano.height());
    m_velo = addRect(0, 0, m_tamano.width(), m_tamano.height(), QPen(Qt::NoPen), QBrush(Qt::black));
    m_velo->setZValue(1000);
    m_velo->setOpacity(0.0);
}

// ---------- recursos ----------

QPixmap EscenaDeBatalla::marcador(const QString& ruta, const QSize& tam) {
    // Si falta un sprite el juego no se cae: se dibuja un rectangulo de color con el nombre del archivo.
    const QSize t = (tam.width() > 0 && tam.height() > 0) ? tam : QSize(32, 32);
    QPixmap pm(t);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setBrush(QColor::fromHsv(static_cast<int>(qHash(ruta) % 360u), 150, 230, 190));
    p.setPen(QPen(Qt::white, 1));
    p.drawRect(0, 0, t.width() - 1, t.height() - 1);
    p.setPen(Qt::black);
    QFont f = p.font();
    f.setPixelSize(std::max(7, std::min(10, t.height() / 3)));
    p.setFont(f);
    p.drawText(QRect(0, 0, t.width(), t.height()), Qt::AlignCenter | Qt::TextWordWrap, ruta.section('/', -1));
    return pm;
}

QPixmap EscenaDeBatalla::cargar(const QStringList& rutas, const QSizeF& tam, Qt::AspectRatioMode modo) {
    const QSize t = tam.toSize();
    const QString clave = rutas.join(QLatin1Char('|')) + QStringLiteral("@%1x%2:%3").arg(t.width()).arg(t.height()).arg(static_cast<int>(modo));
    const auto it = m_cache.constFind(clave);
    if (it != m_cache.constEnd()) return it.value();
    QPixmap resultado;
    for (const QString& r : rutas) {
        if (!QFile::exists(r)) continue;
        const QPixmap pm(r);
        if (pm.isNull()) continue;
        // Pixel art pequeno: sin suavizar. Sprites muy grandes (German 279x580) se reducen con suavizado para no verse ruidosos.
        const QSize destino = pm.size().scaled(t, modo);  // tamano final real tras respetar la proporcion
        const bool reduccionFuerte = destino.height() * 5 < pm.height() * 4 || destino.width() * 5 < pm.width() * 4;  // reduce mas de 1.25x
        resultado = pm.scaled(t, modo, reduccionFuerte ? Qt::SmoothTransformation : Qt::FastTransformation);
        break;
    }
    if (resultado.isNull()) resultado = marcador(rutas.isEmpty() ? QString() : rutas.first(), t);
    m_cache.insert(clave, resultado);
    return resultado;
}

QPixmap EscenaDeBatalla::cargarAlto(const QString& ruta, double alto) {
    const QString clave = QStringLiteral("alto:%1@%2").arg(ruta).arg(static_cast<int>(alto));
    const auto it = m_cache.constFind(clave);
    if (it != m_cache.constEnd()) return it.value();
    QPixmap resultado;
    if (QFile::exists(ruta)) {
        const QPixmap pm(ruta);
        if (!pm.isNull()) resultado = pm.scaledToHeight(static_cast<int>(alto), Qt::SmoothTransformation);
    }
    if (resultado.isNull()) resultado = marcador(ruta, QSize(static_cast<int>(alto * 1.8), static_cast<int>(alto)));
    m_cache.insert(clave, resultado);
    return resultado;
}

// ---------- capas ----------

void EscenaDeBatalla::agregarCapaZoom(const QString& ruta, double zoomFinal, bool desvanecer) {
    const QPixmap pm = cargar({ruta}, m_tamano, Qt::IgnoreAspectRatio);
    QGraphicsPixmapItem* item = addPixmap(pm);
    item->setTransformOriginPoint(pm.width() / 2.0, pm.height() / 2.0);
    item->setZValue(static_cast<qreal>(m_capasZoom.size()));
    m_capasZoom.push_back({item, zoomFinal, desvanecer});
}

void EscenaDeBatalla::agregarCapaParallax(const QStringList& rutas, double factor, double z, double alto, double y,
                                          bool espejoAlterno) {
    CapaParallax c;
    c.factor = factor;
    c.y = y;
    for (const QString& r : rutas) c.imagenes.push_back(cargarAlto(r, alto));
    if (espejoAlterno && c.imagenes.size() == 1) c.imagenes.push_back(c.imagenes.front().transformed(QTransform::fromScale(-1.0, 1.0)));
    c.ancho = std::max(1, c.imagenes.front().width());
    const int cantidad = static_cast<int>(std::ceil(m_tamano.width() / c.ancho)) + 3;
    for (int k = 0; k < cantidad; ++k) {
        QGraphicsPixmapItem* t = addPixmap(c.imagenes.front());
        t->setZValue(z);
        c.teselas.push_back(t);
    }
    m_capasParallax.push_back(std::move(c));
}

// Fraccion (0..1) de la altura de la imagen donde empieza lo opaco: el borde superior del terreno.
double EscenaDeBatalla::fraccionBordeSuperior(const QString& ruta) {
    const QImage img(ruta);
    if (img.isNull() || img.height() <= 0) return 0.59;
    const int paso = 8;
    for (int y = 0; y < img.height(); ++y) {
        int opacos = 0, total = 0;
        for (int x = 0; x < img.width(); x += paso, ++total)
            if (qAlpha(img.pixel(x, y)) > 200) ++opacos;
        if (total > 0 && opacos * 2 > total) return static_cast<double>(y) / img.height();
    }
    return 0.59;
}

void EscenaDeBatalla::posicionarParallax(double camara) {
    for (CapaParallax& c : m_capasParallax) {
        const double desplazado = camara * c.factor;
        const long long base = static_cast<long long>(std::floor(desplazado / c.ancho));
        const double resto = desplazado - base * c.ancho;
        const long long n = static_cast<long long>(c.imagenes.size());
        for (std::size_t k = 0; k < c.teselas.size(); ++k) {
            const long long idx = (((base + static_cast<long long>(k) - 1) % n) + n) % n;  // A, B, A, B...
            c.teselas[k]->setPixmap(c.imagenes[static_cast<std::size_t>(idx)]);
            c.teselas[k]->setPos(camara - resto + (static_cast<double>(k) - 1.0) * c.ancho, c.y);
        }
    }
}

// ---------- modos de la escena ----------

void EscenaDeBatalla::mostrarNivel(NivelDeBatalla* nivel) {
    if (nivel == nullptr) throw std::invalid_argument("EscenaDeBatalla: el nivel no puede ser nulo.");
    reiniciarEscena();
    m_nivel = nivel;
    m_presentacion = false;
    if (const auto* puenteN2 = dynamic_cast<NivelPuenteBoyaca*>(nivel)) {
        // Nivel 2: tres capas, de la mas lenta (montanas azules) a la mas rapida (el terreno de German).
        const double H = m_tamano.height();
        const double suelo = puenteN2->sueloY();
        agregarCapaParallax({QStringLiteral(":/sprites/nivel2/CapaFondoA.png"), QStringLiteral(":/sprites/nivel2/CapaFondoB.png")}, 0.15, kZFondo, H, 0.0);
        // La capa intermedia baja un poco para que el bosque quede apoyado detras del terreno.
        agregarCapaParallax({QStringLiteral(":/sprites/nivel2/CapaInterA.png"), QStringLiteral(":/sprites/nivel2/CapaInterB.png")}, 0.40, kZIntermedio, H, 85.0);
        // La capa frontal baja mucho: su borde superior (pasto y piedras) es el piso donde camina German.
        const QString frontal = QStringLiteral(":/sprites/nivel2/CapaAdelante.png");
        const double altoFrontal = H * 1.5;
        const double bordeY = fraccionBordeSuperior(frontal) * altoFrontal;
        agregarCapaParallax({frontal}, 1.0, kZTerreno, altoFrontal, suelo - bordeY + 6.0, true);
    } else {
        QGraphicsPixmapItem* fondo = addPixmap(cargar({QStringLiteral(":/sprites/nivel1/fondo_nivel1.png")}, m_tamano, Qt::IgnoreAspectRatio));
        fondo->setZValue(kZFondo);
    }
    crearHud();
    actualizarVista(0.0);
}

void EscenaDeBatalla::mostrarPresentacion() {
    reiniciarEscena();
    m_nivel = nullptr;
    m_presentacion = true;
    agregarCapaZoom(QStringLiteral(":/sprites/nivel2/CapaFondoA.png"), 1.10, false);
    agregarCapaZoom(QStringLiteral(":/sprites/nivel2/CapaInterA.png"), 1.40, false);
    agregarCapaZoom(QStringLiteral(":/sprites/nivel2/CapaAdelante.png"), 2.20, false);
    aplicarProgreso(0.0);
}

// ---------- transicion multiplano (zoom por capas) ----------

void EscenaDeBatalla::iniciarTransicion(Direccion direccion, int duracionMs, bool oscurecer) {
    if (m_capasZoom.empty()) throw std::logic_error("EscenaDeBatalla: no hay capas para la transicion.");
    if (duracionMs <= 0) throw std::invalid_argument("EscenaDeBatalla: duracion invalida.");
    m_animacion->stop();
    m_animacion->setDuration(duracionMs);
    m_direccion = direccion;
    m_oscurecer = oscurecer;
    const double inicio = (direccion == Direccion::Acercar) ? 0.0 : 1.0;
    m_animacion->setStartValue(inicio);
    m_animacion->setEndValue(1.0 - inicio);
    aplicarProgreso(inicio);
    m_animacion->start();
}

void EscenaDeBatalla::detenerTransicion() { m_animacion->stop(); }
bool EscenaDeBatalla::transicionActiva() const { return m_animacion->state() == QAbstractAnimation::Running; }

void EscenaDeBatalla::aplicarProgreso(const QVariant& valor) {
    const double t = valor.toDouble();
    for (const CapaZoom& c : m_capasZoom) {
        c.item->setScale(1.0 + (c.zoomFinal - 1.0) * t);
        c.item->setOpacity(c.desvanecer ? 1.0 - t * t : 1.0);
    }
    double oscuridad = 0.0;
    if (m_oscurecer) oscuridad = (m_direccion == Direccion::Alejar) ? 1.0 - t : t;
    if (m_velo != nullptr) m_velo->setOpacity(oscuridad);
}

// ---------- sprites de German segun desgaste, poder, accion y direccion ----------

QStringList EscenaDeBatalla::rutasGerman(const GermanQuintero& g, Entrada& en, double dt, bool& espejo) {
    const bool cenital = g.modo() == GermanQuintero::Modo::Cenital;
    const QPointF d = g.direccion();

    // Hacia donde mira
    if (m_tLanz > 0.0) {
        en.dir = m_dirLanz;
    } else if (cenital) {
        if (std::fabs(d.x()) > 0.01 || std::fabs(d.y()) > 0.01) {
            if (std::fabs(d.x()) >= std::fabs(d.y())) en.dir = d.x() >= 0.0 ? 0 : 1;
            else en.dir = d.y() < 0.0 ? 2 : 3;
        }
    } else if (std::fabs(d.x()) > 0.01) {
        en.dir = d.x() > 0.0 ? 0 : 1;
    }
    if (!cenital && en.dir > 1) en.dir = 0;
    const QString dir = QString::fromLatin1(kDirs[en.dir]);

    // Estado de desgaste (siempre) y poder activo (si lo hay)
    QString desgaste;
    switch (g.desgaste()) {
    case GermanQuintero::Desgaste::Normal: desgaste = QStringLiteral("normal"); break;
    case GermanQuintero::Desgaste::Leve: desgaste = QStringLiteral("leve"); break;
    case GermanQuintero::Desgaste::Medio: desgaste = QStringLiteral("medio"); break;
    case GermanQuintero::Desgaste::Fuerte: desgaste = QStringLiteral("fuerte"); break;
    case GermanQuintero::Desgaste::Fantasma: desgaste = QStringLiteral("fantasma"); break;
    }
    QString estado = desgaste;
    if (g.especialidad() == GermanQuintero::Especialidad::MachoAlfa) estado = QStringLiteral("macho");
    else if (g.especialidad() == GermanQuintero::Especialidad::CrestaVeloz) estado = QStringLiteral("cresta");

    // Accion
    QString anim = QStringLiteral("idle");
    const double vel = cenital ? std::hypot(g.velocidad().x(), g.velocidad().y()) : std::fabs(g.velocidad().x());
    const bool enAire = !cenital && !g.enSuelo();
    if (m_tLanz > 0.0) anim = QStringLiteral("lanz");
    else if (vel > 5.0 || enAire) anim = QStringLiteral("mov");

    int frame = 0;
    en.tAnim += dt;
    if (anim == QLatin1String("lanz")) frame = std::min(kFrames - 1, static_cast<int>((1.0 - m_tLanz / kTiempoLanzamiento) * kFrames));
    else if (enAire) frame = g.velocidad().y() < 0.0 ? 1 : 3;  // en el aire: pose de zancada, subiendo / bajando
    else frame = static_cast<int>(en.tAnim * (anim == QLatin1String("mov") ? kFpsMovimiento : kFpsQuieto)) % kFrames;

    QStringList rutas;
    if (desgaste == QLatin1String("fantasma")) {
        rutas << QStringLiteral(":/sprites/german/german_fantasma_%1.png").arg(frame)
              << QStringLiteral(":/sprites/nivel1/german_fantasma.png");
        return rutas;
    }
    // Si existen sprites de salto propios (german_<estado>_salto_<dir>_<0 sube | 1 baja>.png) se usan primero.
    if (enAire) {
        const int fs = g.velocidad().y() < 0.0 ? 0 : 1;
        rutas << QStringLiteral(":/sprites/german/german_%1_salto_%2_%3.png").arg(estado, dir).arg(fs);
        if (desgaste != estado) rutas << QStringLiteral(":/sprites/german/german_%1_salto_%2_%3.png").arg(desgaste, dir).arg(fs);
    }
    // Idle lateral: no hay version hacia la izquierda, se espeja la de la derecha
    espejo = !cenital && anim == QLatin1String("idle") && en.dir == 1;

    auto nueva = [&](const QString& est) {
        if (anim == QLatin1String("idle")) return QStringLiteral(":/sprites/german/german_%1_idle_%2.png").arg(est).arg(frame);
        return QStringLiteral(":/sprites/german/german_%1_%2_%3_%4.png").arg(est, anim, dir).arg(frame);
    };
    rutas << nueva(estado);
    if (desgaste != estado) rutas << nueva(desgaste);  // poder sin sprite propio (ej. lanzar): usa el del desgaste
    if (desgaste != QLatin1String("normal")) rutas << nueva(QStringLiteral("normal"));
    if (anim != QLatin1String("idle"))
        rutas << QStringLiteral(":/sprites/nivel1/german_%1_%2_%3.png").arg(anim, dir).arg(frame);  // sprites del Nivel 1 originales
    rutas << g.rutaSprite();
    return rutas;
}

// ---------- sincronizacion con el nivel ----------

EscenaDeBatalla::Entrada& EscenaDeBatalla::obtener(ElementoDeBatalla* e) {
    auto it = m_entradas.find(e);
    if (it != m_entradas.end()) return it->second;
    Entrada n;
    n.item = addPixmap(QPixmap());
    double z = kZObjeto;
    if (dynamic_cast<GermanQuintero*>(e) != nullptr) z = kZJugador;
    else if (dynamic_cast<Armas*>(e) != nullptr) z = kZArma;
    else if (dynamic_cast<EnemigoRealista*>(e) != nullptr) z = kZEnemigo;
    n.item->setZValue(z);
    return m_entradas.emplace(e, n).first->second;
}

void EscenaDeBatalla::sincronizar(ElementoDeBatalla* e, double dt) {
    Entrada& en = obtener(e);
    const GermanQuintero* g = dynamic_cast<GermanQuintero*>(e);
    const EnemigoRealista* enemigo = dynamic_cast<EnemigoRealista*>(e);
    const ObjetoDelEscenario* objeto = dynamic_cast<ObjetoDelEscenario*>(e);
    bool visible = e->esVisible();
    bool espejo = false;
    QStringList rutas;
    Qt::AspectRatioMode modo = Qt::KeepAspectRatio;
    if (g != nullptr) {
        visible = true;  // si muere se ve el fantasma
        rutas = rutasGerman(*g, en, dt, espejo);
    } else {
        rutas << e->rutaSprite();
        espejo = e->espejar();
        if (dynamic_cast<ObjetoDelEscenario*>(e) != nullptr) modo = Qt::IgnoreAspectRatio;
    }
    en.item->setVisible(visible);
    if (visible && objeto != nullptr && objeto->tipo() == TipoObjeto::TrampaPendulo) {
        // Demoledora: el sprite es cadena + bola; cuelga de la base y gira con el angulo del pendulo.
        const double r = objeto->tamano().x() / 2.0;
        const QPixmap pmP = cargar(rutas, QSizeF(2.0 * r, objeto->longitudCuerda() + r), Qt::IgnoreAspectRatio);
        if (en.clavePixmap != pmP.cacheKey()) {
            en.item->setPixmap(pmP);
            en.clavePixmap = pmP.cacheKey();
        }
        const QPointF base = objeto->posicionBase();
        const QPointF d = objeto->posicion() - base;
        const double kPiEsc = 3.14159265358979323846;
        en.item->setTransform(QTransform());
        en.item->setTransformOriginPoint(pmP.width() / 2.0, 0.0);
        en.item->setRotation(-std::atan2(d.x(), d.y()) * 180.0 / kPiEsc);  // + = a la derecha
        en.item->setPos(base.x() - pmP.width() / 2.0, base.y());
        en.item->setOpacity(1.0);
        return;
    }
    if (!visible) {
        if (en.barraFondo != nullptr) {
            en.barraFondo->setVisible(false);
            en.barra->setVisible(false);
        }
        return;
    }

    QSizeF caja(e->tamano().x(), e->tamano().y());  // el tamano del elemento se guarda como QPointF
    // German, el soldado y Abelardo tienen cuadros de distinto ancho (poses): se escalan por la altura para que el
    // personaje no cambie de tamano mientras camina.
    if (g != nullptr || (enemigo != nullptr && enemigo->esHumanoide() && enemigo->estaActivo())) caja.setWidth(caja.width() * 4.0);
    const QPixmap pm = cargar(rutas, caja, modo);
    if (en.clavePixmap != pm.cacheKey()) {
        en.item->setPixmap(pm);
        en.clavePixmap = pm.cacheKey();
    }
    const double alt = e->altura();
    const bool apoyadoEnPies = (g != nullptr) || dynamic_cast<EnemigoRealista*>(e) != nullptr;
    const double x = e->posicion().x() - pm.width() / 2.0;
    // Nivel 2: lo que esta sobre el suelo (German, enemigos, cocos) se baja un poco para que se vea pisando la tierra.
    // En una plataforma o en el aire no se baja (se reduce gradualmente en los primeros 10 px de un salto).
    double hundimiento = 0.0;
    const auto* nivelPuente = dynamic_cast<NivelPuenteBoyaca*>(m_nivel);
    if (nivelPuente != nullptr && (g != nullptr || (enemigo != nullptr && enemigo->tipo() != TipoEnemigo::Carreta) ||
                                   (objeto != nullptr && objeto->tipo() == TipoObjeto::CocoEnSuelo))) {
        const double sobre = nivelPuente->sueloY() - (e->posicion().y() + e->tamano().y() / 2.0);
        hundimiento = kHundimientoSuelo * std::max(0.0, std::min(1.0, 1.0 - sobre / 10.0));
    }
    const double y = (apoyadoEnPies ? e->posicion().y() + e->tamano().y() / 2.0 - pm.height() - alt
                                    : e->posicion().y() - pm.height() / 2.0 - alt) + hundimiento;
    en.item->setPos(x, y);
    if (enemigo != nullptr && enemigo->tipo() != TipoEnemigo::Canon) actualizarBarraVida(en, *enemigo, x, y, pm.width());
    en.item->setTransformOriginPoint(pm.width() / 2.0, pm.height() / 2.0);
    en.item->setTransform(QTransform::fromScale(espejo ? -1.0 : 1.0, 1.0));
    en.item->setRotation(e->rotacion());
    en.item->setOpacity(e->opacidad());
    // Los craters van debajo de todo; impactos y explosiones, encima de los enemigos.
    if (const auto* obj = dynamic_cast<const ObjetoDelEscenario*>(e); obj != nullptr && obj->tipo() == TipoObjeto::Efecto)
        en.item->setZValue(obj->variante() >= static_cast<int>(TipoEfecto::CraterPasto) &&
                                   obj->variante() <= static_cast<int>(TipoEfecto::CraterPiedra) ? kZObjeto - 0.5 : kZArma + 0.5);
}

// Barra de vida sobre carretas, canones, soldados y Abelardo (el canon del Nivel 1 no se puede danar).
void EscenaDeBatalla::actualizarBarraVida(Entrada& en, const EnemigoRealista& enemigo, double xSprite, double ySprite, double anchoSprite) {
    if (en.barraFondo == nullptr) {
        en.barraFondo = addRect(0, 0, 10, 6, QPen(Qt::black, 1), QBrush(QColor(60, 15, 15)));
        en.barra = addRect(0, 0, 10, 6, QPen(Qt::NoPen), QBrush(QColor(70, 200, 90)));
        en.barraFondo->setZValue(kZHud - 10);
        en.barra->setZValue(kZHud - 9);
    }
    const bool mostrar = enemigo.estaActivo();
    en.barraFondo->setVisible(mostrar);
    en.barra->setVisible(mostrar);
    if (!mostrar) return;
    const double ancho = std::max(44.0, std::min(110.0, anchoSprite));
    const double ratio = std::max(0.0, std::min(1.0, enemigo.saludRelativa()));
    const double x = xSprite + anchoSprite / 2.0 - ancho / 2.0;
    const double y = ySprite - 12.0;
    en.barraFondo->setRect(0, 0, ancho, 6);
    en.barraFondo->setPos(x, y);
    en.barra->setRect(0, 0, ancho * ratio, 6);
    en.barra->setPos(x, y);
    en.barra->setBrush(QBrush(ratio > 0.5 ? QColor(70, 200, 90) : (ratio > 0.25 ? QColor(235, 200, 50) : QColor(220, 60, 50))));
}

void EscenaDeBatalla::notificarLanzamiento(const QPointF& destino) {
    m_tLanz = kTiempoLanzamiento;
    if (m_nivel == nullptr || m_nivel->jugador() == nullptr) return;
    const GermanQuintero* g = m_nivel->jugador();
    const QPointF d = destino - g->posicion();
    if (g->modo() == GermanQuintero::Modo::Cenital && std::fabs(d.y()) > std::fabs(d.x())) m_dirLanz = d.y() < 0.0 ? 2 : 3;
    else m_dirLanz = d.x() >= 0.0 ? 0 : 1;
}

void EscenaDeBatalla::actualizarVista(double dt) {
    if (m_nivel == nullptr) return;
    if (m_tLanz > 0.0) m_tLanz = std::max(0.0, m_tLanz - dt);

    if (const auto* puente = dynamic_cast<NivelPuenteBoyaca*>(m_nivel)) {
        const double x = m_nivel->jugador() != nullptr ? m_nivel->jugador()->posicion().x() : 0.0;
        const double maxCam = std::max(0.0, puente->anchoMundo() - m_tamano.width());
        m_camara = std::max(0.0, std::min(x - m_tamano.width() * kFraccionCamara, maxCam));
        setSceneRect(m_camara, 0, m_tamano.width(), m_tamano.height());
        posicionarParallax(m_camara);
        if (m_velo != nullptr) m_velo->setPos(m_camara, 0);
    }

    if (m_nivel->jugador() != nullptr) sincronizar(m_nivel->jugador(), dt);
    for (Armas* a : m_nivel->armas()) sincronizar(a, dt);
    for (ObjetoDelEscenario* o : m_nivel->objetosEscenario()) sincronizar(o, dt);
    for (EnemigoRealista* en : m_nivel->enemigos()) sincronizar(en, dt);
    actualizarHud();
}

// ---------- HUD ----------

void EscenaDeBatalla::crearHud() {
    m_hudPanel = addRect(0, 0, m_tamano.width(), 34, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 150)));
    m_hudPanel->setZValue(kZHud);
    m_hudVidaFondo = addRect(0, 0, 150, 12, QPen(Qt::white), QBrush(QColor(90, 20, 20)));
    m_hudVidaFondo->setZValue(kZHud + 1);
    m_hudVida = addRect(0, 0, 150, 12, QPen(Qt::NoPen), QBrush(QColor(70, 200, 90)));
    m_hudVida->setZValue(kZHud + 2);
    m_hudTexto = addSimpleText(QString());
    QFont f = m_hudTexto->font();
    f.setBold(true);
    f.setPixelSize(14);
    m_hudTexto->setFont(f);
    m_hudTexto->setBrush(QBrush(Qt::white));
    m_hudTexto->setZValue(kZHud + 1);
}

void EscenaDeBatalla::actualizarHud() {
    if (m_hudTexto == nullptr || m_nivel == nullptr) return;
    const GermanQuintero* g = m_nivel->jugador();
    QStringList partes;
    partes << QStringLiteral("Tiempo %1 s").arg(static_cast<int>(std::ceil(m_nivel->tiempoRestante())));
    if (g != nullptr) {
        partes << QStringLiteral("Cocos %1/%2").arg(g->cocos()).arg(g->capacidadCocos());
        if (g->especialidad() == GermanQuintero::Especialidad::MachoAlfa)
            partes << QStringLiteral("MACHO ALFA %1 s").arg(static_cast<int>(std::ceil(g->tiempoEspecialidad())));
        else if (g->especialidad() == GermanQuintero::Especialidad::CrestaVeloz)
            partes << QStringLiteral("CRESTA VELOZ %1 s").arg(static_cast<int>(std::ceil(g->tiempoEspecialidad())));
    }
    if (const auto* rio = dynamic_cast<NivelRioTeatinos*>(m_nivel)) {
        partes << QStringLiteral("Carretas %1/%2").arg(rio->carretasDestruidas()).arg(rio->carretasObjetivo())
               << QStringLiteral("Cruzaron %1/%2").arg(rio->carretasCruzaron()).arg(rio->maxCruzadas());
    } else if (const auto* puente = dynamic_cast<NivelPuenteBoyaca*>(m_nivel)) {
        const double meta = std::max(1.0, puente->metaX());
        const double x = g != nullptr ? g->posicion().x() : 0.0;
        partes << QStringLiteral("Avance %1 %").arg(static_cast<int>(std::min(100.0, 100.0 * x / meta)));
        for (const EnemigoRealista* e : puente->enemigos())
            if (e->esJefeFinal() && e->estaActivo())
                partes << QStringLiteral("Abelardo %1 %").arg(static_cast<int>(100.0 * e->saludRelativa()));
    }
    m_hudTexto->setText(partes.join(QStringLiteral("    ")));
    m_hudPanel->setPos(m_camara, 0);
    m_hudTexto->setPos(m_camara + 170, 8);
    m_hudVidaFondo->setPos(m_camara + 10, 11);
    m_hudVida->setPos(m_camara + 10, 11);
    const double vida = g != nullptr ? std::max(0.0, std::min(1.0, g->integridad() / 100.0)) : 0.0;
    m_hudVida->setRect(0, 0, 150 * vida, 12);
}

void EscenaDeBatalla::soltarNivel() { m_nivel = nullptr; }
NivelDeBatalla* EscenaDeBatalla::nivel() const { return m_nivel; }
QSizeF EscenaDeBatalla::tamanoVista() const { return m_tamano; }
double EscenaDeBatalla::camaraIzquierda() const { return m_camara; }
