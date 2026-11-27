#include "enemigorealista.h"
#include <algorithm>

namespace {
double vidaPorTipo(TipoEnemigo t) {
    switch (t) {
    case TipoEnemigo::Carreta: return 50.0;
    case TipoEnemigo::Canon: return 100.0;
    case TipoEnemigo::Centinela: return 60.0;
    case TipoEnemigo::Comandante: return 150.0;
    case TipoEnemigo::EnemigoFinal: return 250.0;
    }
    return 50.0;
}

QPointF tamanoPorTipo(TipoEnemigo t) {
    switch (t) {
    case TipoEnemigo::Carreta: return QPointF(70, 45);
    case TipoEnemigo::Canon: return QPointF(66, 45);
    case TipoEnemigo::Centinela: return QPointF(72, 48);     // canon (sprite 339x229)
    case TipoEnemigo::Comandante: return QPointF(54, 70);    // soldado
    case TipoEnemigo::EnemigoFinal: return QPointF(90, 100); // Abelardo con bazuca
    }
    return QPointF(50, 50);
}

// Cuanto tiempo se siguen viendo los restos de un enemigo destruido (0 = desaparece de una vez).
double duracionRestos(TipoEnemigo t) {
    switch (t) {
    case TipoEnemigo::Carreta: return 5.0;
    case TipoEnemigo::Canon:
    case TipoEnemigo::Centinela: return 3.5;
    case TipoEnemigo::EnemigoFinal: return 1e9;  // Abelardo queda tendido
    case TipoEnemigo::Comandante: return 0.0;
    }
    return 0.0;
}
constexpr double kTiempoColapso = 0.7;  // la carreta se desarma antes de quedar en escombros
constexpr double kTiempoCeniza = 1.2;   // ultimo tramo: los restos se desvanecen en cenizas

bool llevaAgente(TipoEnemigo t) {
    return t == TipoEnemigo::Centinela || t == TipoEnemigo::Comandante || t == TipoEnemigo::EnemigoFinal;
}

double intervaloPorTipo(TipoEnemigo t) {
    if (t == TipoEnemigo::EnemigoFinal) return 1.8;  // el cohete de Abelardo tarda en recargarse
    if (t == TipoEnemigo::Centinela) return 2.0;
    return llevaAgente(t) ? 1.6 : 3.0;
}

void validarAgente(TipoEnemigo t, const AgenteInteligente* a) {
    if (llevaAgente(t) && a == nullptr) throw std::invalid_argument("El centinela, el comandante y Abelardo necesitan un agente.");
    if (!llevaAgente(t) && a != nullptr) throw std::invalid_argument("La carreta y el canon no llevan agente.");
}

constexpr double kGravedad = 900.0;
constexpr double kTiempoVueloCanon = 1.7;
constexpr double kDanoCanon = 25.0;
constexpr double kTiempoVueloSoldado = 1.1;
constexpr double kDanoSoldado = 25.0;   // 4 disparos de 25 matan a German (vida 100)
constexpr double kTiempoVueloCohete = 0.9;
constexpr double kGravedadCohete = 250.0;  // el cohete va casi recto
constexpr double kDanoAbelardo = 2.0 * kDanoSoldado;  // 2 vidas por impacto
constexpr double kFpsCaminata = 10.0;
constexpr double kGravedadJefe = 2240.0;   // 1400 * 1.6: igual que German en el Nivel 2
constexpr double kSaltoJefe = 900.0;       // salta ~180 px, como German
constexpr double kAlcanceCohete = 1000.0;  // Abelardo dispara desde lejos
// Velocidad (px/s) de los disparos directos de vista lateral.
constexpr double kVelBalaCentinela = 520.0;
constexpr double kVelBalaSoldado = 700.0;
constexpr double kVelCohete = 480.0;
constexpr double kDistanciaDisparo = 60.0;  // a esta distancia en x deja de acercarse y solo dispara
}

