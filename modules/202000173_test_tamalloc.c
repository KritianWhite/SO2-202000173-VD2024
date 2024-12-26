#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#ifndef __NR__202000173_tamalloc
#define __NR__202000173_tamalloc 550
#endif

/*
 * Estructura utilizada para recopilar información global sobre el uso de memoria
 * del sistema mediante la syscall _202000173_tamalloc.
 *
 * Campos:
 *   - aggregate_vm_mb: Memoria virtual total agregada de todos los procesos, en MB.
 *   - aggregate_rss_mb: Memoria residente total agregada de todos los procesos, en MB.
 */
struct tamalloc_global_info {
    unsigned long aggregate_vm_mb;
    unsigned long aggregate_rss_mb;
};

/*
 * Llama a la syscall _202000173_tamalloc para obtener estadísticas globales de memoria.
 *
 * Argumentos:
 *   - info: Puntero a una estructura tamalloc_global_info para almacenar los resultados.
 *
 * Retorno:
 *   - 0 si la operación fue exitosa.
 *   - Códigos de error negativos en caso de fallo.
 */
static inline long tamalloc_get_global_stats(struct tamalloc_global_info *info)
{
    return syscall(__NR__202000173_tamalloc, info);
}

/*
 * Imprime las estadísticas globales de memoria en un formato estilizado.
 *
 * Argumentos:
 *   - info: Puntero a una estructura tamalloc_global_info con los datos globales.
 */
static void print_global_info(const struct tamalloc_global_info *info)
{
    printf("\033[1;34m==================================================================\033[0m\n");
    printf("\033[1;36m                      MONITOREO DE MEMORIA                        \033[0m\n");
    printf("\033[1;34m==================================================================\033[0m\n");
    printf("\033[1;37m| %-36s | \033[1;32m%-20lu MB\033[1;37m |\033[0m\n", "vmSize", info->aggregate_vm_mb);
    printf("\033[1;37m| %-36s | \033[1;32m%-20lu MB\033[1;37m |\033[0m\n", "vmRSS", info->aggregate_rss_mb);
    printf("\033[1;34m==================================================================\033[0m\n");
}

/*
 * Manejador de señales para interrumpir el programa
 */
void signal_handler(int signum) {
    printf("\n\033[1;31m[TERMINADO]\033[0m Monitoreo de memoria interrumpido.\n");
    exit(0);
}

/*
 * Programa principal para mostrar información global de memoria en tiempo real.
 */
int main(void)
{
    struct tamalloc_global_info info;

    // Capturar la señal SIGINT (Ctrl+C) para salir del programa
    signal(SIGINT, signal_handler);

    // Bucle infinito para actualizar en tiempo real
    while (1) {
        // Llamamos a la syscall para obtener las estadísticas
        long ret = tamalloc_get_global_stats(&info);
        if (ret < 0) {
            perror("tamalloc_get_global_stats syscall");
            return 1;
        }

        // Limpiamos la pantalla para simular tiempo real
        printf("\033[H\033[J");

        // Imprimimos las estadísticas globales
        print_global_info(&info);

        // Retardo de 1 segundo antes de la próxima actualización
        usleep(1000000);
    }

    return 0;
}
