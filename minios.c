#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h> 

#define MAX_ARCHIVOS 10
#define TAM_NOMBRE 30
#define TAM_CONTENIDO 100

typedef struct {
    int id;
    char nombre[TAM_NOMBRE];
    char contenido[TAM_CONTENIDO];
    int activo; 
} ArchivoVirtual;

ArchivoVirtual discoVirtual[MAX_ARCHIVOS];

void inicializarDisco();
void crearArchivo();
void leerArchivos();
void actualizarArchivo();
void eliminarArchivo();
void retoBsdGamesSnake();
void retoBsdGamesTetris();

int main() {
    int opcion;
    inicializarDisco();

    while (1) {
        printf("\n=========================================\n");
        printf("         MINIOS - KERNEL CONSOLE         \n");
        printf("=========================================\n");
        printf("1. [CREATE] Crear archivo en memoria sec.\n");
        printf("2. [READ]   Ver archivos guardados\n");
        printf("3. [UPDATE] Modificar archivo\n");
        printf("4. [DELETE] Eliminar archivo (Lógico)\n");
        printf("5. [RETO]   Reto BSDgames (Snake)\n");
        printf("6. [RETO]   Reto BSDgames (Tetris)\n");
        printf("7. [SHUTDOWN] Apagar MiniOS\n");
        printf("=========================================\n");
        printf("Seleccione una opción: ");
        
        if (scanf("%d", &opcion) != 1) {
            printf("[ERROR] Entrada inválida. Ingrese un número.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        switch (opcion) {
            case 1:
                crearArchivo();
                break;
            case 2:
                leerArchivos();
                break;
            case 3:
                actualizarArchivo();
                break;
            case 4:
                eliminarArchivo();
                break;
            case 5:
                printf("\n[BOOT] Cargando subproceso 'BSDgames: Snake'...\n");
                sleep(1);
                retoBsdGamesSnake();
                break;
            case 6:
                printf("\n[BOOT] Cargando subproceso 'BSDgames: Tetris'...\n");
                sleep(1);
                retoBsdGamesTetris();
                break;
            case 7:
                printf("\n[SHUTDOWN] Sincronizando sectores... Apagando MiniOS de forma segura.\n");
                exit(0);
            default:
                printf("[ALERTA] Código de operación desconocido.\n");
        }
    }
    return 0;
}

void inicializarDisco() {
    for (int i = 0; i < MAX_ARCHIVOS; i++) {
        discoVirtual[i].id = i + 1;
        discoVirtual[i].activo = 0;
    }
}

void crearArchivo() {
    int libre = -1;
    for (int i = 0; i < MAX_ARCHIVOS; i++) {
        if (discoVirtual[i].activo == 0) {
            libre = i;
            break;
        }
    }

    if (libre == -1) {
        printf("[ERROR] Almacenamiento lleno. No hay sectores Libres.\n");
        return;
    }

    printf("Ingrese el nombre del archivo: ");
    fgets(discoVirtual[libre].nombre, TAM_NOMBRE, stdin);
    discoVirtual[libre].nombre[strcspn(discoVirtual[libre].nombre, "\n")] = 0;

    printf("Ingrese el contenido: ");
    fgets(discoVirtual[libre].contenido, TAM_CONTENIDO, stdin);
    discoVirtual[libre].contenido[strcspn(discoVirtual[libre].contenido, "\n")] = 0;

    discoVirtual[libre].activo = 1;
    printf("[OK] Archivo guardado con éxito en el Sector ID: %d\n", discoVirtual[libre].id);
}

void leerArchivos() {
    int cont = 0;
    printf("\n--- SECTORES DE MEMORIA SECUNDARIA OCUPADOS ---\n");
    for (int i = 0; i < MAX_ARCHIVOS; i++) {
        if (discoVirtual[i].activo == 1) {
            printf("ID: %d | Nombre: %s | Contenido: %s\n", discoVirtual[i].id, discoVirtual[i].nombre, discoVirtual[i].contenido);
            cont++;
        }
    }
    if (cont == 0) {
        printf("[INFO] El almacenamiento está vacío.\n");
    }
}

void actualizarArchivo() {
    int buscarId;
    printf("Ingrese el ID del archivo a modificar: ");
    scanf("%d", &buscarId);
    getchar();

    if (buscarId < 1 || buscarId > MAX_ARCHIVOS) {
        printf("[ERROR] Dirección de memoria fuera de rango.\n");
        return;
    }

    int indice = buscarId - 1;
    if (discoVirtual[indice].activo == 1) {
        printf("Nuevo contenido para '%s': ", discoVirtual[indice].nombre);
        fgets(discoVirtual[indice].contenido, TAM_CONTENIDO, stdin);
        discoVirtual[indice].contenido[strcspn(discoVirtual[indice].contenido, "\n")] = 0;
        printf("[OK] Sector ID %d actualizado.\n", buscarId);
    } else {
        printf("[ERROR] El sector seleccionado está vacío o fue eliminado.\n");
    }
}

void eliminarArchivo() {
    int buscarId;
    printf("Ingrese el ID del archivo a eliminar: ");
    scanf("%d", &buscarId);
    getchar();

    if (buscarId < 1 || buscarId > MAX_ARCHIVOS) {
        printf("[ERROR] Dirección de memoria inválida.\n");
        return;
    }

    int indice = buscarId - 1;
    if (discoVirtual[indice].activo == 1) {
        discoVirtual[indice].activo = 0;
        printf("[OK] Borrado lógico completado. Sector ID %d marcado como LIBRE.\n", buscarId);
    } else {
        printf("[ERROR] El sector ya se encuentra vacío.\n");
    }
}

void retoBsdGamesSnake() {
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    int ancho = 40, alto = 20;
    int posX = ancho / 2, posY = alto / 2;
    int frutaX = rand() % (ancho - 2) + 1;
    int frutaY = rand() % (alto - 2) + 1;
    int dirX = 1, dirY = 0;
    int puntaje = 0;
    int gameOver = 0;

    int colaX[100], colaY[100];
    int tamCola = 2;
    colaX[0] = posX - 1; colaY[0] = posY;
    colaX[1] = posX - 2; colaY[1] = posY;

    while (!gameOver) {
        clear();
        for (int i = 0; i < ancho + 2; i++) mvprintw(0, i, "#");
        for (int i = 0; i < ancho + 2; i++) mvprintw(alto + 1, i, "#");
        for (int i = 1; i <= alto; i++) {
            mvprintw(i, 0, "#");
            mvprintw(i, ancho + 1, "#");
        }

        mvprintw(frutaY, frutaX, "*");
        mvprintw(posY, posX, "O");
        for (int i = 0; i < tamCola; i++) {
            mvprintw(colaY[i], colaX[i], "o");
        }

        mvprintw(alto + 3, 2, "Controles: Flechas de dirección | Salir: 'q'");
        mvprintw(alto + 4, 2, "PUNTUACIÓN: %d", puntaje);
        refresh();

        int tecla = getch();
        switch (tecla) {
            case KEY_UP:    if(dirY == 0) { dirX = 0; dirY = -1; } break;
            case KEY_DOWN:  if(dirY == 0) { dirX = 0; dirY = 1; }  break;
            case KEY_LEFT:  if(dirX == 0) { dirX = -1; dirY = 0; } break;
            case KEY_RIGHT: if(dirX == 0) { dirX = 1; dirY = 0; }  break;
            case 'q': case 'Q': gameOver = 1; break;
        }

        int prevX = colaX[0], prevY = colaY[0];
        int prev2X, prev2Y;
        colaX[0] = posX;
        colaY[0] = posY;
        for (int i = 1; i < tamCola; i++) {
            prev2X = colaX[i]; prev2Y = colaY[i];
            colaX[i] = prevX;  colaY[i] = prevY;
            prevX = prev2X;    prevY = prev2Y;
        }

        posX += dirX;
        posY += dirY;

        if (posX <= 0 || posX >= ancho + 1 || posY <= 0 || posY >= alto + 1) {
            gameOver = 1;
        }
        for (int i = 0; i < tamCola; i++) {
            if (colaX[i] == posX && colaY[i] == posY) {
                gameOver = 1;
            }
        }
        if (posX == frutaX && posY == frutaY) {
            puntaje += 10;
            tamCola++;
            frutaX = rand() % (ancho - 2) + 1;
            frutaY = rand() % (alto - 2) + 1;
        }

        usleep(100000);
    }

    clear();
    mvprintw(alto/2, (ancho/2) - 5, "GAME OVER");
    mvprintw((alto/2) + 1, (ancho/2) - 10, "Puntaje Final: %d", puntaje);
    mvprintw((alto/2) + 3, (ancho/2) - 16, "Presione cualquier tecla para volver...");
    nodelay(stdscr, FALSE);
    refresh();
    getch();
    endwin();
}

// ==========================================
// TETRIS MEJORADO CON 7 PIEZAS Y COLISIONES
// ==========================================

// Formas de las 7 piezas de Tetris (Matriz 4x4)
const int TETROMINOS[7][4][4] = {
    // I
    {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    // J
    {
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // L
    {
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // O
    {
        {1,1,0,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // S
    {
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // T
    {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // Z
    {
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};

int piezaActual[4][4];
int tipoPieza;
int posX, posY;
int tableroTetris[20][10] = {0};
const int anchoTablero = 10;
const int altoTablero = 20;

void rotarPieza(int matriz[4][4]) {
    int temp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = matriz[3 - j][i];
        }
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            matriz[i][j] = temp[i][j];
        }
    }
}

int verificarColision(int nuevaX, int nuevaY, int matriz[4][4]) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (matriz[r][c]) {
                int boardX = nuevaX + c;
                int boardY = nuevaY + r;

                // Límites de la pantalla
                if (boardX < 0 || boardX >= anchoTablero || boardY >= altoTablero) {
                    return 1;
                }
                // Colisión con piezas fijas previas
                if (boardY >= 0 && tableroTetris[boardY][boardX]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void generarNuevaPieza() {
    tipoPieza = rand() % 7;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            piezaActual[i][j] = TETROMINOS[tipoPieza][i][j];
        }
    }
    posX = anchoTablero / 2 - 2;
    posY = 0;
}

void retoBsdGamesTetris() {
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    // Limpiar tablero al iniciar
    for (int y = 0; y < altoTablero; y++) {
        for (int x = 0; x < anchoTablero; x++) {
            tableroTetris[y][x] = 0;
        }
    }

    int offsetX = 10, offsetY = 2;
    int puntaje = 0;
    int gameOver = 0;
    int contadorCaida = 0;

    generarNuevaPieza();

    while (!gameOver) {
        clear();

        // 1. Dibujar bordes del tablero
        for (int i = 0; i <= altoTablero + 1; i++) {
            mvprintw(offsetY + i, offsetX - 1, "|");
            mvprintw(offsetY + i, offsetX + (anchoTablero * 2), "|");
        }
        for (int j = 0; j <= (anchoTablero * 2) + 1; j++) {
            mvprintw(offsetY + altoTablero + 1, offsetX - 1 + j, "-");
        }

        // Dibujar bloques fijos guardados en el tablero
        for (int y = 0; y < altoTablero; y++) {
            for (int x = 0; x < anchoTablero; x++) {
                if (tableroTetris[y][x] == 1) {
                    mvprintw(offsetY + y, offsetX + (x * 2), "[]");
                }
            }
        }

        // Dibujar pieza activa en caída
        for (int py = 0; py < 4; py++) {
            for (int px = 0; px < 4; px++) {
                if (piezaActual[py][px]) {
                    int drawY = offsetY + posY + py;
                    int drawX = offsetX + (posX + px) * 2;
                    if (drawY >= offsetY) {
                        mvprintw(drawY, drawX, "[]");
                    }
                }
            }
        }

        mvprintw(offsetY + altoTablero + 3, 2, "Controles: Flechas (Izq/Der/Abajo) | Arriba: Rotar | Salir: 'q'");
        mvprintw(offsetY + altoTablero + 4, 2, "PUNTUACIÓN TETRIS: %d", puntaje);
        refresh();

        // 2. Captura de teclado y movimiento
        int tecla = getch();
        switch (tecla) {
            case KEY_LEFT:
                if (!verificarColision(posX - 1, posY, piezaActual)) {
                    posX--;
                }
                break;
            case KEY_RIGHT:
                if (!verificarColision(posX + 1, posY, piezaActual)) {
                    posX++;
                }
                break;
            case KEY_DOWN:
                if (!verificarColision(posX, posY + 1, piezaActual)) {
                    posY++;
                }
                break;
            case KEY_UP: {
                // Rotar con comprobación para evitar atravesar paredes
                int piezaCopia[4][4];
                for(int i=0; i<4; i++)
                    for(int j=0; j<4; j++)
                        piezaCopia[i][j] = piezaActual[i][j];
                
                rotarPieza(piezaCopia);
                if (!verificarColision(posX, posY, piezaCopia)) {
                    for(int i=0; i<4; i++)
                        for(int j=0; j<4; j++)
                            piezaActual[i][j] = piezaCopia[i][j];
                }
                break;
            }
            case 'q': case 'Q':
                gameOver = 1;
                break;
        }

        // 3. Gravedad automática
        contadorCaida++;
        if (contadorCaida > 8) { // Velocidad de caída ajustable
            if (!verificarColision(posX, posY + 1, piezaActual)) {
                posY++;
            } else {
                // Fijar pieza al tablero
                for (int py = 0; py < 4; py++) {
                    for (int px = 0; px < 4; px++) {
                        if (piezaActual[py][px]) {
                            int bY = posY + py;
                            int bX = posX + px;
                            if (bY >= 0 && bY < altoTablero && bX >= 0 && bX < anchoTablero) {
                                tableroTetris[bY][bX] = 1;
                            }
                        }
                    }
                }

                // Verificar líneas completas
                for (int y = altoTablero - 1; y >= 0; y--) {
                    int lineaLlena = 1;
                    for (int x = 0; x < anchoTablero; x++) {
                        if (tableroTetris[y][x] == 0) {
                            lineaLlena = 0;
                            break;
                        }
                    }
                    if (lineaLlena) {
                        puntaje += 100;
                        // Bajar las líneas superiores
                        for (int ty = y; ty > 0; ty--) {
                            for (int tx = 0; tx < anchoTablero; tx++) {
                                tableroTetris[ty][tx] = tableroTetris[ty - 1][tx];
                            }
                        }
                        for (int tx = 0; tx < anchoTablero; tx++) {
                            tableroTetris[0][tx] = 0;
                        }
                        y++; // Revisar la misma fila de nuevo
                    }
                }

                // Generar nueva pieza o terminar juego si llega arriba
                generarNuevaPieza();
                if (verificarColision(posX, posY, piezaActual)) {
                    gameOver = 1;
                }
            }
            contadorCaida = 0;
        }

        usleep(50000); // 50ms
    }

    clear();
    mvprintw(offsetY + altoTablero/2, offsetX, "GAME OVER");
    mvprintw(offsetY + altoTablero/2 + 1, offsetX - 4, "Puntaje Final: %d", puntaje);
    mvprintw(offsetY + altoTablero/2 + 3, offsetX - 6, "Presione cualquier tecla...");
    nodelay(stdscr, FALSE);
    refresh();
    getch();
    endwin();
}
