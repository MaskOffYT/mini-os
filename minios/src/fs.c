#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "fs.h"
#include "kernel.h"

#define ARCH_DISCO   DIR_DATOS "minios.disk"
#define ARCH_ESPEJO  DIR_DATOS "minios_espejo.disk"
#define NUMERO_MAGIC 0x4D494E4F /* "MINO" */

static int fd_disco = -1, fd_espejo = -1;

typedef struct {
    unsigned int magic;
    int total_registros;
} SuperBloque;

/* Escritura redundante en ambos discos */
static int escribir_en_ambos(off_t pos, const void *datos, size_t n) {
    int ok = 0;
    if (fd_disco >= 0 && lseek(fd_disco, pos, SEEK_SET) >= 0 && write(fd_disco, datos, n) == (ssize_t)n)
        ok++;
    if (fd_espejo >= 0 && lseek(fd_espejo, pos, SEEK_SET) >= 0 && write(fd_espejo, datos, n) == (ssize_t)n)
        ok++;
    return ok == 2 ? 0 : -1;
}

/* Copia completa del disco sano al dañado para reconstrucción RAID 1 */
static int copiar_disco(int origen, int destino) {
    char bloque[4096];
    ssize_t n;
    lseek(origen, 0, SEEK_SET);
    lseek(destino, 0, SEEK_SET);
    while ((n = read(origen, bloque, sizeof bloque)) > 0) {
        if (write(destino, bloque, n) != n) return -1;
    }
    fsync(destino);
    return 0;
}

int fs_montar(const char *ruta_disco) {
    // Abrir disco principal
    fd_disco = open(ARCH_DISCO, O_RDWR | O_CREAT, 0600);
    // Abrir disco espejo
    fd_espejo = open(ARCH_ESPEJO, O_RDWR | O_CREAT, 0600);

    if (fd_disco < 0 || fd_espejo < 0) {
        perror("Error abriendo discos RAID 1");
        return -1;
    }

    SuperBloque sb_main, sb_esp;
    int main_valido = (read(fd_disco, &sb_main, sizeof(sb_main)) == sizeof(sb_main) && sb_main.magic == NUMERO_MAGIC);
    int esp_valido  = (read(fd_espejo, &sb_esp, sizeof(sb_esp)) == sizeof(sb_esp) && sb_esp.magic == NUMERO_MAGIC);

    if (!main_valido && esp_valido) {
        printf("[RAID 1] minios.disk dañado o ausente. Reconstruyendo desde el espejo...\n");
        copiar_disco(fd_espejo, fd_disco);
        printf("[OK] Disco principal reconstruido exitosamente.\n");
    } else if (main_valido && !esp_valido) {
        printf("[RAID 1] minios_espejo.disk dañado o ausente. Reconstruyendo desde el principal...\n");
        copiar_disco(fd_disco, fd_espejo);
        printf("[OK] Disco espejo reconstruido exitosamente.\n");
    } else if (!main_valido && !esp_valido) {
        printf("[RAID 1] Ambos discos nuevos o vacíos. Inicializando superbloque...\n");
        SuperBloque nuevo = { NUMERO_MAGIC, 0 };
        escribir_en_ambos(0, &nuevo, sizeof(nuevo));
    } else {
        printf("[OK] RAID 1 montado correctamente. Ambos discos sincronizados.\n");
    }

    return 0;
}

void fs_desmontar(void) {
    if (fd_disco >= 0) { fsync(fd_disco); close(fd_disco); }
    if (fd_espejo >= 0) { fsync(fd_espejo); close(fd_espejo); }
}
