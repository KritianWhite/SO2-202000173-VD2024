#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <asm/page.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KritianWhite");
MODULE_DESCRIPTION("Módulo que llama a _202000173_tamalloc y muestra info en /proc/202000173_module_tamalloc");

// ----------------------------------------------------------------------
// 1) Declaración 'extern' de la syscall en el kernel (¡requiere exportarla!)
// ----------------------------------------------------------------------
extern long sys_202000173_tamalloc(size_t size);

// ----------------------------------------------------------------------
// 2) Variables globales para almacenar la dirección y tamaño de Tamalloc
// ----------------------------------------------------------------------
static void  *tamalloc_address = NULL;
static size_t tamalloc_size    = 0;

// ----------------------------------------------------------------------
// 3) Función que se invoca al hacer "cat /proc/202000173_module_tamalloc"
// ----------------------------------------------------------------------
static int tamalloc_proc_show(struct seq_file *m, void *v)
{
    seq_puts(m, "=========================================\n");
    seq_puts(m, "          Información de Tamalloc\n");
    seq_puts(m, "=========================================\n\n");

    if (tamalloc_address) {
        seq_printf(m, "Dirección de memoria (User-Space): %p\n", tamalloc_address);
        seq_printf(m, "Tamaño de memoria asignada       : %zu bytes\n", tamalloc_size);
    } else {
        seq_puts(m, "No se ha asignado memoria con Tamalloc.\n");
    }

    seq_puts(m, "\n=========================================\n");
    return 0;
}

// Función open para seq_file
static int tamalloc_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, tamalloc_proc_show, NULL);
}

// Operaciones para /proc
static const struct proc_ops tamalloc_proc_ops = {
    .proc_open    = tamalloc_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// ----------------------------------------------------------------------
// 4) init_module: Crea /proc/202000173_module_tamalloc y llama a tamalloc
// ----------------------------------------------------------------------
static int __init tamalloc_module_init(void)
{
    long ret;

    pr_info("202000173_module_tamalloc: cargando módulo...\n");

    // Asignamos, por ejemplo, 1 MB de memoria con Tamalloc
    tamalloc_size = 1024 * 1024;
    pr_info("Solicitando Tamalloc de %zu bytes...\n", tamalloc_size);

    // Llamada directa a la syscall _202000173_tamalloc
    ret = sys_202000173_tamalloc(tamalloc_size);
    if (ret < 0) {
        pr_err("Error al asignar memoria con Tamalloc: %ld\n", ret);
        tamalloc_address = NULL;
    } else {
        tamalloc_address = (void *)ret;  // Dirección de espacio de usuario
        pr_info("Tamalloc: memoria asignada en %p\n", tamalloc_address);
    }

    // Crear la entrada /proc/202000173_module_tamalloc
    if (!proc_create("202000173_module_tamalloc", 0, NULL, &tamalloc_proc_ops)) {
        pr_err("Error al crear /proc/202000173_module_tamalloc\n");
        // Si falla, liberamos la memoria
        if (tamalloc_address)
            vm_munmap((unsigned long)tamalloc_address, tamalloc_size);
        return -ENOMEM;
    }

    pr_info("/proc/202000173_module_tamalloc creado.\n");
    return 0;
}

// ----------------------------------------------------------------------
// 5) cleanup_module: Elimina la entrada de /proc y libera la memoria
// ----------------------------------------------------------------------
static void __exit tamalloc_module_exit(void)
{
    pr_info("202000173_module_tamalloc: descargando módulo...\n");

    // Liberamos la región de usuario si fue asignada
    if (tamalloc_address) {
        vm_munmap((unsigned long)tamalloc_address, tamalloc_size);
        pr_info("Tamalloc: se liberó la memoria en %p\n", tamalloc_address);
    }

    remove_proc_entry("202000173_module_tamalloc", NULL);
    pr_info("/proc/202000173_module_tamalloc eliminado.\n");
}

module_init(tamalloc_module_init);
module_exit(tamalloc_module_exit);