EnemigoRealista::EnemigoRealista(TipoEnemigo tipo, const QPointF& pos, AgenteInteligente* agente,
                                 const QPointF& meta, double rapidez, double intervaloDisparo)
    : ElementoDeBatalla(pos, QPointF(0, 0), tamanoPorTipo(tipo), vidaPorTipo(tipo)), m_tipo(tipo),
    m_agente(nullptr), m_meta(meta), m_rapidez(rapidez), m_cruzo(false),
    m_intervaloDisparo(intervaloDisparo == 0.0 ? intervaloPorTipo(tipo) : intervaloDisparo),
    m_tiempoDisparo(llevaAgente(tipo) ? 0.0 : m_intervaloDisparo), m_saludMaxima(vidaPorTipo(tipo)), m_objetivo(pos), m_quiereDisparar(false), m_tAnim(0.0), m_variante(0), m_tMuerte(0.0), m_muerteIniciada(false),
    m_danoDisparo(tipo == TipoEnemigo::EnemigoFinal ? kDanoAbelardo : (tipo == TipoEnemigo::Canon ? kDanoCanon : kDanoSoldado)),
    m_persiguiendo(false), m_suelo(pos.y() + tamanoPorTipo(tipo).y() / 2.0), m_enSuelo(true) {
    validarAgente(tipo, agente);
    if (rapidez < 0.0 || (tipo == TipoEnemigo::Carreta && rapidez == 0.0))
        throw std::invalid_argument("La rapidez debe ser positiva (una carreta debe moverse).");
    if (intervaloDisparo < 0.0) throw std::invalid_argument("El intervalo de disparo no puede ser negativo.");
    m_agente = agente;
}

EnemigoRealista::EnemigoRealista(const EnemigoRealista& otro)
    : ElementoDeBatalla(otro), m_tipo(otro.m_tipo),
    m_agente(otro.m_agente ? new AgenteInteligente(*otro.m_agente) : nullptr), m_meta(otro.m_meta),
    m_rapidez(otro.m_rapidez), m_cruzo(otro.m_cruzo), m_intervaloDisparo(otro.m_intervaloDisparo),
    m_tiempoDisparo(otro.m_tiempoDisparo), m_saludMaxima(otro.m_saludMaxima), m_objetivo(otro.m_objetivo),
    m_quiereDisparar(otro.m_quiereDisparar), m_tAnim(otro.m_tAnim), m_variante(otro.m_variante),
    m_tMuerte(otro.m_tMuerte), m_muerteIniciada(otro.m_muerteIniciada), m_danoDisparo(otro.m_danoDisparo),
    m_persiguiendo(otro.m_persiguiendo), m_suelo(otro.m_suelo), m_enSuelo(otro.m_enSuelo) {}

EnemigoRealista::~EnemigoRealista() { delete m_agente; }

EnemigoRealista& EnemigoRealista::operator=(const EnemigoRealista& otro) {
    if (this == &otro) return *this;
    AgenteInteligente* copia = otro.m_agente ? new AgenteInteligente(*otro.m_agente) : nullptr;
    ElementoDeBatalla::operator=(otro);
    delete m_agente;
    m_agente = copia;
    m_tipo = otro.m_tipo;
    m_meta = otro.m_meta;
    m_rapidez = otro.m_rapidez;
    m_cruzo = otro.m_cruzo;
    m_intervaloDisparo = otro.m_intervaloDisparo;
    m_tiempoDisparo = otro.m_tiempoDisparo;
    m_saludMaxima = otro.m_saludMaxima;
    m_objetivo = otro.m_objetivo;
    m_quiereDisparar = otro.m_quiereDisparar;
    m_tAnim = otro.m_tAnim;
    m_variante = otro.m_variante;
    m_tMuerte = otro.m_tMuerte;
    m_muerteIniciada = otro.m_muerteIniciada;
    m_danoDisparo = otro.m_danoDisparo;
    m_persiguiendo = otro.m_persiguiendo;
    m_suelo = otro.m_suelo;
    m_enSuelo = otro.m_enSuelo;
    return *this;
}

