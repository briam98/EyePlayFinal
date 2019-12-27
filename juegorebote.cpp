#include "juegorebote.h"
#include <unistd.h>
#include <QImage>

juegoRebote::juegoRebote()
{

}


static struct Circulo bola;

static bool primeraIteracion = true;
static bool isStartSeleccionado = false;
static bool isCargado = false;

static bool balon_centro;

static bool gol_jugador1;
static int goles_jugador1;
static bool gol_jugador2;
static int goles_jugador2;
static int nFrames;
static int tiempo;


static int porcentajeCargado;
static int velocidad_carga = 5;

static int ganador = 0;

static int FPS = 30;
static int yMsgGol;

static Mat balon;
static Mat mask_balon;
static Mat info;
static Mat celebracion;
static Mat mask_celebracion;

static Mat boton_empezar;
static Rect rect_empezar;
static Mat boton_salir;
static Rect rect_salir;

static bool comprobarColision = true;

//ESTADOS DEL JUEGO
static int estado_actual = 0;
const int MENU = 0;
const int SEL_MANDO = 1;
const int JUGANDO = 2;
const int CELEBRACION = 3;
const int FIN = 4;

void juegoRebote::mostrarInfoJuego(Mat imagen) {
    info.copyTo(imagen.rowRange(0, 210).colRange(120, 120 + 400));
    boton_empezar.copyTo(imagen.rowRange(rect_empezar.y, rect_empezar.y + rect_empezar.height).colRange(rect_empezar.x, rect_empezar.x + rect_empezar.width));
}

void juegoRebote::cargarSeleccion(Circulo mando, Rect boton) {

    float px, py;

    px = mando.centro.x;
    if (px < boton.x) {
        px = boton.x;
    }

    if (px > boton.x + boton.width) {
        px = boton.x + boton.width;
    }

    py = mando.centro.y;
    if (py < boton.y) {
        py = boton.y;
    }

    if (py > boton.y + boton.height) {
        py = boton.y + rect_empezar.height;
    }

    float distancia = sqrt((mando.centro.x - px)*(mando.centro.x - px) + (mando.centro.y - py)*(mando.centro.y - py));

    if (distancia < mando.radio) {
        // Cargamos la barra de carga si no esta completa
        // en el caso de que este llena lo indicamos con la
        // variable isCargado
        if (porcentajeCargado < 200) {
            porcentajeCargado += velocidad_carga;
        } else {
            isCargado = true;
        }

    } else {
        porcentajeCargado = 0;
    }
}

