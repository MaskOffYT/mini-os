#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "juego.h"
#include "fs.h" // Asegura tener la definición de Partida y guardar_partida

static Partida         partida;                  /* Estado compartido por los dos hilos */
static pthread_mutex_t mutex_partida = PTHREAD_MUTEX_INITIALIZER;
static volatile int    jugando = 0;

static void *hilo_autoguardado(void *arg) {
    int segundos = *(int *)arg;
    while (jugando) {
        sleep(segundos);
        Partida copia;
        pthread_mutex_lock(&mutex_partida);     /* Sección crítica: leer el estado */
        copia = partida;
        pthread_mutex_unlock(&mutex_partida);
        guardar_partida(&copia);                /* La escritura en disco va FUERA del candado */
    }
    return NULL;
}

void iniciar_juego_con_autoguardado(void) {
    int cada = 15;
    pthread_t hilo;
    jugando = 1;
    
    // Inicializar partida o cargar existente aquí si es necesario...
    
    pthread_create(&hilo, NULL, hilo_autoguardado, &cada);

    // --- CICLO DEL JUEGO SIMULADO ---
    printf("\n[JUEGO] Iniciando partida. Autoguardado activo cada %d segundos.\n", cada);
    printf("[JUEGO] Presione Enter para salir del juego...\n");

    while (jugando) {
        pthread_mutex_lock(&mutex_partida);
        // Simulación de cambio de estado (ej: mover serpiente, sumar puntaje)
        partida.puntaje += 10; 
        pthread_mutex_unlock(&mutex_partida);
        
        sleep(3); // Simula el ciclo del juego
        break;    // En una implementación real, esto se rompe por tecla del usuario
    }

    /* Al salir del juego */
    jugando = 0;
    pthread_join(hilo, NULL);
    printf("[JUEGO] Partida finalizada y guardada de forma segura.\n");
}
