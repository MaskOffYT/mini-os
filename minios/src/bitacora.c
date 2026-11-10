#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "kernel.h"
#include "seguridad.h"
#include "bitacora.h"

#define ARCH_BITACORA DIR_DATOS "bitacora.log"

/* Estructura para llevar el conteo de denegaciones por usuario en la sesión actual */
typedef struct {
    char usuario[32];
    int denegaciones;
} ConteoDenegacion;

static ConteoDenegacion sesiones_denegadas[10];
static int total_conteo_usuarios = 0;

static void registrar_denegacion_sesion(const char *usuario) {
    if (!usuario || strcmp(usuario, "-") == 0) return;

    int encontrado = 0;
    for (int i = 0; i < total_conteo_usuarios; i++) {
        if (strcmp(sesiones_denegadas[i].usuario, usuario) == 0) {
            sesiones_denegadas[i].denegaciones++;
            encontrado = 1;
            if (sesiones_denegadas[i].denegaciones == 5) {
                printf("\n[ALERTA DE SEGURIDAD] El usuario '%s' ha acumulado 5 accesos denegados en esta sesión.\n", usuario);
                auditar("ALERTA", "sesion", "5_DENEGACIONES_ACUMULADAS");
            }
            break;
        }
    }
    if (!encontrado && total_conteo_usuarios < 10) {
        strncpy(sesiones_denegadas[total_conteo_usuarios].usuario, usuario, 31);
        sesiones_denegadas[total_conteo_usuarios].denegaciones = 1;
        total_conteo_usuarios++;
    }
}

void auditar(const char *accion, const char *objeto, const char *resultado) {
    char fecha[32], linea[256];
    time_t t = time(NULL);
    strftime(fecha, sizeof fecha, "%Y-%m-%d %H:%M:%S", localtime(&t));
    
    const char *usr = seg_usuario_actual();
    const char *rol = seg_rol_actual();

    int n = snprintf(linea, sizeof linea, "%s | %-10s | %-8s | %-14s | %-10s | %s\n",
                     fecha, usr, rol, accion, objeto, resultado);

    int fd = open(ARCH_BITACORA, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) return;
    write(fd, linea, n);
    close(fd);

    if (strstr(resultado, "DENEGADO") || strstr(resultado, "BLOQUEADO")) {
        registrar_denegacion_sesion(usr);
    }
}

void bitacora_mostrar(void) {
    FILE *f = fopen(ARCH_BITACORA, "r");
    if (!f) { 
        printf("Bitácora vacía.\n"); 
        return; 
    }

    char linea[256];
    int denegados = 0, fallidos = 0;

    /* Estructuras temporales para calcular el usuario con más denegaciones */
    typedef struct {
        char nombre[32];
        int count;
    } StatUser;
    StatUser stats[20];
    int num_stats = 0;

    printf("\n===================================================================================================\n");
    printf("                                      BITÁCORA DE AUDITORÍA (LOG)                                  \n");
    printf("===================================================================================================\n");

    while (fgets(linea, sizeof linea, f)) {
        printf("%s", linea);
        if (strstr(linea, "DENEGADO")) {
            denegados++;
            /* Extraer usuario de la línea (formato: FECHA | USUARIO | ROL | ...) */
            char copia[256];
            strncpy(copia, linea, sizeof(copia));
            char *token = strtok(copia, "|"); // fecha
            if (token) {
                token = strtok(NULL, "|"); // usuario
                if (token) {
                    // limpiar espacios
                    while (*token == ' ') token++;
                    char *end = token + strlen(token) - 1;
                    while (end > token && (*end == ' ' || *end == '\n' || *end == '\r')) *end-- = '\0';

                    if (strlen(token) > 0 && strcmp(token, "-") != 0) {
                        int found = 0;
                        for (int i = 0; i < num_stats; i++) {
                            if (strcmp(stats[i].nombre, token) == 0) {
                                stats[i].count++;
                                found = 1;
                                break;
                            }
                        }
                        if (!found && num_stats < 20) {
                            strncpy(stats[num_stats].nombre, token, 31);
                            stats[num_stats].count = 1;
                            num_stats++;
                        }
                    }
                }
            }
        }
        if (strstr(linea, "LOGIN FALLIDO")) {
            fallidos++;
        }
    }
    fclose(f);

    /* Encontrar el usuario con más denegaciones */
    char *max_usuario = "Ninguno";
    int max_denegaciones = 0;
    for (int i = 0; i < num_stats; i++) {
        if (stats[i].count > max_denegaciones) {
            max_denegaciones = stats[i].count;
            max_usuario = stats[i].nombre;
        }
    }

    printf("===================================================================================================\n");
    printf("\n--- RESUMEN DE SEGURIDAD ---\n");
    printf(" Total accesos denegados : %d\n", denegados);
    printf(" Total logins fallidos   : %d\n", fallidos);
    printf(" Usuario con más deneg.  : %s (%d denegaciones)\n", max_usuario, max_denegaciones);
    printf("===================================================================================================\n");
}
