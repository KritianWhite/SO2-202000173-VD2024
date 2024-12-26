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

#define SYS__202000173_tamalloc 550

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


    // Bucle para actualizar y mostrar los valores en tiempo real
    while (keep_running) {
        // Llamada al syscall
        ret = syscall(SYS__202000173_tamalloc, pid, &mem_info);
        if (ret < 0) {
            fprintf(stderr, "\033[1;31mError en syscall: \033[1;34m%s\n", strerror(errno));
            return 1;
        }

        // Conversión de bytes a kilobytes
        unsigned long vmSize_kb = mem_info.vmSize / 1024;
        unsigned long vmRSS_kb = mem_info.vmRSS / 1024;

        // Limpia la pantalla (opcional para simular "tiempo real")
        printf("\033[H\033[J");
        printf("\033[1;34m=================================================================\033[0m\n");
        printf("\033[1;36m                     MONITOREO DE MEMORIA                        \033[0m\n");
        printf("\033[1;34m=================================================================\033[0m\n");
        printf("\033[1;37mMonitoreando el proceso con PID %d. \n(Presiona Ctrl+C para salir.)\n \n", pid);
        printf("\033[1;34m-----------------------------------------------------------------\033[0m\n");

        // Imprime los resultados actualizados
        printf("\033[1;37m| %-35s | %-23s |\033[0m\n", "Campo", "Valor");
        printf("\033[1;34m-----------------------------------------------------------------\033[0m\n");
        printf("\033[1;37m| %-36s | \033[1;32m%-20lu KB\033[1;37m |\033[0m\n", "Tamaño total de memoria (vmSize)", vmSize_kb);
        printf("\033[1;37m| %-30s | \033[1;32m%-20lu KB\033[1;37m |\033[0m\n", "Tamaño de memoria residente (vmRSS)", vmRSS_kb);
        printf("\033[1;37m| %-36s | \033[1;34m0x%-18lx\033[1;37m    |\033[0m\n", "Dirección de memoria asignada", mem_info.mem_addr);
        printf("\033[1;34m-----------------------------------------------------------------\033[0m\n");

        // Espera 1 segundo antes de la siguiente actualización
        sleep(1);
    }

    printf("\033[1;34m\n=================================================================\033[0m\n");
    printf("\033[1;36m             MONITOREO FINALIZADO. ¡HASTA LUEGO!                 \033[0m\n");
    printf("\033[1;34m=================================================================\033[0m\n");
    return 0;
}
