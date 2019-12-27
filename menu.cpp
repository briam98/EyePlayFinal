#include "menu.h"
#include <QImage>

Menu::Menu()
{

}


//VARIABLES GLOBALES
static bool primeraIteracion = true;
static int velocidad_carga = 5;

static int porcentajesCarga[4];
static Rect itemsMenu[4];
static int itemCargado = -1;

static Mat m_juego1, m_juego2, m_mando, m_salir;

void Menu::inicializarImagenes() {

    QImage m_juego1_q = QImage(":/imagenes/resources/m_juego1.png");
    m_juego1 = Mat(m_juego1_q.height(), m_juego1_q.width(), CV_8UC4, m_juego1_q.scanLine(0));
    cvtColor(m_juego1, m_juego1, COLOR_RGBA2RGB);

    QImage m_juego2_q = QImage(":/imagenes/resources/m_juego2.png");
    m_juego2 = Mat(m_juego2_q.height(), m_juego2_q.width(), CV_8UC4, m_juego2_q.scanLine(0));
    cvtColor(m_juego2, m_juego2, COLOR_RGBA2RGB);

    QImage m_mando_q = QImage(":/imagenes/resources/m_mando.png");
    m_mando = Mat(m_mando_q.height(), m_mando_q.width(), CV_8UC4, m_mando_q.scanLine(0));
    cvtColor(m_mando, m_mando, COLOR_RGBA2RGB);

    QImage m_salir_q = QImage(":/imagenes/resources/m_salir.png");
    m_salir = Mat(m_salir_q.height(), m_salir_q.width(), CV_8UC4, m_salir_q.scanLine(0));
    cvtColor(m_salir, m_salir, COLOR_RGBA2RGB);

}

void Menu::dibujarMenu(Mat imagen) {

    m_juego1.copyTo(imagen.rowRange(0, 120).colRange(0, 120));
    m_juego2.copyTo(imagen.rowRange(0, 120).colRange(imagen.cols - 120, imagen.cols));
    m_mando.copyTo(imagen.rowRange(imagen.rows - 120, imagen.rows).colRange(0, 120));
    m_salir.copyTo(imagen.rowRange(imagen.rows - 120, imagen.rows).colRange(imagen.cols - 120, imagen.cols));
}

void Menu::cargarSeleccion(Circulo mando) {
    int px, py;
    for(int i = 0; i < 4; i++) {

        px = mando.centro.x;
        if (px < itemsMenu[i].x) {
           px = itemsMenu[i].x;
        }

        if (px > itemsMenu[i].x + itemsMenu[i].width) {
            px = itemsMenu[i].x + itemsMenu[i].width;
        }

        py = mando.centro.y;
        if (py < itemsMenu[i].y) {
           py = itemsMenu[i].y;
        }

        if (py > itemsMenu[i].y + itemsMenu[i].height) {
            py = itemsMenu[i].y+ itemsMenu[i].height;
        }

        double distancia = sqrt((mando.centro.x - px)*(mando.centro.x - px) + (mando.centro.y - py)*(mando.centro.y - py));

        if (distancia < mando.radio) {
            // Colision por lo que la barra de mando aumenta
            if (porcentajesCarga[i] < 200) {
                porcentajesCarga[i] += velocidad_carga;
            } else {
                itemCargado = i;
            }

        } else {
            // No hay colision por lo que la barra de carga se reinicia a 0
            porcentajesCarga[i] = 0;
        }
    }
}

void Menu::dibujarBarraCarga(Mat imagen) {
    for (int i = 0; i < 4; i++) {
        if (porcentajesCarga[i] > 0) {
            int px_loading = (120 + (((imagen.cols - 120 - 1) - 120) / 2)) - 100;
            int py_loading = (120 + (((imagen.rows - 120 - 1) - 120) / 2)) - 15;

            int p2x_loading = px_loading + 200;
            int p2y_loading = py_loading + 30;

            // Borde negro de la barra de carga
            rectangle(imagen, Point(px_loading, py_loading) , Point(p2x_loading, p2y_loading), Scalar(0, 0, 0), 1, LINE_8, 0);

            // Rectangulo verde que indica el porcentaje de carga de la barra
            p2x_loading = px_loading + porcentajesCarga[i];
            rectangle(imagen, Point(px_loading + 1, py_loading + 1) , Point(p2x_loading - 1, p2y_loading - 1), Scalar(0, 255, 0), FILLED, LINE_8, 0);

            putText(imagen, "Cargando ...", Point(px_loading + 50, p2y_loading + 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 2, FILLED);
        }
    }

}

int Menu::iterarMenu(Mat imagen, Circulo mando1, Circulo mando2, int nFrame) {
    // Si es la primara vez que se entra al menu
    // inicializamos los valores
    if (primeraIteracion) {
        primeraIteracion = false;

        inicializarImagenes();

        for (int i = 0; i < 4; i++) {
            porcentajesCarga[i] = 0;
        }

        itemsMenu[0] = Rect(0, 0, 120, 120);
        itemsMenu[1] = Rect(imagen.cols - 120 - 1, 0, 120, 120);
        itemsMenu[2] = Rect(0, imagen.rows - 120 -1, 120, 120);
        itemsMenu[3] = Rect(imagen.cols - 120 - 1, imagen.rows - 120 -1, 120, 120);
    }

    // Si no se ha seleccionado nada se muestra el menu
    if (itemCargado < 0) {
        dibujarMenu(imagen);
        cargarSeleccion(mando1);
        dibujarBarraCarga(imagen);
    } else {
        // Se muestra lo seleccionado
        if (itemCargado == 0 || itemCargado == 1) {
           int aux = util::juego(itemCargado, imagen, mando1, mando2, nFrame);

           if(aux == -10) {
               itemCargado = -1;
          }

           return aux;
        } else if (itemCargado == 2) {
            //Configurar mando uno
            itemCargado = -1;
            return -2;
        } else {
            // Salimos del juego
            return -3;
        }
    }
    return 0;
}
