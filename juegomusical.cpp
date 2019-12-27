#include "juegomusical.h"
#include <QImage>
JuegoMusical::JuegoMusical()
{

}

const int NUM_RECEPTORES = 5;
const int NEGRO = 0;
const int ROJO = 1;
const int VERDE = 2;
const int AZUL = 3;
const int ORO = 4;
const int NO_INICIADO = -1;
const int INICIADO = 0;
const int TERMINADO = 1;

static Point dimensionesReceptor(70, 60);
static int radioReceptor = dimensionesReceptor.x / 2;
static Scalar colorReceptor(0,0,0);
static Rect arrayReceptores[NUM_RECEPTORES];
static Point centroReceptores[NUM_RECEPTORES];
static Point_<float> vUnitarioReceptores[NUM_RECEPTORES];

static int velocidadJuego = 5;   // Aumentar para que aumente la velocidad de los emisores
static int tiempoAparicion = 25; // Disminuir para que aumente la velocidad de aparicion
static int TIEMPO_JUEGO = 30;

static list<Emisor> listasEmisores[NUM_RECEPTORES];

static int radioEmisor = 25;
static int tamEmisor = radioEmisor*2;
static Scalar colorEmisor(0,0,255);
static Point centroInicioEmisor;

static Mat tambor;
static Mat mask_tambor;
static Mat notas[4][5];
static Mat mask_notas[4][5];
static string sonidoNotas[4];

static Mat bigImage;
static Mat bigImageRoi;

static bool primeraIteracion = true;
static bool primeraIteracionJuego = true;
static int estadoJuego = NO_INICIADO;

static list<puntuacion> listaPuntuacion;
static int puntosGlobales = 0;
static double tamLetraInicial = 1;

static bool isCargado = false;
static bool isStartSeleccionado = false;
static int porcentajeCargado;
static int velocidad_carga = 5;

static Mat info;
static Mat boton_empezar;
static Rect rect_empezar;
static Mat boton_volver;
static Rect rect_salir;

void JuegoMusical::mostrarInfoJuego(Mat image) {
    Mat destinationRoi(image, Rect(120, 50, 470, 210));
    info.copyTo(destinationRoi);

    boton_empezar.copyTo(Mat(image, rect_empezar));
}

int JuegoMusical::cargarSeleccion(Circulo mando, Rect boton) {

    Rect boton2 = Rect(boton.x - 50, boton.y - 50, boton.width, boton.height);
    float px, py;

    px = mando.centro.x;
    if (px < boton2.x) {
        px = boton2.x;
    }

    if (px > boton2.x + boton2.width) {
        px = boton2.x + boton2.width;
    }

    py = mando.centro.y;
    if (py < boton2.y) {
        py = boton2.y;
    }

    if (py > boton2.y + boton2.height) {
        py = boton2.y + rect_empezar.height;
    }

    float distancia = sqrt((mando.centro.x - px)*(mando.centro.x - px) + (mando.centro.y - py)*(mando.centro.y - py));

    if (distancia < mando.radio) {

        if (porcentajeCargado < 200) {
            porcentajeCargado += velocidad_carga;
        } else {
            isCargado = true;
            return -1;
        }

    } else {
        porcentajeCargado = 0;
    }
    return 0;
}

