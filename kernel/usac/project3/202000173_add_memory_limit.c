#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/capability.h>
#include <linux/mm.h>
#include <linux/list.h>

struct memory_limitation {
    pid_t pid;
    size_t memory_limit;
};

struct memory_limitation_entry {
    pid_t  pid;
    size_t memory_limit;
    struct list_head list; /* Para enlazar con list_head */
};

// Lista global para almacenar los procesos limitados
LIST_HEAD(memory_limited_processes);


static long get_process_memory_usage(struct task_struct *task)
{
    if (!task || !task->mm)
        return 0;

    /* total_vm está en "páginas"; lo convertimos a bytes con PAGE_SHIFT. */
    return (long)(task->mm->total_vm << PAGE_SHIFT);
}

SYSCALL_DEFINE2(_202000173_add_memory_limit, pid_t, process_pid, size_t, memory_limit) {
    struct memory_limitation_entry *entry;
    struct task_struct *task;
    long current_usage;

    // Validar PID y límite de memoria
    if (process_pid <= 0 || memory_limit <= 0) {
        return -EINVAL; // PID o límite inválido
    }

    // Validar permisos (sudoers)
    if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
    }

    // Verificar si el proceso ya existe en la lista
    task = get_pid_task(find_vpid(process_pid), PIDTYPE_PID);
    if (!task) return -ESRCH;

     /* Obtener cuánto está usando de memoria para ver si ya excede el límite */
    current_usage = get_process_memory_usage(task);
    if (current_usage > (long)memory_limit) {
        put_task_struct(task);  /* Liberar referencia a la task_struct */
        return -100;
    }

    /* Revisar si ya está en la lista */
    list_for_each_entry(entry, &memory_limited_processes, list) {
        if (entry->pid == process_pid) {
            put_task_struct(task);
            return -101;
        }
    }

    // Crear nueva entrada
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        put_task_struct(task);
        return -ENOMEM;
    }

    entry->pid = process_pid;
    entry->memory_limit = memory_limit;

    // Agregar a la lista
    list_add(&entry->list, &memory_limited_processes);
    put_task_struct(task);

    return 0; // Éxito
}