bool EnemigoRealista::operator==(const EnemigoRealista& o) const {
    if (!ElementoDeBatalla::operator==(o) || m_tipo != o.m_tipo || (m_agente == nullptr) != (o.m_agente == nullptr) ||
        (m_agente && !(*m_agente == *o.m_agente)))
        return false;
    return m_meta == o.m_meta && casiIgual(m_rapidez, o.m_rapidez) && m_cruzo == o.m_cruzo &&
           casiIgual(m_intervaloDisparo, o.m_intervaloDisparo) && casiIgual(m_tiempoDisparo, o.m_tiempoDisparo) &&
           casiIgual(m_saludMaxima, o.m_saludMaxima) && m_objetivo == o.m_objetivo &&
           m_quiereDisparar == o.m_quiereDisparar && casiIgual(m_tAnim, o.m_tAnim) &&
           m_variante == o.m_variante && casiIgual(m_tMuerte, o.m_tMuerte) && m_muerteIniciada == o.m_muerteIniciada &&
           casiIgual(m_danoDisparo, o.m_danoDisparo) && m_persiguiendo == o.m_persiguiendo &&
           casiIgual(m_suelo, o.m_suelo) && m_enSuelo == o.m_enSuelo;
}

void EnemigoRealista::actualizar(double dt) {
    if (!estaActivo()) {
        // Destruido por daño (no por cruzar el puente): corre el reloj de sus restos.
        if (duracionRestos(m_tipo) > 0.0 && m_integridad <= 0.0) {
            m_muerteIniciada = true;
            m_tMuerte += dt;
        }
        return;
    }
    m_tiempoDisparo = std::max(0.0, m_tiempoDisparo - dt);
    m_tAnim += dt;
    switch (m_tipo) {
    case TipoEnemigo::Carreta: avanzar(dt); break;
    case TipoEnemigo::Canon: m_velocidad = QPointF(0, 0); break;
    case TipoEnemigo::Centinela:
    case TipoEnemigo::Comandante:
    case TipoEnemigo::EnemigoFinal: pensar(dt); break;
    }
}

void EnemigoRealista::avanzar(double dt) {
    if (m_cruzo) {
        m_velocidad = QPointF(0, 0);
        return;
    }
    const QPointF v = m_meta - m_posicion;
    const double dist = std::hypot(v.x(), v.y());
    if (dist <= m_rapidez * dt) {
        m_posicion = m_meta;
        m_velocidad = QPointF(0, 0);
        m_cruzo = true;
    } else {
        m_velocidad = v * (m_rapidez / dist);
        m_posicion += m_velocidad * dt;
    }
}

void EnemigoRealista::pensar(double dt) {
    const bool jefe = (m_tipo == TipoEnemigo::EnemigoFinal);
    // El agente solo decide cada 'retardo de reaccion'; entre decisiones sigue con la velocidad anterior.
    if (m_agente->actualizar(dt)) {
        const Estimulo est = m_agente->percibir(m_posicion, m_objetivo, saludRelativa());
        // Abelardo ve desde muy lejos; cuando ve a German empieza a perseguirlo y ya no se detiene ni se retira.
        if (jefe && est.distanciaCuadrada <= m_agente->radioVision() * m_agente->radioVision()) m_persiguiendo = true;
        EstadoAgente estado = m_agente->razonar(est);
        if (jefe && m_persiguiendo) estado = EstadoAgente::Ataque;
        const QPointF dir = m_agente->actuar(estado, est);
        const double dx = m_objetivo.x() - m_posicion.x();
        m_quiereDisparar = (estado == EstadoAgente::Ataque) && (!jefe || std::fabs(dx) < kAlcanceCohete);
        const bool enPosicion = m_quiereDisparar && std::fabs(dx) < std::max(kDistanciaDisparo, m_tamano.x() * 1.4);
        if (jefe && estado == EstadoAgente::Ataque)  // el jefe corre hacia German a toda velocidad, sin frenarse por el angulo
            m_velocidad.setX(enPosicion || std::fabs(dx) < 20.0 ? 0.0 : (dx > 0.0 ? 1.0 : -1.0) * m_rapidez);
        else
            // Vista lateral: los enemigos se mueven sobre su suelo o plataforma, solo en x.
            m_velocidad.setX(enPosicion ? 0.0 : dir.x() * m_rapidez);
        if (!jefe) m_velocidad.setY(0.0);
    }
    m_posicion.setX(m_posicion.x() + m_velocidad.x() * dt);

    if (jefe) {  // Abelardo tiene gravedad y puede saltar entre el suelo y las plataformas
        m_velocidad.setY(m_velocidad.y() + kGravedadJefe * dt);
        m_posicion.setY(m_posicion.y() + m_velocidad.y() * dt);
        if (m_posicion.y() + m_tamano.y() / 2.0 >= m_suelo && m_velocidad.y() >= 0.0) {
            m_posicion.setY(m_suelo - m_tamano.y() / 2.0);
            m_velocidad.setY(0.0);
            m_enSuelo = true;
        } else {
            m_enSuelo = false;
        }
    }

    // Defiende su puesto: nunca sale del tramo A-B (asi no se sale del mapa). Abelardo persiguiendo no tiene limite.
    if (jefe && m_persiguiendo) return;
    const double xMin = std::min(m_agente->puntoA().x(), m_agente->puntoB().x());
    const double xMax = std::max(m_agente->puntoA().x(), m_agente->puntoB().x());
    if (xMax > xMin && (m_posicion.x() < xMin || m_posicion.x() > xMax)) {
        m_posicion.setX(std::max(xMin, std::min(m_posicion.x(), xMax)));
        m_velocidad.setX(0.0);
    }
}

