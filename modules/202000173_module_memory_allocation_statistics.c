#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

#ifndef __NR__202000173_memory_allocation_statistics
#define __NR__202000173_memory_allocation_statistics 551
#endif

struct process_mem_stat {
    int pid;                       // PID del proceso
    unsigned long long reserved_kb; 
    unsigned long long commited_kb;
    unsigned char usage_percent;   // (commited / reserved)*100
    unsigned int  oom_score;       // Placeholder (0 si no se usa)
};

struct memory_summary {
    unsigned long long total_reserved_mb;
    unsigned long long total_commited_mb;
};

struct mem_stats_req {
    struct process_mem_stat *stats_buf;
    unsigned int num_procs_max;
    unsigned int num_procs_used;
    int specific_pid;
};

// Función para imprimir una barra de progreso
void printBar(const char *label, double percentage, int color) {
    printf("\033[1;%dm", color); // Cambiar color usando código ANSI
    printf("%-15s | ", label);   // Etiqueta de la barra
    for (int i = 0; i < (int)(percentage / 2); i++) { // Cada '#' representa 2%
        printf("#");
    }
    printf(" %.2f%%", percentage); // Mostrar porcentaje con 2 decimales
    printf("\033[0m\n"); // Restablecer color
}

int main(int argc, char *argv[]) {
    static struct process_mem_stat pstats[4096];
    struct mem_stats_req req;
    int pid_filter = 0;

    if (argc >= 2) {
        pid_filter = atoi(argv[1]);
    }

    memset(&req, 0, sizeof(req));
    req.stats_buf     = pstats;
    req.num_procs_max = 4096;
    req.num_procs_used = 0;
    req.specific_pid  = pid_filter;

    long rc = syscall(__NR__202000173_memory_allocation_statistics, &req);
    if (rc < 0) {
        fprintf(stderr, "Error al invocar la syscall: %s\n", strerror(errno));
        return 1;
    }

    printf("Procesos reportados: %u\n\n", req.num_procs_used);

    printf("\033[1;37m__________________________________________________________________\n");
    printf("\033[1;37m│ \033[1;31m%-6s \033[1;37m│ \033[1;31m%-15s \033[1;37m│ \033[1;31m%-15s \033[1;37m│ \033[1;31m%-8s \033[1;37m│ \033[1;31m%-5s \033[1;37m│\n", 
           "PID", "Reserved (KB)", "Commited (KB)", "Usage (%)", "OOM");
    printf("\033[1;37m__________________________________________________________________\n");

    for (unsigned i = 0; i < req.num_procs_used; i++) {
        struct process_mem_stat *st = &pstats[i];
        printf("\033[1;37m│ \033[1;34m%-6d \033[1;37m│ \033[1;34m%-15llu \033[1;37m│ \033[1;34m%-15llu \033[1;37m│ \033[1;34m%-8u \033[1;37m│ \033[1;34m%-6u \033[1;37m│\n",
               st->pid,
               st->reserved_kb,
               st->commited_kb,
               st->usage_percent,
               st->oom_score);
        printf("\033[1;37m__________________________________________________________________\n");
    }
    printf("\033[0m"); // Restablecer color

    printf("\033[1;31mResumen:\033[0m\n");
    printf("\033[1;37mTotal Reserved = \033[1;35m%llu MB\n", ((struct memory_summary *)&pstats[req.num_procs_used])->total_reserved_mb);
    printf("\033[1;37mTotal Commited = \033[1;32m%llu MB\n", ((struct memory_summary *)&pstats[req.num_procs_used])->total_commited_mb);

    // Calcular porcentaje de memoria commited sobre reserved
    struct memory_summary *sum = 
        (struct memory_summary *)&pstats[req.num_procs_used];
    double total_usage_percent = 
        (sum->total_reserved_mb > 0) ? 
        ((double)sum->total_commited_mb / sum->total_reserved_mb) * 100.0 : 0;

    // Agregar gráfico de barras para totales
    printf("\033[1;37m\nGráfico de barras (en porcentaje):\n");
    printf("----------------------------------------------------------\n");

    // Barra para memoria reservada
    printBar("Total Reserved", 100.0, 35);

    // Barra para memoria commited
    printBar("Total Commited", total_usage_percent, 32);

    return 0;
}
