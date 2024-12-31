#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/vmstat.h>
#include <linux/uaccess.h>
#include <linux/string.h>

// *
// LICENSE GPL Version
// *
MODULE_LICENSE("GPL");
MODULE_AUTHOR("KritianWHite");
MODULE_DESCRIPTION("Módulo para mostrar snapshot de memoria en /proc/202000173_module_capture_memory_snapshot");

// Función auxiliar para convertir páginas a MB y GB y mostrarlas formateadas
static void print_memory_line(struct seq_file *m, const char *label, unsigned long pages) {
    unsigned long long bytes = (unsigned long long)pages * (unsigned long long)PAGE_SIZE;
    unsigned long long mb = bytes >> 20; // Dividir por 2^20
    unsigned long long gb = bytes >> 30; // Dividir por 2^30

    // Ajustamos el formato para que se vea bonito XD
    // Por ejemplo: "Total RAM        : 10240 MB (10 GB)"
    // %-18s deja el label en un ancho fijo de 18 chars para alineación
    seq_printf(m, "%-18s: %10llu MB (%llu GB)\n", label, mb, gb);
}

// Función para llenar la información de memoria
static void fill_mem_snapshot(struct seq_file *m) {
    struct sysinfo i;
    si_meminfo(&i);

    unsigned long total_ram    = i.totalram;
    unsigned long free_ram     = i.freeram;
    unsigned long cache_ram    = global_node_page_state(NR_FILE_PAGES) - global_node_page_state(NR_SHMEM) - i.bufferram;
    unsigned long buffer_ram   = i.bufferram;
    unsigned long active_ram   = global_node_page_state(NR_ACTIVE_FILE) + global_node_page_state(NR_ACTIVE_ANON);
    unsigned long inactive_ram = global_node_page_state(NR_INACTIVE_FILE) + global_node_page_state(NR_INACTIVE_ANON);

    // Imprime cada valor con la función auxiliar
    print_memory_line(m, "Total RAM", total_ram);
    print_memory_line(m, "Free RAM", free_ram);
    print_memory_line(m, "Cached RAM", cache_ram);
    print_memory_line(m, "Buffered RAM", buffer_ram);
    print_memory_line(m, "Active RAM", active_ram);
    print_memory_line(m, "Inactive RAM", inactive_ram);
}

// Función show que se llama cuando hacemos "cat /proc/202000173_module_capture_memory_snapshot"
static int capture_mem_show(struct seq_file *m, void *v) {
    seq_puts(m, "=========================================\n");
    seq_puts(m, "          Memory Snapshot Info\n");
    seq_puts(m, "=========================================\n\n");

    fill_mem_snapshot(m);

    seq_puts(m, "\n=========================================\n");
    return 0;
}

// Función open para seq_file
static int capture_mem_open(struct inode *inode, struct file *file) {
    return single_open(file, capture_mem_show, NULL);
}

static const struct proc_ops capture_mem_ops = {
    .proc_open    = capture_mem_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init capture_mem_init(void) {
    proc_create("202000173_module_capture_memory_snapshot", 0, NULL, &capture_mem_ops);
    pr_info("202000173_module_capture_memory_snapshot: Módulo cargado. Lee /proc/202000173_module_capture_memory_snapshot\n");
    return 0;
}

static void __exit capture_mem_exit(void) {
    remove_proc_entry("202000173_module_capture_memory_snapshot", NULL);
    pr_info("202000173_module_capture_memory_snapshot: Módulo descargado.\n");
}

module_init(capture_mem_init);
module_exit(capture_mem_exit);
