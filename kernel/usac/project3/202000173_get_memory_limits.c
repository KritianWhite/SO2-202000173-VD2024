#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/list.h>  

struct memory_limitation {
    pid_t  pid;
    size_t memory_limit;
};

/* Estructura interna (misma definición que en 202000173_add_memory_limit.c) */
struct memory_limitation_entry {
    pid_t  pid;
    size_t memory_limit;
    struct list_head list;
};

// Lista global para almacenar los procesos limitados
extern struct list_head memory_limited_processes;

SYSCALL_DEFINE3(_202000173_get_memory_limits, struct memory_limitation __user *, u_processes_buffer, size_t, max_entries, int __user *, processes_returned)
{
    struct memory_limitation_entry *entry;
    size_t count = 0;

    /* Validar max_entries */
    if (max_entries <= 0) return -EINVAL;

    /* Validar punteros */
    if (!u_processes_buffer || !processes_returned) return -EINVAL;

    /* Recorrer la lista y copiar hasta max_entries */
    list_for_each_entry(entry, &memory_limited_processes, list) {
        struct memory_limitation tmp;

        if (count >= max_entries)
            break;

        /* Copiamos los datos a una struct temporal */
        tmp.pid = entry->pid;
        tmp.memory_limit = entry->memory_limit;

        /* copy_to_user() retorna bytes que NO se copiaron => != 0 indica error */
        if (copy_to_user(&u_processes_buffer[count], &tmp, sizeof(tmp)) != 0) {
            return -EFAULT;
        }
        count++;
    }

   /* Guardar la cantidad de procesos copiados en processes_returned */
    if (copy_to_user(processes_returned, &count, sizeof(count)) != 0) {
        return -EFAULT;
    }

    // kfree(buffer); // Liberar memoria temporal
    return count; // Éxito
}
