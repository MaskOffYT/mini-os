#include <stdio.h>
#include "kernel.h"
#include "fs.h"
#include "seguridad.h"
#include "bitacora.h"
#include "sistema.h"
#include "proc.h"
#include "virtualizacion.h"

static void mostrar_menu(void) {
    printf("\n===== MiniOS v3.0 | usuario: %s (%s) =====\n", seg_usuario_actual(), seg_rol_actual());
    printf(" 1. Create     2. Read    3. Update    4. Delete\n");
    printf(" 5. Procesos   6. Ver Matriz   7. Mod. Matriz\n");
    printf(" 8. Bitácora   11. Virtualización   12. Cerrar sesión   0. Apagar\n");
}

int main(void) {
    if (fs_montar(DIR_DATOS "minios.disk") != 0) return 1;
    seg_cargar_matriz();
    auditar("ARRANQUE", "sistema", "OK");

    while (1) {
        while (!seg_login()) { } /* Bucle de login */

        int en_sesion = 1;
        while (en_sesion) {
            mostrar_menu();
            switch (leer_entero(" Opción: ")) {
                case 1:  if (seg_verificar(OBJ_REGISTROS, D_CREATE)) fs_crear();      break;
                case 2:  if (seg_verificar(OBJ_REGISTROS, D_READ))  fs_listar();      break;
                case 3:  if (seg_verificar(OBJ_REGISTROS, D_UPDATE)) fs_actualizar(); break;
                case 4:  if (seg_verificar(OBJ_REGISTROS, D_DELETE)) fs_borrar();     break;
                case 5:  if (seg_verificar(OBJ_PROCESOS, D_EXEC))   proc_menu_gestion(); break;
                case 6:  if (seg_verificar(OBJ_MATRIZ, D_READ))     seg_ver_matriz(); break;
                case 7:  if (seg_verificar(OBJ_MATRIZ, D_UPDATE))   seg_modificar_matriz(); break;
                case 8:  if (seg_verificar(OBJ_BITACORA, D_READ)) bitacora_mostrar(); break;
	        case 9:  if (seg_verificar(OBJ_USUARIOS, D_READ))     seg_gestionar_usuarios(); break;
		case 10: seg_cambiar_password(); break; // La contraseña la puede cambiar cualquier usuario logueado
                case 11: if (seg_verificar(OBJ_VIRTUALIZACION, D_EXEC)) virt_info(); break;
                case 12: 
                    seg_cerrar_sesion(); 
                    en_sesion = 0; 
                    break;
                case 0:
                    if (seg_verificar(OBJ_SISTEMA, D_EXEC)) {
                        auditar("APAGADO", "sistema", "OK");
                        fs_desmontar();
                        return 0;
                    }
                    break;
                default:
                    if (feof(stdin)) {
                        fs_desmontar(); return 0;
                    }
                    printf("Opción inválida.\n");
            }
        }
    }
}
