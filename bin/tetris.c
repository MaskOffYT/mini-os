#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>

const int TETROMINOS[7][4][4] = {
    { {0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0} }, // I
    { {1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0} }, // J
    { {0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0} }, // L
    { {1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0} }, // O
    { {0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0} }, // S
    { {0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0} }, // T
    { {1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0} }  // Z
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
                if (boardX < 0 || boardX >= anchoTablero || boardY >= altoTablero) return 1;
                if (boardY >= 0 && tableroTetris[boardY][boardX]) return 1;
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

void jugarTetris() {
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    srand(time(NULL));

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
        for (int i = 0; i <= altoTablero + 1; i++) {
            mvprintw(offsetY + i, offsetX - 1, "|");
            mvprintw(offsetY + i, offsetX + (anchoTablero * 2), "|");
        }
        for (int j = 0; j <= (anchoTablero * 2) + 1; j++) {
            mvprintw(offsetY + altoTablero + 1, offsetX - 1 + j, "-");
        }

        for (int y = 0; y < altoTablero; y++) {
            for (int x = 0; x < anchoTablero; x++) {
                if (tableroTetris[y][x] == 1) {
                    mvprintw(offsetY + y, offsetX + (x * 2), "[]");
                }
            }
        }

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

        mvprintw(offsetY + altoTablero + 3, 2, "Controles: Flechas | Arriba: Rotar | Salir: 'q'");
        mvprintw(offsetY + altoTablero + 4, 2, "PUNTUACIÓN TETRIS: %d", puntaje);
        refresh();

        int tecla = getch();
        switch (tecla) {
            case KEY_LEFT:
                if (!verificarColision(posX - 1, posY, piezaActual)) posX--;
                break;
            case KEY_RIGHT:
                if (!verificarColision(posX + 1, posY, piezaActual)) posX++;
                break;
            case KEY_DOWN:
                if (!verificarColision(posX, posY + 1, piezaActual)) posY++;
                break;
            case KEY_UP: {
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

        contadorCaida++;
        if (contadorCaida > 10) {
            if (!verificarColision(posX, posY + 1, piezaActual)) {
                posY++;
            } else {
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
                        for (int ty = y; ty > 0; ty--) {
                            for (int tx = 0; tx < anchoTablero; tx++) {
                                tableroTetris[ty][tx] = tableroTetris[ty - 1][tx];
                            }
                        }
                        for (int tx = 0; tx < anchoTablero; tx++) {
                            tableroTetris[0][tx] = 0;
                        }
                        y++;
                    }
                }

                generarNuevaPieza();
                if (verificarColision(posX, posY, piezaActual)) {
                    gameOver = 1;
                }
            }
            contadorCaida = 0;
        }
        usleep(50000);
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

int main() {
    jugarTetris();
    return 0;
}
