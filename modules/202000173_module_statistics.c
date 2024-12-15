// SPDX-License-Identifier: GPL
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/statfs.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h>
#include <linux/namei.h>
#include <linux/kernel_stat.h>

// *
// LICENSE GPL Version
// *
MODULE_LICENSE("GPL");
MODULE_AUTHOR("KritianWhite");
MODULE_DESCRIPTION("Módulo para mostrar estadísticas del sistema con un formato organizado");

// Parámetro de módulo que indica la partición sobre la que se obtendrán las estadísticas de disco.
// Por defecto se utiliza la raíz "/".
static char *partition = "/";
module_param(partition, charp, 0644);
MODULE_PARM_DESC(partition, "Partition to report disk usage for");

// Función auxiliar para imprimir estadísticas de tamaño en múltiplos de KB, mostrando su equivalente en MB y GB.
// Recibe el nombre de la métrica (label) y el valor en KB (kb_val).
// Realiza operaciones con desplazamientos de bits para obtener MB y GB.
// Luego imprime la información de forma alineada.
static void print_size_line(struct seq_file *m, const char *label, unsigned long kb_val)
{
    unsigned long mb = kb_val >> 10; // Convierte KB a MB dividiendo por 1024 (2^10)
    unsigned long gb = mb >> 10;     // Convierte MB a GB dividiendo por 1024 (2^10)
    seq_printf(m, "%-20s: %10lu KB (%lu MB (%lu GB))\n", label, kb_val, mb, gb);
}

// Función auxiliar para imprimir líneas asociadas a estadísticas de CPU.
// Recibe el nombre de la métrica (label) y el valor crudo (val), mostrándolos de forma alineada.
static void print_cpu_line(struct seq_file *m, const char *label, u64 val)
{
    seq_printf(m, "%-20s: %llu\n", label, (unsigned long long)val);
}

// Obtiene información de memoria del sistema y la muestra en la secuencia.
// Se utilizan las funciones del kernel para recuperar la información total y libre.
// Los valores se obtienen primero en bytes y luego se convierten a KB.
// Finalmente, se imprime la información utilizando la función auxiliar que formatea la salida.
static void get_memory_info(struct seq_file *m) {
    struct sysinfo i;
    unsigned long mem_total_kb, mem_free_kb;
    unsigned long total_bytes, free_bytes;

    // Obtiene información del sistema: total y libre de RAM
    si_meminfo(&i);

    // Calcula el total y el libre en bytes a partir de la información de sysinfo
    total_bytes = i.totalram * i.mem_unit;
    free_bytes  = i.freeram * i.mem_unit;

    // Convierte los valores a KB dividiendo entre 1024
    mem_total_kb = total_bytes / 1024;
    mem_free_kb  = free_bytes  / 1024;

    // Imprime los resultados formateados
    print_size_line(m, "Memory Total", mem_total_kb);
    print_size_line(m, "Memory Free", mem_free_kb);
}

// Obtiene información del disco de la partición especificada.
// Primero, se resuelve la ruta de la partición usando kern_path.
// Luego, se llama a vfs_statfs para obtener estadísticas del sistema de archivos.
// Los valores retornados se convierten a KB y se muestran con el formato definido.
// Retorna 0 en caso de éxito, o un código de error si falla la obtención de información.
static int get_disk_info(struct seq_file *m) {
    struct path path;
    struct kstatfs stat;
    int err;
    u64 total_bytes, free_bytes;
    unsigned long disk_total_kb, disk_free_kb;

    // Obtiene el path interno de la partición dada
    err = kern_path(partition, LOOKUP_FOLLOW, &path);
    if (err)
        return err;

    // Obtiene las estadísticas del sistema de archivos
    err = vfs_statfs(&path, &stat);
    path_put(&path);
    if (err)
        return err;

    // Calcula el total y el libre en bytes
    total_bytes = (u64)stat.f_blocks * stat.f_bsize;
    free_bytes  = (u64)stat.f_bfree  * stat.f_bsize;

    // Convierte a KB dividiendo entre 1024
    disk_total_kb = (unsigned long)(total_bytes / 1024ULL);
    disk_free_kb  = (unsigned long)(free_bytes  / 1024ULL);

    // Imprime la información de disco formateada
    print_size_line(m, "Disk Total", disk_total_kb);
    print_size_line(m, "Disk Free", disk_free_kb);

    return 0;
}

