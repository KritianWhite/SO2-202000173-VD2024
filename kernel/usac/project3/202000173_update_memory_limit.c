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

SYSCALL_DEFINE2(_202000173_update_memory_limit, pid_t, process_pid, size_t, memory_limit) {
    struct memory_limitation *entry;

    // Validar PID y límite de memoria
    if (process_pid <= 0 || memory_limit <= 0) {
        return -EINVAL; // PID o límite inválido
    }

    // Validar permisos (sudoers)
    if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
    }

    // Buscar el proceso en la lista
    list_for_each_entry(entry, &memory_limited_processes, list) {
        if (entry->pid == process_pid) {
            entry->memory_limit = memory_limit; // Actualizar límite
            return 0; // Éxito
        }
    }

    return -ESRCH; // Proceso no encontrado
}
