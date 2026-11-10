#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>

void jugarSnake() {
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    srand(time(NULL));

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
        for (int i = 0; i < ancho + 2; i++) {
            mvprintw(0, i, "#");
            mvprintw(alto + 1, i, "#");
        }
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
        mvprintw(alto + 4, 2, "PUNTUACIÓN SNAKE: %d", puntaje);
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

        usleep(100000); // 100ms
    }

    clear();
    mvprintw(alto/2, (ancho/2) - 5, "GAME OVER");
    mvprintw((alto/2) + 1, (ancho/2) - 10, "Puntaje Final: %d", puntaje);
    mvprintw((alto/2) + 3, (ancho/2) - 16, "Presione cualquier tecla para salir...");
    nodelay(stdscr, FALSE);
    refresh();
    getch();
    endwin();
}

int main() {
    jugarSnake();
    return 0;
}
