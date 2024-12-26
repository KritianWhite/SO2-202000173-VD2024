#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#ifndef __NR__202000173_tamalloc_stats
#define __NR__202000173_tamalloc_stats 552
#endif

static inline long tamalloc_get_stat(size_t size)
{
    return syscall(__NR__202000173_tamalloc_stats, size);
}

void sleep_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[])
{
    size_t size = 1024 * 1024; // Tamaño predeterminado: 1 MB
    if (argc > 1)
        size = (size_t)atol(argv[1]);

    while (1) {
        system("clear"); // Limpia la pantalla (simulación de tiempo real)

        long addr = tamalloc_get_stat(size);
        if (addr < 0) {
            perror("tamalloc_get_stat syscall");
            return 1;
        }

        printf("[Tiempo Real] Se asignaron %zu bytes en 0x%lx\n", size, addr);

        // Simulación de escritura para demostrar interacción
        char *ptr = (char *)addr;
        if (addr != 0) {
            ptr[0] = 'H';           // Escribir algo al inicio
            ptr[size - 1] = 'Z';    // Escribir algo al final
            printf("Datos escritos: Inicio = '%c', Final = '%c'\n", ptr[0], ptr[size - 1]);
        }

        // Espera 1 segundo antes de actualizar
        sleep_ms(1000);
    }

    return 0;
}
