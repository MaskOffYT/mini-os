#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include "kernel.h"
#include "seguridad.h"
#include "bitacora.h"

#define ARCH_MATRIZ DIR_DATOS "matriz.dat"
#define ARCH_USUARIOS DIR_DATOS "usuarios.dat"
#define MAX_INTENTOS  3

static const char *NOMBRE_ROL[NUM_ROLES] = { "admin", "operador", "invitado" };

static const char *NOMBRE_OBJ[NUM_OBJETOS] = {
    "registros", "juegos", "partidas", "procesos",
    "matriz", "sistema", "usuarios", "bitacora", "virtualiz"
};

static Usuario sesion;
static int     hay_sesion = 0;

/* POLÍTICA: filas = dominios (roles), columnas = 9 objetos protegidos */
static unsigned char matriz[NUM_ROLES][NUM_OBJETOS] = {
    /*                   registros             juegos  partidas              procesos              matriz          sistema         usuarios            bitacora  virtualizacion */
    /* admin    */ { D_CREATE|D_READ|D_UPDATE|D_DELETE, D_EXEC, D_READ,               D_READ|D_UPDATE|D_DELETE, D_READ|D_UPDATE, D_EXEC,         D_CREATE|D_READ|D_UPDATE, D_READ,   D_EXEC },
    /* operador */ { D_CREATE|D_READ|D_UPDATE,          D_EXEC, D_CREATE|D_READ|D_UPDATE, D_READ|D_UPDATE|D_DELETE, D_READ,          0,              0,                        0,        D_EXEC },
    /* invitado */ { D_READ,                            D_EXEC, 0,                    D_READ,                   0,               0,              0,                        0,        0      },
};

/* Hash didáctico (djb2 con sal) */
static unsigned long hash_clave(const char *clave, unsigned long sal) {
    unsigned long h = 5381 ^ sal;
    for (const unsigned char *p = (const unsigned char *)clave; *p; p++)
        h = ((h << 5) + h) + *p;
    return h;
}

static void leer_clave(const char *prompt, char *buf, int n) {
    struct termios viejo, nuevo;
    int tty = isatty(STDIN_FILENO);
    if (tty) {
        tcgetattr(STDIN_FILENO, &viejo);
        nuevo = viejo;
        nuevo.c_lflag &= ~ECHO; /* apagar el eco: la clave no se ve */
        tcsetattr(STDIN_FILENO, TCSANOW, &nuevo);
    }
    leer_linea(prompt, buf, n);
    if (tty) { tcsetattr(STDIN_FILENO, TCSANOW, &viejo); printf("\n"); }
}

static void crear_usuario(const char *nombre, const char *clave, Rol rol) {
    Usuario u;
    memset(&u, 0, sizeof u);
    strncpy(u.nombre, nombre, sizeof u.nombre - 1);
    u.sal  = ((unsigned long)rand() << 16) ^ (unsigned long)time(NULL);
    u.hash = hash_clave(clave, u.sal);
    u.rol  = (int)rol;
    u.intentos_fallidos = 0;
    u.bloqueado = 0;
    
    int fd = open(ARCH_USUARIOS, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) return;
    write(fd, &u, sizeof u);
    close(fd);
}

static int buscar_usuario(const char *nombre, Usuario *u, off_t *pos) {
    int fd = open(ARCH_USUARIOS, O_RDONLY);
    if (fd < 0) return 0;
    off_t p = 0;
    while (read(fd, u, sizeof *u) == sizeof *u) {
        if (strcmp(u->nombre, nombre) == 0) { 
            *pos = p; 
            close(fd); 
            return 1; 
        }
        p += sizeof *u;
    }
    close(fd);
    return 0;
}

static void guardar_usuario(const Usuario *u, off_t pos) {
    int fd = open(ARCH_USUARIOS, O_WRONLY);
    if (fd < 0) return;
    lseek(fd, pos, SEEK_SET);
    write(fd, u, sizeof *u);
    fsync(fd);
    close(fd);
}

