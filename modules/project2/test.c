#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <time.h>

#define SYS_TAMALLOC 552 // Definimos el número de syscall para tamalloc

int main() {
    // Mensaje de bienvenida y visualización del PID del programa
    printf("\033[1;32m=== Bienvenido al programa de tamalloc ===\033[0m\n");
    printf("\033[1;34mPID del programa: %d\033[0m\n", getpid());

    // Explicamos el propósito del programa
    printf("\033[1;36mEste programa asigna memoria usando tamalloc.\033[0m\n");
    printf("\033[1;33mPresiona ENTER para continuar...\033[0m\n");
    getchar(); // Espera a que el usuario presione ENTER

    size_t total_size = 10 * 1024 * 1024; // Tamaño predeterminado de memoria a asignar: 10 MB

    // Llamamos a la syscall tamalloc para asignar memoria
    char *buffer = (char *)syscall(SYS_TAMALLOC, total_size);
    if ((long)buffer < 0) {
        // Si la asignación falla, mostramos un mensaje de error
        perror("\033[1;31mError: La llamada a tamalloc falló\033[0m");
        return 1;
    }
    // Mostramos la dirección de memoria asignada
    printf("\033[1;32mDirección de memoria: %p\033[0m\n", buffer);

    // Informamos que comenzaremos a leer y escribir en la memoria
    printf("\033[1;33mPresiona ENTER para comenzar a leer memoria byte por byte...\033[0m\n");
    getchar(); // Espera a que el usuario presione ENTER

    srand(time(NULL)); // Inicializamos el generador de números aleatorios

    // Iteramos sobre la memoria asignada
    for (size_t i = 0; i < total_size; i++) {
        char t = buffer[i]; // Leemos el byte actual

        // Verificamos si el byte no está inicializado a 0
        if (t != 0) {
            printf("\033[1;31mERROR FATAL: La memoria en el byte %zu no estaba inicializada a 0\033[0m\n", i);
            return 10; // Terminamos el programa con un código de error
        }

        // Escribimos un caracter aleatorio entre 'A' y 'Z' en la memoria
        char random_letter = 'A' + (rand() % 26);
        buffer[i] = random_letter;

        // Mostramos progreso cada 1 MB de memoria procesada
        if (i % (1024 * 1024) == 0 && i > 0) { 
            printf("\033[1;34mProgreso: %zu MB procesados...\033[0m\n", i / (1024 * 1024));
            sleep(1); // Pausa de 1 segundo para simular "tiempo real"
        }
    }

    // Informamos que toda la memoria fue verificada y procesada correctamente
    printf("\033[1;32mTodas las regiones de memoria verificadas y llenadas correctamente.\033[0m\n");
    printf("\033[1;33mPresiona ENTER para finalizar el programa.\033[0m\n");
    getchar(); // Espera a que el usuario presione ENTER

    // Mensaje de despedida
    printf("\033[1;36mGracias por usar tamalloc. Hasta la próxima!\033[0m\n");
    return 0;
}