void EnemigoRealista::saltar() {
    if (m_tipo != TipoEnemigo::EnemigoFinal || !m_enSuelo) return;
    m_velocidad.setY(-kSaltoJefe);
    m_enSuelo = false;
}
bool EnemigoRealista::persiguiendo() const { return m_persiguiendo; }
void EnemigoRealista::setPersiguiendo(bool p) { m_persiguiendo = p; }
double EnemigoRealista::suelo() const { return m_suelo; }
void EnemigoRealista::setSuelo(double sueloY) { m_suelo = sueloY; }
bool EnemigoRealista::enSuelo() const { return m_enSuelo; }
double EnemigoRealista::danoDisparo() const { return m_danoDisparo; }
void EnemigoRealista::setDanoDisparo(double dano) {
    if (dano < 0.0) throw std::invalid_argument("El dano no puede ser negativo.");
    m_danoDisparo = dano;
}

QString EnemigoRealista::rutaSprite() const {
    const bool camina = std::fabs(m_velocidad.x()) > 1.0;
    const int cuadro = camina ? static_cast<int>(m_tAnim * kFpsCaminata) % 8 + 1 : 1;  // 8 cuadros de caminata
    const QString ab = (m_variante % 2 == 0) ? QStringLiteral("a") : QStringLiteral("b");
    switch (m_tipo) {
    case TipoEnemigo::Carreta: {
        const QString base = QStringLiteral(":/sprites/nivel1/carreta_") + ab + QStringLiteral("_");
        if (estaActivo())  // con la mitad de vida o menos ya se incendia
            return base + (saludRelativa() <= 0.5 ? QStringLiteral("incendio.png") : QStringLiteral("reposo.png"));
        return base + (m_tMuerte < kTiempoColapso ? QStringLiteral("colapso.png") : QStringLiteral("escombros.png"));
    }
    case TipoEnemigo::Canon:
    case TipoEnemigo::Centinela: {
        const QString base = QStringLiteral(":/sprites/nivel1/canon_") + ab + QStringLiteral("_ang1_");
        if (!estaActivo()) return base + QStringLiteral("destruido.png");
        if (m_intervaloDisparo > 0.25 && m_tiempoDisparo > m_intervaloDisparo - 0.25)  // fogonazo al disparar
            return base + QStringLiteral("disparo.png");
        return base + (saludRelativa() <= 0.5 ? QStringLiteral("humo.png") : QStringLiteral("reposo.png"));
    }
    case TipoEnemigo::Comandante:
        return QStringLiteral(":/sprites/nivel2/soldado_caminando_%1.png").arg(cuadro, 2, 10, QLatin1Char('0'));
    case TipoEnemigo::EnemigoFinal:
        if (!estaActivo()) return QStringLiteral(":/sprites/nivel2/heroe_bazuca_16.png");  // tendido sobre la bazuca
        return QStringLiteral(":/sprites/nivel2/heroe_bazuca_%1.png").arg(cuadro, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral(":/sprites/nivel1/carreta_a_reposo.png");
}

// Los canones miran a la IZQUIERDA en su sprite (se espejan si German esta a su derecha);
// los soldados miran a la DERECHA (se espejan si German esta a su izquierda).
bool EnemigoRealista::espejar() const {
    switch (m_tipo) {
    case TipoEnemigo::Carreta: return false;
    case TipoEnemigo::Canon:
    case TipoEnemigo::Centinela: return m_objetivo.x() > m_posicion.x();
    case TipoEnemigo::Comandante:
    case TipoEnemigo::EnemigoFinal: return m_objetivo.x() < m_posicion.x();
    }
    return false;
}

bool EnemigoRealista::esVisible() const {
    if (estaActivo()) return true;
    const double d = duracionRestos(m_tipo);
    if (d <= 0.0 || m_integridad > 0.0) return false;  // sin restos, o inactivo sin haber sido destruido
    return !m_muerteIniciada || m_tMuerte < d;
}

// Los restos se van desvaneciendo (cenizas) en el ultimo tramo.
double EnemigoRealista::opacidad() const {
    if (estaActivo() || !m_muerteIniciada) return 1.0;
    const double resta = duracionRestos(m_tipo) - m_tMuerte;
    return resta >= kTiempoCeniza ? 1.0 : std::max(0.0, resta / kTiempoCeniza);
}

bool EnemigoRealista::libre() const { return !esVisible(); }
int EnemigoRealista::variante() const { return m_variante; }
void EnemigoRealista::setVariante(int variante) { m_variante = variante < 0 ? 0 : variante; }

bool EnemigoRealista::esHumanoide() const { return m_tipo == TipoEnemigo::Comandante || m_tipo == TipoEnemigo::EnemigoFinal; }

bool EnemigoRealista::puedeDisparar() const {
    if (!estaActivo() || m_tiempoDisparo > 0.0) return false;
    if (m_tipo == TipoEnemigo::Canon) return true;
    return llevaAgente(m_tipo) && m_quiereDisparar;
}

bool EnemigoRealista::disparar(const QPointF& objetivo, Armas& proyectil) {
    if (!puedeDisparar()) return false;
    m_tiempoDisparo = m_intervaloDisparo;
    m_quiereDisparar = false;
    if (m_tipo == TipoEnemigo::Canon) {  // Nivel 1 (vista cenital): tiro parabolico
        proyectil.reiniciar(m_posicion, QPointF(0, 0), m_danoDisparo, 0, kTiempoVueloCanon + 0.5, kGravedad, 0.5,
                            TipoProyectil::BalaDeCanon, ModoVista::Cenital);
        proyectil.apuntar(m_posicion, objetivo, kTiempoVueloCanon);
        return true;
    }
    // Nivel 2 (vista lateral): centinela, soldado y Abelardo disparan en linea recta.
    double velocidad = kVelBalaSoldado;
    TipoProyectil tipoBala = TipoProyectil::BalaDeCanon;
    if (m_tipo == TipoEnemigo::Centinela) velocidad = kVelBalaCentinela;
    else if (m_tipo == TipoEnemigo::EnemigoFinal) {
        velocidad = kVelCohete;
        tipoBala = TipoProyectil::Cohete;
    }
    const double distanciaAlBlanco = std::hypot(objetivo.x() - m_posicion.x(), objetivo.y() - m_posicion.y());
    const double T = std::max(0.15, distanciaAlBlanco / velocidad);
    proyectil.reiniciar(m_posicion, QPointF(0, 0), m_danoDisparo, 0, T + 1.5, kGravedad, 0.5, tipoBala, ModoVista::Lateral);
    proyectil.apuntarDirecto(m_posicion, objetivo, T);
    return true;
}

void EnemigoRealista::impactoRecibido(double dano, const QPointF& origen) {
    recibirImpacto(dano);
    if (m_agente == nullptr || !estaActivo() || !m_agente->aprender(m_posicion, origen)) return;
    const double xMin = std::min(m_agente->puntoA().x(), m_agente->puntoB().x());
    const double xMax = std::max(m_agente->puntoA().x(), m_agente->puntoB().x());
    m_posicion.setX(std::max(xMin, std::min(m_agente->coberturaSugerida(m_posicion).x(), xMax)));
}

void EnemigoRealista::reiniciar(const QPointF& pos, const QPointF& meta, double rapidez, double vida) {
    if (m_tipo != TipoEnemigo::Carreta) throw std::logic_error("Solo una carreta se puede reiniciar.");
    if (rapidez <= 0.0 || vida <= 0.0) throw std::invalid_argument("Rapidez y vida deben ser mayores que cero.");
    m_posicion = pos;
    m_meta = meta;
    m_rapidez = rapidez;
    m_velocidad = QPointF(0, 0);
    m_integridad = vida;
    m_saludMaxima = vida;
    m_activo = true;
    m_cruzo = false;
    m_tiempoDisparo = m_intervaloDisparo;
    m_tMuerte = 0.0;
    m_muerteIniciada = false;
}

double EnemigoRealista::saludRelativa() const { return m_saludMaxima > 0.0 ? m_integridad / m_saludMaxima : 0.0; }
bool EnemigoRealista::esMovil() const { return m_tipo == TipoEnemigo::Carreta; }
bool EnemigoRealista::esComandante() const { return m_tipo == TipoEnemigo::Comandante; }
bool EnemigoRealista::esJefeFinal() const { return m_tipo == TipoEnemigo::EnemigoFinal; }

TipoEnemigo EnemigoRealista::tipo() const { return m_tipo; }
void EnemigoRealista::setTipo(TipoEnemigo tipo) {
    validarAgente(tipo, m_agente);
    m_tipo = tipo;
}
AgenteInteligente* EnemigoRealista::agente() const { return m_agente; }
void EnemigoRealista::setAgente(AgenteInteligente* agente) {
    validarAgente(m_tipo, agente);
    if (agente == m_agente) return;
    delete m_agente;
    m_agente = agente;
}
QPointF EnemigoRealista::meta() const { return m_meta; }
void EnemigoRealista::setMeta(const QPointF& meta) { m_meta = meta; }
double EnemigoRealista::rapidez() const { return m_rapidez; }
void EnemigoRealista::setRapidez(double rapidez) {
    if (rapidez < 0.0) throw std::invalid_argument("La rapidez no puede ser negativa.");
    m_rapidez = rapidez;
}
bool EnemigoRealista::cruzoElPuente() const { return m_cruzo; }
void EnemigoRealista::setCruzo(bool cruzo) { m_cruzo = cruzo; }
double EnemigoRealista::intervaloDisparo() const { return m_intervaloDisparo; }
void EnemigoRealista::setIntervaloDisparo(double t) {
    if (t <= 0.0) throw std::invalid_argument("El intervalo de disparo debe ser mayor que cero.");
    m_intervaloDisparo = t;
}
double EnemigoRealista::tiempoDisparo() const { return m_tiempoDisparo; }
void EnemigoRealista::setTiempoDisparo(double t) {
    if (t < 0.0) throw std::invalid_argument("El tiempo de disparo no puede ser negativo.");
    m_tiempoDisparo = t;
}
double EnemigoRealista::saludMaxima() const { return m_saludMaxima; }
void EnemigoRealista::setSaludMaxima(double salud) {
    if (salud <= 0.0) throw std::invalid_argument("La salud maxima debe ser mayor que cero.");
    m_saludMaxima = salud;
}
QPointF EnemigoRealista::objetivo() const { return m_objetivo; }
void EnemigoRealista::setObjetivo(const QPointF& objetivo) { m_objetivo = objetivo; }
bool EnemigoRealista::quiereDisparar() const { return m_quiereDisparar; }
void EnemigoRealista::setQuiereDisparar(bool quiere) { m_quiereDisparar = quiere; }