int seg_login(void) {
    if (access(ARCH_USUARIOS, F_OK) != 0) {
        srand((unsigned)time(NULL) ^ (unsigned)getpid());
        crear_usuario("admin",    "admin123", ROL_ADMIN);
        crear_usuario("operador", "oper123",  ROL_OPERADOR);
        crear_usuario("invitado", "inv123",   ROL_INVITADO);
    }
    char nombre[32], clave[64];
    Usuario u;
    off_t pos;

    leer_linea("Usuario: ", nombre, sizeof nombre);
    if (feof(stdin)) exit(0);
    leer_clave("Contraseña: ", clave, sizeof clave);

    if (!buscar_usuario(nombre, &u, &pos)) {
        auditar("LOGIN", nombre, "LOGIN FALLIDO");
        printf("Usuario o contraseña incorrectos.\n");
        return 0;
    }
    if (u.bloqueado) {
        auditar("LOGIN", nombre, "CUENTA BLOQUEADA");
        printf("Cuenta bloqueada. Contacte al administrador.\n");
        return 0;
    }
    if (hash_clave(clave, u.sal) != u.hash) {
        u.intentos_fallidos++;
        if (u.intentos_fallidos >= MAX_INTENTOS) u.bloqueado = 1;
        guardar_usuario(&u, pos);
        auditar("LOGIN", nombre, u.bloqueado ? "BLOQUEO" : "LOGIN FALLIDO");
        printf("Usuario o contraseña incorrectos.\n");
        return 0;
    }
    u.intentos_fallidos = 0;
    guardar_usuario(&u, pos);
    sesion = u;
    hay_sesion = 1;
    auditar("LOGIN", nombre, "LOGIN EXITOSO");
    return 1;
}

void seg_cerrar_sesion(void) {
    auditar("LOGOUT", sesion.nombre, "OK");
    hay_sesion = 0;
    memset(&sesion, 0, sizeof sesion);
}

const char *seg_usuario_actual(void) { 
    return hay_sesion ? sesion.nombre : "-"; 
}

const char *seg_rol_actual(void) { 
    return hay_sesion ? NOMBRE_ROL[sesion.rol] : "-"; 
}

/* MECANISMO: Mediación completa con fallo seguro y auditoría integrada */
int seg_verificar(Objeto obj, unsigned char derecho) {
    char accion[16];
    if (derecho & D_CREATE) snprintf(accion, sizeof accion, "CREATE");
    else if (derecho & D_READ) snprintf(accion, sizeof accion, "READ");
    else if (derecho & D_UPDATE) snprintf(accion, sizeof accion, "UPDATE");
    else if (derecho & D_DELETE) snprintf(accion, sizeof accion, "DELETE");
    else if (derecho & D_EXEC) snprintf(accion, sizeof accion, "EXEC");
    else snprintf(accion, sizeof accion, "DERECHO_%02X", derecho);

    if (hay_sesion && (matriz[sesion.rol][obj] & derecho) == derecho) {
        auditar(accion, NOMBRE_OBJ[obj], "PERMITIDO");
        return 1;
    }
    auditar(accion, NOMBRE_OBJ[obj], "DENEGADO");
    printf("  ACCESO DENEGADO\n");
    return 0;
}

