#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char datos[50];
} BloqueMemoria;

const char* DISCO = "memoria_secundaria.dat";

void crear_registro();
void leer_registros();
void actualizar_registro();
void eliminar_registro();

int main() {
    int opcion;

    while (1) { printf("\n--- MINIOS: GESTION DE MEMORIA SECUNDARIA ---\n"); printf("1. Crear Registro (Create)\n"); printf("2. Leer Registros (Read)\n"); printf("3. Actualizar Registro (Update)\n"); printf("4. Eliminar  Registro (Delete)\n"); printf("5. Salir del Sistema\n"); printf("Seleccione una opcion: ");

        if (scanf("%d", &opcion) != 1) {
            printf("Error: Entrada no valida.\n");
            while(getchar() != '\n');
            continue;
        }

        switch (opcion) {
            case 1:
                crear_registro();
                break;
            case 2:
                leer_registros();
                break;
            case 3:
                actualizar_registro();
                break;
            case 4:
                eliminar_registro();
                break;
            case 5:
                printf("Apagando Mini OS... ¡Adios!\n");
                exit(0);
            default:
                printf("Opcion incorrecta. Intente de nuevo.\n");
        }
    }

    return 0;
}

void crear_registro() {
    FILE *archivo = fopen(DISCO, "ab");
    if (archivo == NULL) {
        printf("Error al acceder a la memoria secundaria.\n");
        return;
    }

    BloqueMemoria nuevo;
    printf("Ingrese ID del bloque: ");
    scanf("%d", &nuevo.id);
    getchar();
    printf("Ingrese los datos a guardar: ");
    fgets(nuevo.datos, sizeof(nuevo.datos), stdin);
    nuevo.datos[strcspn(nuevo.datos, "\n")] = 0;

    fwrite(&nuevo, sizeof(BloqueMemoria), 1, archivo);
    fclose(archivo);
    printf("¡Bloque guardado con exito en %s!\n", DISCO);
}

void leer_registros() {
    FILE *archivo = fopen(DISCO, "rb");
    if (archivo == NULL) {
        printf("La memoria secundaria esta vacia o no ha sido creada.\n");
        return;
    }

    BloqueMemoria bloque;
    printf("\n--- CONTENIDO DE LA MEMORIA SECUNDARIA ---\n");
    while (fread(&bloque, sizeof(BloqueMemoria), 1, archivo)) {
        printf("ID: %d | Datos: %s\n", bloque.id, bloque.datos);
    }
    fclose(archivo);
}

void actualizar_registro() {
    FILE *archivo = fopen(DISCO, "r+b");
    if (archivo == NULL) {
        printf("Error al abrir la memoria secundaria.\n");
        return;
    }

    int id_buscar;
    printf("Ingrese el ID del bloque que desea actualizar: ");
    scanf("%d", &id_buscar);
    getchar();

    BloqueMemoria bloque;
    int encontrado = 0;

    while (fread(&bloque, sizeof(BloqueMemoria), 1, archivo)) {
        if (bloque.id == id_buscar) {
            printf("ID encontrado. Datos actuales: %s\n", bloque.datos);
            printf("Ingrese los nuevos datos: ");
            fgets(bloque.datos, sizeof(bloque.datos), stdin);
            bloque.datos[strcspn(bloque.datos, "\n")] = 0;

            fseek(archivo, -sizeof(BloqueMemoria), SEEK_CUR);
            fwrite(&bloque, sizeof(BloqueMemoria), 1, archivo);
            encontrado = 1;
            printf("¡Registro actualizado con \351xito!\n");
            break;
        }
    }
    if (!encontrado) printf("ID no encontrado.\n");
    fclose(archivo);
}

void eliminar_registro() {
    FILE *archivo = fopen(DISCO, "rb");
    if (archivo == NULL) {
        printf("Error al abrir la memoria secundaria.\n");
        return;
    }

    int id_eliminar;
    printf("Ingrese el ID del bloque a eliminar: ");
    scanf("%d", &id_eliminar);

    FILE *temporal = fopen("temp.dat", "wb");
    BloqueMemoria bloque;
    int encontrado = 0;

    while (fread(&bloque, sizeof(BloqueMemoria), 1, archivo)) {
        if (bloque.id == id_eliminar) {
            encontrado = 1;
        } else {
            fwrite(&bloque, sizeof(BloqueMemoria), 1, temporal);
        }
    }

    fclose(archivo);
    fclose(temporal);

    remove(DISCO);
    rename("temp.dat", DISCO);

    if (encontrado) printf("¡Bloque eliminado correctamente de la memoria!\n");
    else printf("El ID especificado no existe.\n");
}
