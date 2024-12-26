#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

struct memory_info {
    unsigned long vmSize;     // Tamaño total de la memoria virtual
    unsigned long vmRSS;      // Tamaño de la memoria residente
    unsigned long mem_addr;   // Dirección de memoria asignada
};

#define SYS__202000173_tamalloc 550 // Cambia esto si tu syscall tiene otro número

// Variable global para controlar el bucle
volatile sig_atomic_t keep_running = 1;

// Manejador de señal para terminar el programa con Ctrl+C
void handle_signal(int signal) {
    keep_running = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <pid>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    struct memory_info mem_info;
    int ret;

    // Configura el manejador de señal para capturar Ctrl+C
    signal(SIGINT, handle_signal);

    printf("Monitoreando el proceso con PID %d. Presiona Ctrl+C para salir.\n", pid);

    // Bucle para actualizar y mostrar los valores en tiempo real
    while (keep_running) {
        // Llamada al syscall
        ret = syscall(SYS__202000173_tamalloc, pid, &mem_info);
        if (ret < 0) {
            fprintf(stderr, "Error en syscall: %s\n", strerror(errno));
            return 1;
        }

        // Conversión de bytes a kilobytes
        unsigned long vmSize_kb = mem_info.vmSize / 1024;
        unsigned long vmRSS_kb = mem_info.vmRSS / 1024;

        // Limpia la pantalla (opcional para simular "tiempo real")
        printf("\033[H\033[J");

        // Imprime los resultados actualizados
        printf("Resultados de la syscall _202000173_tamalloc:\n");
        printf("vmSize: %lu KB\n", vmSize_kb);
        printf("vmRSS: %lu KB\n", vmRSS_kb);
        printf("Dirección virtual asignada: 0x%lx\n", mem_info.mem_addr);

        // Espera 1 segundo antes de la siguiente actualización
        sleep(1);
    }

    printf("\nMonitoreo terminado. ¡Hasta luego!\n");
    return 0;
}
