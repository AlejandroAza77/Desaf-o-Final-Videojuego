#ifndef ESCENADEBATALLA_H
#define ESCENADEBATALLA_H

#include <QGraphicsScene>
#include <QHash>
#include <QPixmap>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <unordered_map>
#include <vector>

class ElementoDeBatalla;
class EnemigoRealista;
class GermanQuintero;
class NivelDeBatalla;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
class QVariantAnimation;

// Escena grafica: consulta un NivelDeBatalla (no es su duena) y dibuja un QGraphicsPixmapItem por
// elemento. Tambien anima la transicion multiplano (zoom por capas) y el scroll con parallax del Nivel 2.
class EscenaDeBatalla : public QGraphicsScene {
    Q_OBJECT
public:
    enum class Direccion { Acercar, Alejar };

    explicit EscenaDeBatalla(const QSizeF& vista = QSizeF(960, 480), QObject* parent = nullptr);
    ~EscenaDeBatalla() override = default;

    // Una escena es un QObject: no se copia. == y != comparan su configuracion.
    EscenaDeBatalla(const EscenaDeBatalla&) = delete;
    EscenaDeBatalla& operator=(const EscenaDeBatalla&) = delete;
    bool operator==(const EscenaDeBatalla& otro) const;
    bool operator!=(const EscenaDeBatalla& otro) const { return !(*this == otro); }

    void mostrarNivel(NivelDeBatalla* nivel);  // el nivel sigue siendo de quien lo creo
    void mostrarPresentacion();                // capas con zoom, sin nivel
    void actualizarVista(double dt);           // sincroniza items, animacion, camara y HUD
    void notificarLanzamiento(const QPointF& destino);
    void soltarNivel();                        // deja de consultar el nivel (antes de que se destruya)

    void iniciarTransicion(Direccion direccion, int duracionMs = 2500, bool oscurecer = false);
    void detenerTransicion();
    bool transicionActiva() const;

    NivelDeBatalla* nivel() const;
    QSizeF tamanoVista() const;
    double camaraIzquierda() const;

signals:
    void transicionTerminada();

private slots:
    void aplicarProgreso(const QVariant& valor);

private:
    struct Entrada {
        QGraphicsPixmapItem* item = nullptr;
        qint64 clavePixmap = 0;
        double tAnim = 0.0;
        int dir = 0;  // 0 der, 1 izq, 2 arriba, 3 abajo
        QGraphicsRectItem* barraFondo = nullptr;  // barra de vida de los enemigos
        QGraphicsRectItem* barra = nullptr;
    };
    struct CapaZoom {
        QGraphicsPixmapItem* item;
        double zoomFinal;
        bool desvanecer;
    };
    struct CapaParallax {
        std::vector<QPixmap> imagenes;                // 1 o 2 imagenes que se alternan
        std::vector<QGraphicsPixmapItem*> teselas;
        double factor = 1.0;                          // 1 = se mueve con el mundo; menor = mas lenta
        double ancho = 1.0;
        double y = 0.0;                               // y del borde superior de las teselas
    };

    QSizeF m_tamano;
    NivelDeBatalla* m_nivel;
    QVariantAnimation* m_animacion;
    QGraphicsRectItem* m_velo;
    QGraphicsRectItem* m_hudPanel;
    QGraphicsRectItem* m_hudVidaFondo;
    QGraphicsRectItem* m_hudVida;
    QGraphicsSimpleTextItem* m_hudTexto;
    std::vector<CapaZoom> m_capasZoom;
    std::vector<CapaParallax> m_capasParallax;
    std::unordered_map<const ElementoDeBatalla*, Entrada> m_entradas;
    QHash<QString, QPixmap> m_cache;
    Direccion m_direccion;
    bool m_oscurecer;
    bool m_presentacion;
    double m_camara;
    double m_tLanz;
    int m_dirLanz;

    void reiniciarEscena();
    void crearHud();
    void actualizarHud();
    void agregarCapaZoom(const QString& ruta, double zoomFinal, bool desvanecer);
    // alto: alto en pantalla de la imagen; y: donde queda su borde superior; espejoAlterno: para una imagen que no empata
    // consigo misma, la segunda tesela es su espejo (asi nunca se nota la costura).
    void agregarCapaParallax(const QStringList& rutas, double factor, double z, double alto, double y, bool espejoAlterno = false);
    static double fraccionBordeSuperior(const QString& ruta);
    void posicionarParallax(double camara);
    void sincronizar(ElementoDeBatalla* e, double dt);
    Entrada& obtener(ElementoDeBatalla* e);
    void actualizarBarraVida(Entrada& en, const EnemigoRealista& enemigo, double xSprite, double ySprite, double anchoSprite);
    QStringList rutasGerman(const GermanQuintero& g, Entrada& en, double dt, bool& espejo);
    QPixmap cargar(const QStringList& rutas, const QSizeF& tam, Qt::AspectRatioMode modo);
    QPixmap cargarAlto(const QString& ruta, double alto);
    static QPixmap marcador(const QString& ruta, const QSize& tam);
};

#endif // ESCENADEBATALLA_H