void juegoRebote::dibujarBarraCarga(Mat imagen) {
    if (porcentajeCargado > 0) {
        int px_loading = rect_empezar.x + 20;
        int py_loading = rect_empezar.y - 70;

        int p2x_loading = px_loading + 200;
        int p2y_loading = py_loading + 30;

        // Borde negro de la barra de carga
        rectangle(imagen, Point(px_loading, py_loading) , Point(p2x_loading, p2y_loading), Scalar(0, 0, 0), 1, LINE_8, 0);

        // Rectangulo verde que indica el porcentaje de carga de la barra
        p2x_loading = px_loading + porcentajeCargado;
        rectangle(imagen, Point(px_loading + 1, py_loading + 1) , Point(p2x_loading - 1, p2y_loading - 1), Scalar(0, 255, 0), FILLED, LINE_8, 0);

        putText(imagen, "Cargando ...", Point(px_loading + 50, p2y_loading + 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 2, FILLED);
    }
}

void juegoRebote::calcularColisionesBordes(Mat imagen) {
    int ancho_imagen = imagen.cols - 1;
    int altura_imagen = imagen.rows - 1;

    if (bola.centro.x + bola.radio >= ancho_imagen) {
        estado_actual = CELEBRACION;
        gol_jugador1 = true;
        goles_jugador1++;
    }

    if (bola.centro.x - bola.radio <= 0) {
        estado_actual = CELEBRACION;
        gol_jugador2 = true;
        goles_jugador2++;
    }

    if (bola.centro.y + bola.radio >= altura_imagen) {
        bola.centro.y = altura_imagen - bola.radio;
        bola.velocidad.y *= -1;
    }

    if (bola.centro.y - bola.radio <= 0) {
        bola.centro.y = bola.radio;
        bola.velocidad.y *= -1;
    }
}

void juegoRebote::desplazarBola(Circulo mando , float distancia) {

    float distancia_overlap = ((mando.radio +  bola.radio) - distancia);

    float desplazamiento_x = ((bola.centro.x - mando.centro.x) / distancia) * distancia_overlap;
    float desplazamiento_y = ((bola.centro.y - mando.centro.y) / distancia) * distancia_overlap;

    bola.centro.x += desplazamiento_x;
    bola.centro.y += desplazamiento_y;
}

void juegoRebote::calcularNuevaVelocidad(Circulo mando) {

    Point_<float> v_union(mando.centro.x - bola.centro.x, mando.centro.y - bola.centro.y);
    float modulo_union = static_cast<float>(sqrt((v_union.x * v_union.x) + (v_union.y * v_union.y)));
    Point_<float> v_union_unitario(v_union.x / modulo_union, v_union.y / modulo_union);
    float escalar1 = bola.velocidad.x * v_union_unitario.x + bola.velocidad.y * v_union_unitario.y;

    Point_<float> v_x(escalar1 * v_union_unitario.x, escalar1 * v_union_unitario.y);

    Point_<float> v_unitario_perpendicular(-v_union_unitario.y, v_union_unitario.x);
    float escalar2 = bola.velocidad.x * v_unitario_perpendicular.x + bola.velocidad.y * v_unitario_perpendicular.y;

    Point_<float> v_y(escalar2 * v_unitario_perpendicular.x, escalar2 * v_unitario_perpendicular.y);

    Point_<float> _v_x(-v_x.x, -v_x.y);

    Point_<float> nueva_velocidad(_v_x.x + v_y.x, _v_x.y + v_y.y);

    bola.velocidad.x = ceil(nueva_velocidad.x);
    bola.velocidad.y = ceil(nueva_velocidad.y);
}

void juegoRebote::calcularColisionesMando(Circulo mando) {
    float distancia = static_cast<float>(sqrt((mando.centro.x - bola.centro.x)*(mando.centro.x - bola.centro.x) +
                           (mando.centro.y - bola.centro.y)*(mando.centro.y - bola.centro.y)));
    int sumaRadios = bola.radio + mando.radio;

    if (comprobarColision) {
        if (distancia <= sumaRadios) {
            //Existe colision entre el mando y la bola
            desplazarBola(mando, distancia);
            calcularNuevaVelocidad(mando);
        }
    }

}

void juegoRebote::dibujarBalon(Mat imagen) {

    if (balon_centro) {
        balon_centro = false;
        bola.centro.x = imagen.cols / 2;
        bola.centro.y = imagen.rows / 2;

    } else {
        if (bola.centro.x < bola.radio) {
            bola.centro.x = bola.radio ;
        }

        if (bola.centro.x > imagen.cols - bola.radio) {
            bola.centro.x = imagen.cols - bola.radio;
        }

        if (bola.centro.y < bola.radio) {
            bola.centro.y = bola.radio ;
        }

        if (bola.centro.y > imagen.rows - bola.radio) {
            bola.centro.y = imagen.rows - bola.radio;
        }
    }

    Rect selection(bola.centro.x - 25, bola.centro.y - 25, 50, 50);
    Mat roi_destino(imagen, selection);

    balon.copyTo(roi_destino, mask_balon);

    line(imagen, Point(imagen.cols / 2 - 1, 0), Point(imagen.cols / 2 - 1, imagen.rows - 1), Scalar(255, 255, 255),1, LINE_8, 0);
}

void juegoRebote::moverBola() {

    if (bola.velocidad.x == 0.f && bola.velocidad.y == 0.f) {
        bola.velocidad.x = 5;
        bola.velocidad.y = 5;
    }

    bola.centro.x += bola.velocidad.x * 2;
    bola.centro.y += bola.velocidad.y * 2;
}

void juegoRebote::mostrarMensajeGol(Mat imagen) {

    if (yMsgGol + 240 > imagen.rows - 1) {
        yMsgGol += 3;
        return;
    }

    Rect selection(120, yMsgGol, 400, 240);
    Mat roi_destino(imagen, selection);

    celebracion.copyTo(roi_destino, mask_celebracion);

    yMsgGol += 3;
}

void juegoRebote::mostrarTiempo(Mat imagen) {
    char mensajeInicio[100];
    sprintf(mensajeInicio, "El balon aparece en %d", tiempo);

    putText(imagen, mensajeInicio, Point(imagen.cols / 2 - 160, imagen.rows / 2 - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, FILLED);
}

void juegoRebote::dibujarMarcador(Mat imagen) {
    char mensajeInicio[100];
    sprintf(mensajeInicio, "J1: %d - J2: %d", goles_jugador1, goles_jugador2);

    putText(imagen, mensajeInicio, Point(imagen.cols / 2 - 57, 30), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2, FILLED);
}

void juegoRebote::inicializar(Mat imagen) {

    //Inicializamos la bola
    bola.centro = Point(imagen.cols / 2 - 1, imagen.rows / 2 - 1);
    bola.velocidad.x = -5;
    bola.velocidad.y = 0;
    bola.radio = 25;

    rect_empezar.x = imagen.cols/2 - 125 -1;
    rect_empezar.y = imagen.rows - 75;
    rect_empezar.width = 240;
    rect_empezar.height = 75;

    rect_salir.x = imagen.cols - 240;
    rect_salir.y = imagen.rows - 75;
    rect_salir.width = 240;
    rect_salir.height = 75;

    yMsgGol = 0;

    isStartSeleccionado = false;
    balon_centro = true;

    gol_jugador1 = false;
    goles_jugador1 = 0;

    gol_jugador2 = false;
    goles_jugador2 = 0;

    ganador = 0;

    nFrames = 0;
    tiempo = 3;
}

void juegoRebote::mostarCelebracion(Mat imagen) {
    mostrarMensajeGol(imagen);

    // Su la imagen del gol llega al borde mostramos el contador del tiempo
    // si el juego todavia no ha finalizado
    if (yMsgGol + 240 > imagen.rows - 1) {

        if (goles_jugador1 == 5) {
            estado_actual = FIN;
            ganador = 1;
        } else if (goles_jugador2 == 5) {
            estado_actual = FIN;
            ganador = 2;
        } else {

            if (yMsgGol + 240 > imagen.rows - 1 + 40) {
                mostrarTiempo(imagen);

                if (nFrames % FPS == 0) {
                    tiempo--;
                }
            }
        }
    }

    nFrames++;

    // Si el tiempo ha terminado reiniciamos valores
    if (tiempo < 0) {
        if (gol_jugador1) {
            bola.velocidad.x = 5;
            bola.velocidad.y = 0;
        } else {
            bola.velocidad.x = -5;
            bola.velocidad.y = 0;
        }

        gol_jugador2 = false;
        gol_jugador1 = false;
        balon_centro = true;
        nFrames = 0;
        tiempo = 3;
        yMsgGol = 0;
        estado_actual = JUGANDO;
    }
}

void juegoRebote::inicializarImagenes() {

    QImage info_q = QImage(":/imagenes/resources/info_juego2.png");
    info = Mat(info_q.height(), info_q.width(), CV_8UC4, info_q.scanLine(0));
    cvtColor(info, info, COLOR_RGBA2RGB);

    QImage balon_q = QImage(":/imagenes/resources/balon.png");
    balon = Mat(balon_q.height(), balon_q.width(), CV_8UC4, balon_q.scanLine(0));
    cvtColor(balon, balon, COLOR_RGBA2RGB);

    QImage mask_balon_q = QImage(":/imagenes/resources/mask_balon.png");
    mask_balon = Mat(mask_balon_q.height(), mask_balon_q.width(), CV_8UC4, mask_balon_q.scanLine(0));
    cvtColor(mask_balon, mask_balon, COLOR_RGBA2RGB);

    QImage celebracion_q = QImage(":/imagenes/resources/celebracion.png");
    celebracion = Mat(celebracion_q.height(), celebracion_q.width(), CV_8UC4, celebracion_q.scanLine(0));
    cvtColor(celebracion, celebracion, COLOR_RGBA2RGB);

    QImage mask_celebracion_q = QImage(":/imagenes/resources/mask_celebracion.png");
    mask_celebracion = Mat(mask_celebracion_q.height(), mask_celebracion_q.width(), CV_8UC4, mask_celebracion_q.scanLine(0));
    cvtColor(mask_celebracion, mask_celebracion, COLOR_RGBA2RGB);

    QImage boton_empezar_q = QImage(":/imagenes/resources/boton_empezar.png");
    boton_empezar = Mat(boton_empezar_q.height(), boton_empezar_q.width(), CV_8UC4, boton_empezar_q.scanLine(0));
    cvtColor(boton_empezar, boton_empezar, COLOR_RGBA2RGB);

    QImage boton_salor_q = QImage(":/imagenes/resources/boton_volver.png");
    boton_salir = Mat(boton_salor_q.height(), boton_salor_q.width(), CV_8UC4, boton_salor_q.scanLine(0));
    cvtColor(boton_salir, boton_salir, COLOR_RGBA2RGB);
}

void juegoRebote::mostrarBotonSalir(Mat imagen) {
    boton_salir.copyTo(imagen.colRange(imagen.cols - 240, imagen.cols).rowRange(imagen.rows - 75, imagen.rows));
}

void juegoRebote::dibujarBordes(Mat imagen) {
    Mat overlay;
    imagen.copyTo(overlay);

    rectangle(overlay, Point(0, 0) , Point(20, imagen.rows - 1), Scalar(0, 0, 255), FILLED, LINE_8);
    rectangle(overlay, Point(imagen.cols - 21, 0) , Point(imagen.cols - 1, imagen.rows - 1), Scalar(255, 0, 0), FILLED, LINE_8);

    double alpha = 0.2;

    // Añadimos el canal alfa para que los bordes se vean transparentes
    addWeighted(overlay, alpha, imagen, 1 - alpha, 0, imagen);
}

int juegoRebote::iterar(Mat imagen, Circulo mando1, Circulo mando2) {

    if (primeraIteracion) {
        primeraIteracion = false;
        inicializar(imagen);
        inicializarImagenes();
    }

    // Mostramos el menu del juego con el boton de empezar
    // y la descripcion
    if (estado_actual == MENU) {
        mostrarInfoJuego(imagen);
        cargarSeleccion(mando1, rect_empezar);

        if (isCargado) {
            estado_actual = SEL_MANDO;
            isCargado = false;
        }
        dibujarBarraCarga(imagen);
    } else if (estado_actual == SEL_MANDO) {
        // Comprobamos si se ha seleccionado el mando 2
        if (!mando2.isSelecionado) {
            return -1;
        }

        estado_actual = JUGANDO;
    }

    if (estado_actual == JUGANDO) {
        // Dibujamos los elementos del juego
        // y realizamos los calculos oportunos
        dibujarBordes(imagen);
        dibujarBalon(imagen);
        dibujarMarcador(imagen);
        moverBola();
        calcularColisionesBordes(imagen);

        // Comprobamos si cada uno de los mandos se encuentra en su mitad del campo
        // en cuyo caso comprobamos si hay colision con el balón
        if (mando1.centro.x < imagen.cols / 2) {
            calcularColisionesMando(mando1);
        }

        if (mando2.centro.x > imagen.cols / 2) {
            calcularColisionesMando(mando2);
        }
    } else if (estado_actual == CELEBRACION) {
        // Tras un gol mostramos los mensajes correspondientes
        mostarCelebracion(imagen);
    }

    if (estado_actual == FIN) {
        // Mostramos el mensaje indicando el jugador que ha ganado y
        // el botón para volver al menu inicial de la aplicacion
        char msg[100];
        sprintf(msg, "El jugador %d ha ganado", ganador);
        putText(imagen, msg, Point(imagen.cols / 2 - 170, imagen.rows / 2 - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2, FILLED);

        mostrarBotonSalir(imagen);
        cargarSeleccion(mando1, rect_salir);

        // Si se ha cargado volver al menu reiniciamos los valores del juego
        if (isCargado) {
            inicializar(imagen);
            isCargado = false;
            estado_actual = MENU;
            return -10;
        }

        dibujarBarraCarga(imagen);
        return 0;
    }

    return 0;
}
