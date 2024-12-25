#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

static unsigned long get_process_rss_kb(pid_t pid)
{
    char path[64];
    FILE *f;
    unsigned long rss_kb = 0;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f) {
        // Si el proceso no existe o no hay permisos
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // Ejemplo: "VmRSS:     1234 kB"
            sscanf(line, "VmRSS: %lu kB", &rss_kb);
            break;
        }
    }
    fclose(f);
    return rss_kb;
}

// Estructura simple para guardar la info de cada bloque mapeado
struct allocation {
    void   *addr;
    size_t  size;
};

static struct allocation *blocks       = NULL; // Arreglo dinámico de asignaciones
static size_t             blocks_count = 0;    // Cuántos bloques tenemos
static bool               stop_loop    = false;

/*
* ---------------------------------------------------------------------
* Manejador de señal Ctrl+C
* ---------------------------------------------------------------------
*/
static void handle_sigint(int sig)
{
    (void)sig; // no se usa
    stop_loop = true;
}

/*
* main
*/
int main(int argc, char *argv[])
{
    // Cantidad de MB a asignar en cada iteración
    // Ajusta este valor para “llenar” más rápido o más lento
    size_t chunk_mb = 10;

    // Instalar handler de Ctrl+C
    signal(SIGINT, handle_sigint);

    printf("Se asignarán bloques de %zu MB indefinidamente...\n", chunk_mb);
    printf("Presiona Ctrl+C para terminar.\n");

    while (!stop_loop) {
        // Limpia pantalla con secuencia ANSI
        printf("\033[H\033[J");

        // 1) Reserva un nuevo bloque con mmap
        size_t block_size = chunk_mb * 1024 * 1024;
        void *addr = mmap(
            NULL,
            block_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0
        );
        if (addr == MAP_FAILED) {
            perror("mmap");
            fprintf(stderr, "No se pudo asignar otro bloque. Saliendo...\n");
            break;
        }

        // Guardar en nuestro array dinámico
        //    - Realloc para blocks_count + 1
        struct allocation *new_blocks = realloc(blocks, (blocks_count + 1) * sizeof(*blocks));
        if (!new_blocks) {
            perror("realloc");
            // Liberar el bloque recién asignado, porque no pudimos ampliar el array
            munmap(addr, block_size);
            break;
        }
        blocks = new_blocks;
        blocks[blocks_count].addr = addr;
        blocks[blocks_count].size = block_size;
        blocks_count++;

        // “Tocar” el nuevo bloque para llenar la memoria y disparar page faults
        memset(addr, 0xFF, block_size);

        // Calcular cuánta memoria se ha asignado en total (en MB)
        size_t total_bytes = 0;
        for (size_t i = 0; i < blocks_count; i++) {
            total_bytes += blocks[i].size;
        }
        size_t total_mb = total_bytes >> 20;

        // Leer nuestro propio RSS
        unsigned long rss_kb = get_process_rss_kb(getpid());
        double rss_mb = rss_kb / 1024.0;

        // Imprimir estado
        printf("=========================================\n");
        printf("Bloques asignados       : %zu\n", blocks_count);
        printf("Ultimo bloque           : %p \n", addr);
        printf("Memoria total           : %zu kb\n", total_bytes);
        printf("RSS (proceso)           : %lu kb\n", rss_kb);
        printf("=========================================\n");

        usleep(300000);  // 0.3 seg para salir
    }

    // liberamos todos los bloques
    printf("\nLiberando %zu bloques...\n", blocks_count);
    for (size_t i = 0; i < blocks_count; i++) {
        munmap(blocks[i].addr, blocks[i].size);
    }
    free(blocks);
    printf("Saliendo.\n");
    return 0;
}
