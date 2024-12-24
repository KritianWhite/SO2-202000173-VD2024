#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mman.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KritianWhite");
MODULE_DESCRIPTION("Módulo para mostrar información de tamalloc en /proc/202000173_tamalloc");

// Variables globales para almacenar información de tamalloc
static void *tamalloc_address = NULL;
static size_t tamalloc_size = 0;

// Función auxiliar para escribir la información en el archivo /proc
static int tamalloc_proc_show(struct seq_file *m, void *v) {
    seq_puts(m, "=========================================\n");
    seq_puts(m, "          Información de Tamalloc\n");
    seq_puts(m, "=========================================\n\n");

    if (tamalloc_address) {
        seq_printf(m, "Dirección de memoria asignada: %p\n", tamalloc_address);
        seq_printf(m, "Tamaño de memoria asignada   : %zu bytes\n", tamalloc_size);
    } else {
        seq_puts(m, "No se ha asignado memoria con Tamalloc.\n");
    }

    seq_puts(m, "\n=========================================\n");
    return 0;
}

// Función open para seq_file
static int tamalloc_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, tamalloc_proc_show, NULL);
}

// Operaciones para /proc
static const struct proc_ops tamalloc_proc_ops = {
    .proc_open    = tamalloc_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// Función init: Llama a tamalloc y guarda los resultados en /proc
static int __init tamalloc_proc_init(void) {
    // Asignar memoria con tamalloc
    tamalloc_size = 1024 * 1024; // 1 MB
    tamalloc_address = (void *)vm_mmap(NULL, 0, tamalloc_size, PROT_READ | PROT_WRITE,
                                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, 0);

    if (IS_ERR(tamalloc_address)) {
        pr_err("Error al asignar memoria con Tamalloc: %ld\n", PTR_ERR(tamalloc_address));
        tamalloc_address = NULL;
    } else {
        pr_info("Memoria asignada con Tamalloc en: %p\n", tamalloc_address);
    }

    // Crear la entrada en /proc
    if (!proc_create("202000173_tamalloc", 0, NULL, &tamalloc_proc_ops)) {
        pr_err("Error al crear la entrada en /proc/202000173_tamalloc\n");
        return -ENOMEM;
    }

    pr_info("/proc/202000173_tamalloc creado. Consulta la información de Tamalloc.\n");
    return 0;
}

// Función exit: Elimina la entrada de /proc y libera memoria
static void __exit tamalloc_proc_exit(void) {
    if (tamalloc_address) {
        vm_munmap((unsigned long)tamalloc_address, tamalloc_size);
        pr_info("Memoria de Tamalloc liberada en: %p\n", tamalloc_address);
    }

    remove_proc_entry("202000173_tamalloc", NULL);
    pr_info("/proc/202000173_tamalloc eliminado.\n");
}

module_init(tamalloc_proc_init);
module_exit(tamalloc_proc_exit);
