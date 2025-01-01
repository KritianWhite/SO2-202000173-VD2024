#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>

struct memory_limitation {
    pid_t pid;
    size_t memory_limit;
    struct list_head list;  // Lista enlazada para manejar múltiples procesos
};

// Lista global para almacenar los procesos limitados
static LIST_HEAD(memory_limited_processes);

SYSCALL_DEFINE1(_202000173_remove_memory_limit, pid_t, process_pid) {
    struct memory_limitation *entry, *tmp;

    // Validar PID
    if (process_pid <= 0) {
        return -EINVAL; // PID inválido
    }

    // Validar permisos (sudoers)
    if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
    }

    // Buscar y eliminar el proceso de la lista
    list_for_each_entry_safe(entry, tmp, &memory_limited_processes, list) {
        if (entry->pid == process_pid) {
            list_del(&entry->list);
            kfree(entry);
            return 0; // Éxito
        }
    }

    return -ESRCH; // Proceso no encontrado
}