void JuegoMusical::dibujarBarraCarga(Mat imagen) {
    if (porcentajeCargado > 0) {
        int px_loading = rect_empezar.x + 20;
        int py_loading = rect_empezar.y - 140;

        int p2x_loading = px_loading + 200;
        int p2y_loading = py_loading + 30;

        rectangle(imagen, Point(px_loading, py_loading) , Point(p2x_loading, p2y_loading), Scalar(0, 0, 0), 1, LINE_8, 0);

        p2x_loading = px_loading + porcentajeCargado;
        rectangle(imagen, Point(px_loading + 1, py_loading + 1) , Point(p2x_loading - 1, p2y_loading - 1), Scalar(0, 255, 0), FILLED, LINE_8, 0);

        putText(imagen, "Cargando ...", Point(px_loading + 50, p2y_loading + 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 2, FILLED);
    }
}

void JuegoMusical::addPuntos(Emisor* emisor, int puntos) {

    Scalar color;
    if (puntos < 50) {
        color = Scalar(0,0,255);
        emisor->setImagen(notas[emisor->getNum_Nota()][ROJO]);
        emisor->setImagenMask(mask_notas[emisor->getNum_Nota()][ROJO]);
    } else if (puntos >= 50 && puntos < 75) {
        color = Scalar(255,0,0);
        emisor->setImagen(notas[emisor->getNum_Nota()][AZUL]);
        emisor->setImagenMask(mask_notas[emisor->getNum_Nota()][AZUL]);
    } else if (puntos >= 75 && puntos < 100) {
        color = Scalar(0,255,0);
        emisor->setImagen(notas[emisor->getNum_Nota()][VERDE]);
        emisor->setImagenMask(mask_notas[emisor->getNum_Nota()][VERDE]);
    } else {
        color = Scalar(55,175,212);
        emisor->setImagen(notas[emisor->getNum_Nota()][ORO]);
        emisor->setImagenMask(mask_notas[emisor->getNum_Nota()][ORO]);
    }
    Point centroReceptor = centroReceptores[emisor->getDestino()];
    puntuacion aux;
    if (emisor->getDestino() == 2) {
        aux = puntuacion(Point(centroReceptor.x - radioReceptor - 20, centroReceptor.y + radioReceptor + 20),
                         puntos, tamLetraInicial, color);
    } else {
        aux = puntuacion(Point(centroReceptor.x - radioReceptor - 20, centroReceptor.y - radioReceptor - 20),
                         puntos, tamLetraInicial, color);
    }

    listaPuntuacion.push_back(aux);
    puntosGlobales += puntos;
}

void JuegoMusical::mostrarPuntos() {
    list<puntuacion>::iterator it = listaPuntuacion.begin();

    while (it != listaPuntuacion.end()) {
        String stringPuntos = to_string((*it).puntos);
        putText(bigImage, stringPuntos, (*it).posicion,
                FONT_HERSHEY_SCRIPT_SIMPLEX, (*it).tamLetra, (*it).color, 2, LINE_AA);
        (*it).disminuirTam();
        if ((*it).tamLetra < 0.5) {
            it = listaPuntuacion.erase(it);  // alternatively, i = items.erase(i);
        }
        else {
            ++it;
        }
    }
}

void JuegoMusical::colisiones(Circulo mando) {
    Point centroMando = Point(static_cast<int>(mando.centro.x) + 50, static_cast<int>(mando.centro.y + 50));
    circle(bigImage, centroMando, 2, Scalar(0,0,0), 2, LINE_AA, 0);
    for (int i = 0; i < NUM_RECEPTORES; i++) {
        int distancia = static_cast<int>(sqrt(pow(centroReceptores[i].x - centroMando.x, 2) +
                        pow(centroReceptores[i].y - centroMando.y, 2)));

        if (distancia <= radioReceptor + mando.radio) {
            //Si esta colisionando comprobar si ese receptor esta colisionando con algun emisor
            for (list<Emisor>::iterator it=listasEmisores[i].begin(); it != listasEmisores[i].end(); ++it) {
                if (!(*it).isColisionado()) {
                    Point centroEmisor = Point(static_cast<int>((*it).getCentro().x), static_cast<int>((*it).getCentro().y));
                    int distancia2 = static_cast<int>(sqrt(pow(centroReceptores[i].x - centroEmisor.x, 2) +
                                    pow(centroReceptores[i].y - centroEmisor.y, 2)));
                    if (distancia2 <= (*it).getRadio() + radioReceptor) {
                        addPuntos(&(*it), 100-distancia2);
                        (*it).setColisionado(true);
                        return;
                    }
                }
            }
        }
    }
}

void JuegoMusical::moverEmisores() {
    for (int i = 0; i < NUM_RECEPTORES; i++) {
        bool borrarPrimero = false;
        for (list<Emisor>::iterator it=listasEmisores[i].begin(); it != listasEmisores[i].end(); it++) {

            Point_<float> centroEmisor = (*it).getCentro();

            float nuevaX = centroEmisor.x - vUnitarioReceptores[i].x * velocidadJuego;
            float nuevaY = centroEmisor.y - vUnitarioReceptores[i].y * velocidadJuego;

            (*it).setCentro(Point_<float>(nuevaX, nuevaY));

            if (centroEmisor.y <= 0 || centroEmisor.x <= 0) {
                borrarPrimero = true;
            }
        }
        if (borrarPrimero) {
            Emisor aux = listasEmisores[i].front();
            listasEmisores[i].pop_front();
            aux.deleteEmisor();
        }
    }
}

void JuegoMusical::inicializarValores(Mat image) {
    srand(static_cast<unsigned int>(time(nullptr)));
    //Inicializar listas de emisores
    for (int i = 0; i < NUM_RECEPTORES; i++) {
        list<Emisor> aux;
        listasEmisores[i] = aux;
    }

    primeraIteracionJuego = true;

    rect_empezar.x = 250;
    rect_empezar.y = 480+50 - 75;
    rect_empezar.width = 240;
    rect_empezar.height = 75;

    rect_salir = rect_empezar;

    bigImage = Mat(image.rows+tamEmisor,image.cols+2*tamEmisor, CV_8UC3);
    bigImageRoi = Mat(bigImage, Rect(tamEmisor,tamEmisor,image.cols,image.rows));

    centroInicioEmisor = Point(bigImage.cols / 2, 500);

    inicializarImagenes();

    cvtColor(tambor, mask_tambor, COLOR_BGR2GRAY);

    threshold(mask_tambor, mask_tambor, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);

    centroReceptores[0] = Point(110, 260);
    centroReceptores[1] = Point(180, 140);
    centroReceptores[2] = Point(370, 100);
    centroReceptores[3] = Point(560, 140);
    centroReceptores[4] = Point(625, 260);

    for (int i = 0; i < NUM_RECEPTORES; i++) {
        Point puntoRectangulo(centroReceptores[i].x - 30, centroReceptores[i].y - 30);
        arrayReceptores[i] = Rect(puntoRectangulo.x, puntoRectangulo.y, dimensionesReceptor.x, dimensionesReceptor.y);
        Point diferenciaCentros(centroInicioEmisor.x - centroReceptores[i].x, centroInicioEmisor.y - centroReceptores[i].y);
        double modulo = sqrt(diferenciaCentros.x*diferenciaCentros.x + diferenciaCentros.y*diferenciaCentros.y);
        float unitarioX = diferenciaCentros.x / static_cast<float>(modulo);
        float unitarioY = diferenciaCentros.y / static_cast<float>(modulo);
        vUnitarioReceptores[i] = Point_<float>(unitarioX, unitarioY);
    }

}

static int tiempoInicial;
int JuegoMusical::iterar(Mat image, Circulo mando, int frame) {
    if (primeraIteracion) {
        inicializarValores(image);
        primeraIteracion = false;
    }

    //Copiamos el frame actual a uno más grande
    image.copyTo(bigImageRoi);

    if (estadoJuego == NO_INICIADO) {
        mostrarInfoJuego(bigImage);
        if (cargarSeleccion(mando, rect_empezar) == -1) {
            estadoJuego = INICIADO;
        }
        if (isCargado) {
            isStartSeleccionado = true;
            isCargado = false;
        }

        dibujarBarraCarga(bigImage);
    } else if (estadoJuego == INICIADO) {
        if (primeraIteracionJuego) {
            tiempoInicial = clock();
            primeraIteracionJuego = false;
        }
        if (frame % tiempoAparicion == 0) {
            int destino = rand() % 5;
            int num_imagen = rand() % 4;
            Emisor nuevoEmisor(centroInicioEmisor,
                               notas[num_imagen][NEGRO], mask_notas[num_imagen][NEGRO],
                               num_imagen, radioEmisor, destino, false);
            listasEmisores[destino].push_back(nuevoEmisor);
        }

        int tiempoActual = clock();
        int tiempoPasado = (tiempoActual - tiempoInicial) / CLOCKS_PER_SEC;
        Scalar colorTiempo = Scalar(255,255,255);
        if (tiempoPasado >= TIEMPO_JUEGO - 5) {
            colorTiempo = Scalar(0,0,255);
            if (tiempoPasado == TIEMPO_JUEGO) {
                estadoJuego = TERMINADO;
            }
        }

        String stringPuntos = "Puntuacion: " + to_string(puntosGlobales);
        putText(bigImage, stringPuntos, Point(450,510),
                FONT_HERSHEY_DUPLEX, 0.8, Scalar(255,255,255), 1, LINE_AA);

        String stringTiempo = "Tiempo: " + to_string(TIEMPO_JUEGO - tiempoPasado);
        putText(bigImage, stringTiempo, Point(60, 510),
                FONT_HERSHEY_DUPLEX, 0.8, colorTiempo, 1, LINE_AA);

        //Pintamos los emisores en la imagen mas grande
        for (int i = 0; i < NUM_RECEPTORES; i++) {
            Mat destinationRoi(bigImage, arrayReceptores[i]);
            tambor.copyTo(destinationRoi, mask_tambor);
            circle(bigImage, centroReceptores[i], 2, colorReceptor, 1, LINE_AA, 0);

            for (list<Emisor>::iterator it=listasEmisores[i].begin(); it != listasEmisores[i].end(); ++it) {
                int px = static_cast<int>((*it).getIzqSuperior().x);
                int py = static_cast<int>((*it).getIzqSuperior().y);
                Rect rectDibujo;

                if (px > 0 && py > 0 && px < bigImage.cols-tamEmisor) {
                    rectDibujo = Rect(px, py, (*it).getRadio()*2, (*it).getRadio()*2);

                    Mat destinationRoiEmisor(bigImage, rectDibujo);
                    (*it).getImagen().copyTo(destinationRoiEmisor, (*it).getImagenMask());
                }
            }
        }

        colisiones(mando);
        moverEmisores();
        mostrarPuntos();
    } else if (estadoJuego == TERMINADO) {
        String stringFinal = "Puntuacion final: " + to_string(puntosGlobales);
        putText(bigImage, stringFinal, Point(bigImage.rows/2-95, 100),
                FONT_HERSHEY_SIMPLEX, 1.2, Scalar(255,255,255), 2, LINE_AA);

        boton_volver.copyTo(Mat(bigImage, rect_salir));
        if (cargarSeleccion(mando, rect_salir) == -1) {
            estadoJuego = NO_INICIADO;
            inicializarValores(image);
            return -10;
        }
        dibujarBarraCarga(bigImage);
    }

    //Copiamos la parte que nos interesa de la bigImagen a imagen
    Mat imageRoi(image, Rect(0,0,image.cols,image.rows));
    bigImageRoi.copyTo(imageRoi);

    return 0;
}

void JuegoMusical::inicializarImagenes() {

    QImage aux = QImage(":/imagenes/resources/info_juego1.png");
    info = Mat(aux.height(), aux.width(), CV_8UC4, aux.scanLine(0));
    cvtColor(info, info, COLOR_RGBA2RGB);

    QImage aux1 = QImage(":/imagenes/resources/boton_empezar.png");
    boton_empezar = Mat(aux1.height(), aux1.width(), CV_8UC4, aux1.scanLine(0));
    cvtColor(boton_empezar, boton_empezar, COLOR_RGBA2RGB);

    QImage aux2 = QImage(":/imagenes/resources/boton_volver.png");
    boton_volver = Mat(aux2.height(), aux2.width(), CV_8UC4, aux2.scanLine(0));
    cvtColor(boton_volver, boton_volver, COLOR_RGBA2RGB);

    QImage aux3 = QImage(":/imagenes/resources/tambor1.png");
    tambor = Mat(aux3.height(), aux3.width(), CV_8UC4, aux3.scanLine(0));
    cvtColor(tambor, tambor, COLOR_RGBA2RGB);

    QImage aux00 = QImage(":/imagenes/resources/nota0.png");
    notas[0][0] = Mat(aux00.height(), aux00.width(), CV_8UC4, aux00.scanLine(0));
    QImage aux01 = QImage(":/imagenes/resources/nota0_rojo.png");
    notas[0][1] = Mat(aux01.height(), aux01.width(), CV_8UC4, aux01.scanLine(0));
    QImage aux02 = QImage(":/imagenes/resources/nota0_verde.png");
    notas[0][2] = Mat(aux02.height(), aux02.width(), CV_8UC4, aux02.scanLine(0));
    QImage aux03 = QImage(":/imagenes/resources/nota0_azul.png");
    notas[0][3] = Mat(aux03.height(), aux03.width(), CV_8UC4, aux03.scanLine(0));
    QImage aux04 = QImage(":/imagenes/resources/nota0_oro.png");
    notas[0][4] = Mat(aux04.height(), aux04.width(), CV_8UC4, aux04.scanLine(0));

    QImage aux10 = QImage(":/imagenes/resources/nota1.png");
    notas[1][0] = Mat(aux10.height(), aux10.width(), CV_8UC4, aux10.scanLine(0));
    QImage aux11 = QImage(":/imagenes/resources/nota1_rojo.png");
    notas[1][1] = Mat(aux11.height(), aux11.width(), CV_8UC4, aux11.scanLine(0));
    QImage aux12 = QImage(":/imagenes/resources/nota1_verde.png");
    notas[1][2] = Mat(aux12.height(), aux12.width(), CV_8UC4, aux12.scanLine(0));
    QImage aux13 = QImage(":/imagenes/resources/nota1_azul.png");
    notas[1][3] = Mat(aux13.height(), aux13.width(), CV_8UC4, aux13.scanLine(0));
    QImage aux14 = QImage(":/imagenes/resources/nota1_oro.png");
    notas[1][4] = Mat(aux14.height(), aux14.width(), CV_8UC4, aux14.scanLine(0));

    QImage aux20 = QImage(":/imagenes/resources/nota2.png");
    notas[2][0] = Mat(aux20.height(), aux20.width(), CV_8UC4, aux20.scanLine(0));
    QImage aux21 = QImage(":/imagenes/resources/nota2_rojo.png");
    notas[2][1] = Mat(aux21.height(), aux21.width(), CV_8UC4, aux21.scanLine(0));
    QImage aux22 = QImage(":/imagenes/resources/nota2_verde.png");
    notas[2][2] = Mat(aux22.height(), aux22.width(), CV_8UC4, aux22.scanLine(0));
    QImage aux23 = QImage(":/imagenes/resources/nota2_azul.png");
    notas[2][3] = Mat(aux23.height(), aux23.width(), CV_8UC4, aux23.scanLine(0));
    QImage aux24 = QImage(":/imagenes/resources/nota2_oro.png");
    notas[2][4] = Mat(aux24.height(), aux24.width(), CV_8UC4, aux24.scanLine(0));

    QImage aux30 = QImage(":/imagenes/resources/nota3.png");
    notas[3][0] = Mat(aux30.height(), aux30.width(), CV_8UC4, aux30.scanLine(0));
    QImage aux31 = QImage(":/imagenes/resources/nota3_rojo.png");
    notas[3][1] = Mat(aux31.height(), aux31.width(), CV_8UC4, aux31.scanLine(0));
    QImage aux32 = QImage(":/imagenes/resources/nota3_verde.png");
    notas[3][2] = Mat(aux32.height(), aux32.width(), CV_8UC4, aux32.scanLine(0));
    QImage aux33 = QImage(":/imagenes/resources/nota3_azul.png");
    notas[3][3] = Mat(aux33.height(), aux33.width(), CV_8UC4, aux33.scanLine(0));
    QImage aux34 = QImage(":/imagenes/resources/nota3_oro.png");
    notas[3][4] = Mat(aux34.height(), aux34.width(), CV_8UC4, aux34.scanLine(0));

    for (int i = 0; i < 4; i++) {
        for (int j = 0 ; j < 5; j++) {
            cvtColor(notas[i][j], notas[i][j], COLOR_RGBA2RGB);
            cvtColor(notas[i][j], mask_notas[i][j], COLOR_BGR2GRAY);
            threshold(mask_notas[i][j], mask_notas[i][j], 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
        }
    }
}
