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

SYSCALL_DEFINE3(_202000173_get_memory_limits, struct memory_limitation __user *, user_buffer, size_t, max_entries, int __user *, processes_returned) {
    struct memory_limitation *entry;
    struct memory_limitation *buffer;
    size_t count = 0;

    // Validar acceso al buffer del usuario
    if (!access_ok(user_buffer, max_entries * sizeof(struct memory_limitation))) {
        return -EFAULT; // Error de acceso
    }

    // Asignar memoria temporal para el buffer en el kernel
    buffer = kmalloc_array(max_entries, sizeof(struct memory_limitation), GFP_KERNEL);
    if (!buffer) {
        return -ENOMEM;
    }

    // Copiar datos de la lista al buffer
    list_for_each_entry(entry, &memory_limited_processes, list) {
        if (count >= max_entries) break;
        buffer[count].pid = entry->pid;
        buffer[count].memory_limit = entry->memory_limit;
        count++;
    }

    // Copiar el buffer al espacio de usuario
    if (copy_to_user(user_buffer, buffer, count * sizeof(struct memory_limitation))) {
        kfree(buffer);
        return -EFAULT;
    }

    // Copiar el número de procesos al espacio de usuario
    if (copy_to_user(processes_returned, &count, sizeof(int))) {
        kfree(buffer);
        return -EFAULT;
    }

    kfree(buffer); // Liberar memoria temporal
    return 0; // Éxito
}
