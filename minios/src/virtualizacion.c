#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "virtualizacion.h"

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

static void leer_dmi(const char *campo, char *buf, int n) {
    char ruta[96];
    snprintf(ruta, sizeof ruta, "/sys/class/dmi/id/%s", campo);
    FILE *f = fopen(ruta, "r");
    strcpy(buf, "(no disponible)");
    if (f) { 
        if (fgets(buf, n, f)) 
            buf[strcspn(buf, "\n")] = '\0'; 
        fclose(f); 
    }
}

/* Función auxiliar para verificar si EPT (Intel) o NPT (AMD) están presentes en /proc/cpuinfo */
static int verificar_ept_npt(void) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) return 0;
    char linea[256];
    int soporta = 0;
    while (fgets(linea, sizeof(linea), f)) {
        /* Intel usa 'ept', AMD usa 'npt' o 'Nested Paging' */
        if (strstr(linea, "ept") || strstr(linea, "npt") || strstr(linea, "nested paging")) {
            soporta = 1;
            break;
        }
    }
    fclose(f);
    return soporta;
}

void virt_info(void) {
    int en_vm = 0;
    int vtx_expuesto = 0;
    int amdv_expuesto = 0;
    char fabricante[13] = {0};
    char firma[13] = "(ninguna)";
    char tipo_hipervisor[32] = "Hardware Nativo / Desconocido";

#if defined(__x86_64__) || defined(__i386__)
    unsigned int a, b, c, d;

    __get_cpuid(0, &a, &b, &c, &d);                           /* hoja 0: fabricante */
    memcpy(fabricante, &b, 4);
    memcpy(fabricante + 4, &d, 4); 
    memcpy(fabricante + 8, &c, 4);
    printf("CPU Fabricante: %s\n", fabricante);

    __get_cpuid(1, &a, &b, &c, &d);                           /* hoja 1: capacidades */
    en_vm = (c >> 31) & 1;                                    /* ECX[31] = bit hypervisor */
    printf("Dentro de una VM:   %s\n", en_vm ? "SÍ" : "NO");

    /* Detección de VT-x (Intel) en ECX[5] */
    if (strcmp(fabricante, "GenuineIntel") == 0) {
        vtx_expuesto = (c >> 5) & 1;
        printf("VT-x expuesto:      %s\n", vtx_expuesto ? "sí" : "no");
    }

    /* Detección de AMD-V (AMD) usando hoja 0x80000001, bit 2 de ECX */
    if (strcmp(fabricante, "AuthenticAMD") == 0) {
        unsigned int ax, bx, cx, dx;
        if (__get_cpuid(0x80000001, &ax, &bx, &cx, &dx)) {
            amdv_expuesto = (cx >> 2) & 1; /* ECX[2] = SVM (Secure Virtual Machine / AMD-V) */
            printf("AMD-V expuesto:     %s\n", amdv_expuesto ? "sí" : "no");
        }
    }

    if (en_vm) {
        __cpuid(0x40000000, a, b, c, d);                      /* hoja del hipervisor */
        memcpy(firma, &b, 4);
        memcpy(firma + 4, &c, 4); 
        memcpy(firma + 8, &d, 4);
        printf("Firma del hipervisor: %s\n", firma);

        if (strstr(firma, "VBox")) strcpy(tipo_hipervisor, "VirtualBox (Hipervisor tipo 2)");
        else if (strstr(firma, "VMware")) strcpy(tipo_hipervisor, "VMware (Hipervisor tipo 2/1)");
        else if (strstr(firma, "KVM")) strcpy(tipo_hipervisor, "KVM (Hipervisor tipo 1)");
        else if (strstr(firma, "Microsoft")) strcpy(tipo_hipervisor, "Hyper-V / WSL2");
        else if (strstr(firma, "Xen")) strcpy(tipo_hipervisor, "Xen (Hipervisor tipo 1)");
    }

#else
    printf("CPUID solo existe en procesadores x86. Entorno ARM detectado.\n");
    strcpy(fabricante, "ARM / Apple Silicon");
#endif

    int tiene_ept_npt = verificar_ept_npt();
    printf("Soporte EPT / NPT:    %s\n", tiene_ept_npt ? "SÍ" : "NO");

    char fab[96], prod[96];
    leer_dmi("sys_vendor", fab, sizeof fab);
    leer_dmi("product_name", prod, sizeof prod);
    printf("DMI (Sistema):      %s / %s\n", fab, prod);

    printf("systemd-detect-virt: ");
    fflush(stdout);
    system("systemd-detect-virt 2>/dev/null || echo no disponible");

    /* Frase de conclusión requerida */
    printf("\n===================================================================================================\n");
    printf("CONCLUSIÓN: MiniOS corre dentro de %s, sobre un procesador %s.\n", 
           en_vm ? tipo_hipervisor : "Hardware Físico (Bare-Metal)", 
           strlen(fabricante) > 0 ? fabricante : "Desconocido");
    printf("===================================================================================================\n");
}