void seg_guardar_matriz(void) {
    int fd = open(ARCH_MATRIZ, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return;
    write(fd, matriz, sizeof matriz);
    fsync(fd);
    close(fd);
}

void seg_cargar_matriz(void) {
    int fd = open(ARCH_MATRIZ, O_RDONLY);
    if (fd < 0) {
        seg_guardar_matriz();
        return;
    }
    if (read(fd, matriz, sizeof matriz) != sizeof matriz) {
        printf("matriz.dat dañada: se usan valores por defecto\n");
    }
    close(fd);
}

/* GESTIÓN DE USUARIOS (Solo Admin) */
void seg_gestionar_usuarios(void) {
    if (sesion.rol != ROL_ADMIN) {
        printf("  ACCESO DENEGADO: Solo el administrador puede gestionar usuarios.\n");
        return;
    }

    int opcion;
    do {
        printf("\n--- GESTIÓN DE USUARIOS ---\n");
        printf(" 1. Listar usuarios\n");
        printf(" 2. Crear nuevo usuario\n");
        printf(" 3. Desbloquear usuario\n");
        printf(" 0. Volver al menú principal\n");
        opcion = leer_entero(" Opción: ");

        if (opcion == 1) {
            int fd = open(ARCH_USUARIOS, O_RDONLY);
            if (fd < 0) {
                printf("[ERROR] No se pudo abrir el archivo de usuarios.\n");
                continue;
            }
            Usuario u;
            printf("\n%-15s | %-10s | %-10s | %-10s\n", "NOMBRE", "ROL", "ESTADO", "INT. FALLIDOS");
            printf("-------------------------------------------------------------\n");
            while (read(fd, &u, sizeof(u)) == sizeof(u)) {
                const char *rol_str = (u.rol >= 0 && u.rol < NUM_ROLES) ? NOMBRE_ROL[u.rol] : "desconocido";
                const char *estado_str = u.bloqueado ? "BLOQUEADO" : "ACTIVO";
                printf("%-15s | %-10s | %-10s | %-13d\n", u.nombre, rol_str, estado_str, u.intentos_fallidos);
            }
            close(fd);
        } else if (opcion == 2) {
            char nombre[32], clave[64];
            leer_linea("Nombre del nuevo usuario: ", nombre, sizeof(nombre));
            if (strlen(nombre) == 0) {
                printf("[ERROR] Nombre inválido.\n");
                continue;
            }
            leer_clave("Contraseña inicial (min 6 caracteres): ", clave, sizeof(clave));
            if (strlen(clave) < 6) {
                printf("[ERROR] La contraseña debe tener al menos 6 caracteres.\n");
                continue;
            }
            int r = leer_entero("Rol (0=admin, 1=operador, 2=invitado): ");
            if (r < 0 || r >= NUM_ROLES) {
                printf("[ERROR] Rol inválido.\n");
                continue;
            }
            crear_usuario(nombre, clave, (Rol)r);
            printf("[OK] Usuario '%s' creado con éxito.\n", nombre);
            auditar("CREAR_USUARIO", nombre, "EXITO");
        } else if (opcion == 3) {
            char nombre[32];
            leer_linea("Nombre del usuario a desbloquear: ", nombre, sizeof(nombre));
            Usuario u;
            off_t pos;
            if (buscar_usuario(nombre, &u, &pos)) {
                u.bloqueado = 0;
                u.intentos_fallidos = 0;
                guardar_usuario(&u, pos);
                printf("[OK] El usuario '%s' ha sido desbloqueado.\n", nombre);
                auditar("DESBLOQUEAR_USUARIO", nombre, "EXITO");
            } else {
                printf("[ERROR] Usuario no encontrado.\n");
            }
        }
    } while (opcion != 0);
}

/* CAMBIAR MI CONTRASEÑA */
void seg_cambiar_password(void) {
    char clave_actual[64], clave_nueva[64], clave_conf[64];
    printf("\n--- CAMBIAR MI CONTRASEÑA ---\n");
    leer_clave("Ingrese su contraseña actual: ", clave_actual, sizeof(clave_actual));

    if (hash_clave(clave_actual, sesion.sal) != sesion.hash) {
        printf("[ERROR] La contraseña actual es incorrecta.\n");
        auditar("CAMBIAR_PASSWORD", sesion.nombre, "FALLIDO");
        return;
    }

    leer_clave("Nueva contraseña (mínimo 6 caracteres): ", clave_nueva, sizeof(clave_nueva));
    if (strlen(clave_nueva) < 6) {
        printf("[ERROR] La contraseña es muy corta. Debe tener al menos 6 caracteres.\n");
        return;
    }

    leer_clave("Confirme la nueva contraseña: ", clave_conf, sizeof(clave_conf));
    if (strcmp(clave_nueva, clave_conf) != 0) {
        printf("[ERROR] Las contraseñas no coinciden.\n");
        return;
    }

    Usuario u;
    off_t pos;
    if (buscar_usuario(sesion.nombre, &u, &pos)) {
        u.sal = ((unsigned long)rand() << 16) ^ (unsigned long)time(NULL);
        u.hash = hash_clave(clave_nueva, u.sal);
        guardar_usuario(&u, pos);
        sesion = u;
        printf("[OK] Contraseña actualizada exitosamente.\n");
        auditar("CAMBIAR_PASSWORD", sesion.nombre, "EXITO");
    } else {
        printf("[ERROR] Error interno al actualizar el usuario.\n");
    }
}

void seg_ver_matriz(void) {
    printf("\n=======================================================================================================================\n");
    printf("                                              MATRIZ DE ACCESO DE MINIOS                                               \n");
    printf("=======================================================================================================================\n");
    printf("%-10s | %-15s | %-15s | %-15s | %-15s | %-15s | %-15s | %-15s | %-15s | %-15s\n", 
           "ROL", "REGISTROS", "JUEGOS", "PARTIDAS", "PROCESOS", "MATRIZ", "SISTEMA", "USUARIOS", "BITACORA", "VIRTUALIZACION");
    printf("-----------------------------------------------------------------------------------------------------------------------\n");

    for (int r = 0; r < NUM_ROLES; r++) {
        printf("%-10s | ", NOMBRE_ROL[r]);
        for (int o = 0; o < NUM_OBJETOS; o++) {
            unsigned char d = matriz[r][o];
            char buf[6];
            buf[0] = (d & D_CREATE) ? 'C' : '-';
            buf[1] = (d & D_READ)   ? 'R' : '-';
            buf[2] = (d & D_UPDATE) ? 'U' : '-';
            buf[3] = (d & D_DELETE) ? 'D' : '-';
            buf[4] = (d & D_EXEC)   ? 'E' : '-';
            buf[5] = '\0';
            printf("%-15s | ", buf);
        }
        printf("\n");
    }
    printf("=======================================================================================================================\n");
}

void seg_modificar_matriz(void) {
    if (sesion.rol != ROL_ADMIN) {
        printf("  ACCESO DENEGADO: Solo el administrador puede modificar la matriz.\n");
        return;
    }

    printf("\n--- MODIFICAR MATRIZ DE ACCESO ---\n");
    int r = leer_entero("Rol a modificar (0=admin, 1=operador, 2=invitado): ");
    if (r < 0 || r >= NUM_ROLES) {
        printf("[ERROR] Rol inválido.\n");
        return;
    }

    int o = leer_entero("Objeto a modificar (0 a 8): ");
    if (o < 0 || o >= NUM_OBJETOS) {
        printf("[ERROR] Objeto inválido.\n");
        return;
    }

    int nuevos_derechos = leer_entero("Nuevos derechos (suma en decimal, ej: 31 para CRUD+E, 6 para R+U): ");
    if (nuevos_derechos < 0 || nuevos_derechos > 31) {
        printf("[ERROR] Derechos inválidos (0 a 31).\n");
        return;
    }

    if (r == ROL_ADMIN && o == OBJ_MATRIZ) {
        if ((nuevos_derechos & (D_READ | D_UPDATE)) != (D_READ | D_UPDATE)) {
            printf("[SEGURIDAD ERROR] El administrador no puede quitarse los permisos de lectura y actualización sobre la matriz.\n");
            return;
        }
    }

    matriz[r][o] = (unsigned char)nuevos_derechos;
    seg_guardar_matriz();
    printf("[OK] Matriz modificada y guardada en disco exitosamente.\n");
}