// Obtiene información acumulada de CPU de todo el sistema.
// Recorre todas las CPUs posibles y acumula las estadísticas de uso (user, nice, system, idle, etc.)
// en un array temporal. Luego las imprime con el formato alineado.
// Cada métrica corresponde a una categoría de tiempo de CPU.
static void get_cpu_info(struct seq_file *m) {
    unsigned int cpu;
    u64 cpu_stats[8] = {0}; 
    // Índices: 0:user, 1:nice, 2:system, 3:idle, 4:iowait, 5:irq, 6:softirq, 7:steal

    // Suma los valores de todas las CPUs
    for_each_possible_cpu(cpu) {
        const struct kernel_cpustat *kcs = &kcpustat_cpu(cpu);
        cpu_stats[0] += kcs->cpustat[CPUTIME_USER];
        cpu_stats[1] += kcs->cpustat[CPUTIME_NICE];
        cpu_stats[2] += kcs->cpustat[CPUTIME_SYSTEM];
        cpu_stats[3] += kcs->cpustat[CPUTIME_IDLE];
        cpu_stats[4] += kcs->cpustat[CPUTIME_IOWAIT];
        cpu_stats[5] += kcs->cpustat[CPUTIME_IRQ];
        cpu_stats[6] += kcs->cpustat[CPUTIME_SOFTIRQ];
        cpu_stats[7] += kcs->cpustat[CPUTIME_STEAL];
    }

    // Imprime las estadísticas de CPU con sus etiquetas descriptivas
    print_cpu_line(m, "CPU User",    cpu_stats[0]);
    print_cpu_line(m, "CPU Nice",    cpu_stats[1]);
    print_cpu_line(m, "CPU System",  cpu_stats[2]);
    print_cpu_line(m, "CPU Idle",    cpu_stats[3]);
    print_cpu_line(m, "CPU IOWait",  cpu_stats[4]);
    print_cpu_line(m, "CPU IRQ",     cpu_stats[5]);
    print_cpu_line(m, "CPU SoftIRQ", cpu_stats[6]);
    print_cpu_line(m, "CPU Steal",   cpu_stats[7]);
}

// Función show para el archivo /proc/202000173_module_statistics.
// Aquí se realiza la secuencia completa de impresiones:
// Se imprimen un encabezado, luego las estadísticas de CPU, memoria y disco,
// y finalmente información sobre la partición monitoreada.
static int system_stats_show(struct seq_file *m, void *v) {
    seq_puts(m, "=========================================\n");
    seq_puts(m, "            System Statistics\n");
    seq_puts(m, "=========================================\n\n");

    seq_puts(m, "[CPU Usage]\n");
    get_cpu_info(m);

    seq_puts(m, "\n[Memory Usage]\n");
    get_memory_info(m);

    seq_puts(m, "\n[Disk Usage]\n");
    if (get_disk_info(m))
        seq_puts(m, "Error reading disk info.\n");

    seq_puts(m, "\n=========================================\n");
    seq_printf(m, "Partition Monitored: %s\n", partition);
    seq_puts(m, "=========================================\n");

    return 0;
}

// Función open para inicializar la lectura del archivo /proc/202000173_module_statistics.
// single_open se encarga de administrar el ciclo de vida del seq_file y el show.
static int system_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, system_stats_show, NULL);
}

// Operaciones de proc para el archivo /proc/202000173_module_statistics.
// Aquí se asignan las funciones open, read, lseek y release para manejar
// la lectura secuencial del contenido.
static const struct proc_ops system_stats_ops = {
    .proc_open    = system_stats_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// Función de inicialización del módulo.
// Crea la entrada /proc/202000173_module_statistics y muestra un mensaje informando su disponibilidad.
static int __init system_stats_init(void) {
    proc_create("202000173_module_statistics", 0, NULL, &system_stats_ops);
    pr_info("202000173_module_statistics module loaded. Read /proc/202000173_module_statistics.\n");
    return 0;
}

// Función de limpieza del módulo.
// Elimina la entrada /proc/202000173_module_statistics y muestra un mensaje de descarte.
static void __exit system_stats_exit(void) {
    remove_proc_entry("202000173_module_statistics", NULL);
    pr_info("202000173_module_statistics module unloaded.\n");
}

// Macros para indicar las funciones de inicio y fin del módulo
module_init(system_stats_init);
module_exit(system_stats_exit);
