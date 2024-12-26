#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <ctype.h>

#ifndef __NR__202000173_memory_allocation_statistics
#define __NR__202000173_memory_allocation_statistics 551
#endif

/*
 * Estructura utilizada para recopilar información sobre el uso de memoria
 * de procesos individuales mediante la syscall _202000173_memory_allocation_statistics.
 *
 * Campos:
 *   - vm_kb: Memoria virtual total en KB.
 *   - rss_kb: Memoria física (Resident Set Size) en KB.
 *   - rss_percent_of_vm: Porcentaje de memoria física sobre la virtual.
 *   - oom_adjustment: Ajuste del proceso respecto al OOM killer.
 */
struct tamalloc_proc_info {
    unsigned long vm_kb;
    unsigned long rss_kb;
    unsigned int  rss_percent_of_vm;
    int           oom_adjustment;
};

/*
 * Llama a la syscall _202000173_memory_allocation_statistics para un PID dado.
 *
 * Argumentos:
 *   - pid: Identificador del proceso (PID).
 *   - info: Puntero a una estructura tamalloc_proc_info para almacenar los resultados.
 *
 * Retorno:
 *   - 0 si la operación fue exitosa.
 *   - Códigos de error negativos en caso de fallo.
 */
static inline long tamalloc_get_indiviual_stats(pid_t pid, struct tamalloc_proc_info *info)
{
    return syscall(__NR__202000173_memory_allocation_statistics, pid, info);
}

/*
 * Imprime una fila de datos formateada.
 *
 * Argumentos:
 *   - pid: Identificador del proceso.
 *   - pinfo: Puntero a una estructura tamalloc_proc_info con los datos del proceso.
 */
static void body_table(pid_t pid, const struct tamalloc_proc_info *pinfo)
{
    printf("\033[1;37m│ \033[1;34m%-6d \033[1;37m│ \033[1;34m%-15lu \033[1;37m│ \033[1;34m%-15lu \033[1;37m│ \033[1;34m%-8u \033[1;37m│ \033[1;34m%-6d \033[1;37m│\n",
           pid,
           pinfo->vm_kb,
           pinfo->rss_kb,
           pinfo->rss_percent_of_vm,
           pinfo->oom_adjustment);
    printf("\033[1;37m__________________________________________________________________\n");
}

/*
 * Imprime el encabezado de la tabla.
 */
static void header_table(void)
{
    printf("\033[1;37m__________________________________________________________________\n");
    printf("\033[1;37m│ \033[1;31m%-6s \033[1;37m│ \033[1;31m%-15s \033[1;37m│ \033[1;31m%-15s \033[1;37m│ \033[1;31m%-8s \033[1;37m│ \033[1;31m%-5s \033[1;37m│\n", 
           "PID", "Reserved (KB)", "Commited (KB)", "Usage (%)", "OOM");
    printf("\033[1;37m__________________________________________________________________\n");
}

/*
 * Programa principal que utiliza la syscall _202000173_memory_allocation_statistics
 * para mostrar información de memoria de procesos.
 *
 * Argumentos:
 *   - argc: Número de argumentos.
 *   - argv: Argumentos en línea de comandos.
 *
 * Uso:
 *   - Sin argumentos: Itera sobre todos los procesos y muestra sus datos.
 *   - Con PID: Muestra los datos de un proceso específico.
 */
int main(int argc, char *argv[])
{
    pid_t pid = 0;

    // Si se pasa un argumento, lo interpretamos como PID.
    if (argc >= 2) {
        pid = (pid_t)atoi(argv[1]);
    }

    if (pid != 0) {
        /*
         * Si se proporcionó un PID, intentamos obtener las estadísticas
         * del proceso correspondiente y las mostramos.
         */
        struct tamalloc_proc_info pinfo;
        long ret = tamalloc_get_indiviual_stats(pid, &pinfo);
        if (ret < 0) {
            perror("tamalloc_get_indiviual_stats syscall");
            return 1;
        }

        header_table();
        body_table(pid, &pinfo);
    }
    else {
        /*
         * Si no se proporciona un PID, iteramos sobre todos los procesos
         * en el directorio /proc y mostramos sus datos si son válidos.
         */
        DIR *dp;
        struct dirent *entry;

        dp = opendir("/proc");
        if (!dp) {
            perror("No se pudo abrir /proc");
            return 1;
        }

        header_table();

        while ((entry = readdir(dp)) != NULL) {
            /*
             * Verificamos si el directorio es numérico, lo que indica
             * que corresponde a un PID.
             */
            if (entry->d_type == DT_DIR) {
                const char *dname = entry->d_name;
                int is_numeric = 1;
                for (int i = 0; dname[i] != '\0'; i++) {
                    if (!isdigit((unsigned char)dname[i])) {
                        is_numeric = 0;
                        break;
                    }
                }
                if (is_numeric) {
                    pid_t current_pid = (pid_t)atoi(dname);
                    if (current_pid > 0) {
                        struct tamalloc_proc_info pinfo;
                        long ret = tamalloc_get_indiviual_stats(current_pid, &pinfo);
                        if (ret == 0) {
                            body_table(current_pid, &pinfo);
                        }
                    }
                }
            }
        }
        closedir(dp);

    }

    return 0;
}
