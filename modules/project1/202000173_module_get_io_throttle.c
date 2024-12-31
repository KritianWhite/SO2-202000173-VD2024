#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h>

// *
// LICENSE GPL Version
// *
MODULE_LICENSE("GPL");
MODULE_AUTHOR("KritianWhite");
MODULE_DESCRIPTION("Módulo para mostrar I/O stats de un PID en /proc/202000173_module_get_io_throttle");

static int pid = 0;
module_param(pid, int, 0644);
MODULE_PARM_DESC(pid, "PID del proceso a monitorear");

struct io_stats_user {
    u64 rchar;
    u64 wchar;
    u64 syscr;
    u64 syscw;
    u64 read_bytes;
    u64 write_bytes;
};

// Función auxiliar para imprimir valores en bytes, MB y GB
static void print_io_bytes_line(struct seq_file *m, const char *label, u64 bytes_val) {
    u64 mb = bytes_val >> 20; // divide entre 2^20 = 1048576
    u64 gb = bytes_val >> 30; // divide entre 2^30 = 1073741824

    // Mostramos valores enteros
    // Ejemplo: rchar: 102400 bytes (100 MB (0 GB))
    seq_printf(m, "%-12s: %llu bytes (%llu MB (%llu GB))\n", label, bytes_val, mb, gb);
}

// Función auxiliar para imprimir valores enteros (ej. syscr, syscw)
static void print_io_count_line(struct seq_file *m, const char *label, u64 count) {
    seq_printf(m, "%-12s: %llu\n", label, count);
}

static int io_throttle_show(struct seq_file *m, void *v) {
    struct task_struct *task;
    struct io_stats_user stats;

    seq_puts(m, "=========================================\n");
    seq_puts(m, "          I/O Statistics Monitor\n");
    seq_puts(m, "=========================================\n\n");

    if (pid == 0) {
        seq_puts(m, "No se ha especificado un PID. Use insmod con pid=[PID].\n");
        seq_puts(m, "Ejemplo: insmod 202000173_module_get_io_throttle.ko pid=0001\n");
        return 0;
    }

    rcu_read_lock();
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        rcu_read_unlock();
        seq_printf(m, "El proceso con PID %d no existe.\n", pid);
        return 0;
    }

    stats.rchar       = task->ioac.rchar;
    stats.wchar       = task->ioac.wchar;
    stats.syscr       = task->ioac.syscr;
    stats.syscw       = task->ioac.syscw;
    stats.read_bytes  = task->ioac.read_bytes;
    stats.write_bytes = task->ioac.write_bytes;
    rcu_read_unlock();

    seq_printf(m, "I/O Stats para PID %d:\n\n", pid);

    // Mostrar las métricas
    print_io_bytes_line(m, "rchar",       stats.rchar);
    print_io_bytes_line(m, "wchar",       stats.wchar);
    print_io_count_line(m, "syscr",       stats.syscr);
    print_io_count_line(m, "syscw",       stats.syscw);
    print_io_bytes_line(m, "read_bytes",  stats.read_bytes);
    print_io_bytes_line(m, "write_bytes", stats.write_bytes);

    seq_puts(m, "\n=========================================\n");

    return 0;
}

static int io_throttle_open(struct inode *inode, struct file *file) {
    return single_open(file, io_throttle_show, NULL);
}

static const struct proc_ops io_throttle_ops = {
    .proc_open    = io_throttle_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init io_throttle_init(void) {
    proc_create("202000173_module_get_io_throttle", 0, NULL, &io_throttle_ops);
    pr_info("202000173_module_get_io_throttle: Módulo cargado. Use 'cat /proc/202000173_module_get_io_throttle' con pid definido.\n");
    return 0;
}

static void __exit io_throttle_exit(void) {
    remove_proc_entry("202000173_module_get_io_throttle", NULL);
    pr_info("202000173_module_get_io_throttle: Descargando modulo.....\n");
}

module_init(io_throttle_init);
module_exit(io_throttle_exit);
