#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "sistema.h"

void acerca_de(void) {
    struct utsname u;
    uname(&u);
    printf("MiniOS v3.0 sobre %s %s (%s)\n", u.sysname, u.release, u.machine);
    printf("Núcleos: %ld | PID: %d | UID Linux: %d\n",
        sysconf(_SC_NPROCESSORS_ONLN), getpid(), getuid());
}
