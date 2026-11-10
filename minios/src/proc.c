#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "proc.h"

void memoria_proceso(pid_t pid) {
    char ruta[64], linea[128];
    snprintf(ruta, sizeof ruta, "/proc/%d/status", pid);
    FILE *f = fopen(ruta, "r");
    if (!f) { 
        printf("El proceso con PID %d ya no existe.\n", pid); 
        return; 
    }
    
    printf("\n--- ESTADÍSTICAS DE MEMORIA (PID: %d) ---\n", pid);
    while (fgets(linea, sizeof linea, f)) {
        if (strncmp(linea, "VmSize", 6) == 0 || strncmp(linea, "VmRSS", 5) == 0) {
            printf("  %s", linea);
        }
    }
    fclose(f);
    
    long tam_pagina = sysconf(_SC_PAGESIZE);
    printf("  Tamaño de página del sistema: %ld bytes\n", tam_pagina);
    printf("-----------------------------------------\n");
}
