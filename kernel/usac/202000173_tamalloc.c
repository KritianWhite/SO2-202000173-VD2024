#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h> // Para asignar memoria virtual

struct memory_info {
    unsigned long vmSize;     // Tamaño total de la memoria virtual (bytes)
    unsigned long vmRSS;      // Tamaño de la memoria residente (bytes)
    unsigned long mem_addr;   // Dirección virtual de la memoria asignada
};

SYSCALL_DEFINE2(_202000173_tamalloc, pid_t, pid, struct memory_info __user *, user_mem_info)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct memory_info mem_info;
    unsigned long addr;
    unsigned long size = 1024 * 1024; // PAGE_SIZE * 10; // Por ejemplo, 10 páginas = 40 KB

    // Buscar el proceso por PID
    rcu_read_lock();
    task = find_task_by_vpid(pid);
    if (!task) {
        rcu_read_unlock();
        return -ESRCH; // No se encontró el proceso
    }

    // Obtener la estructura mm del proceso
    mm = get_task_mm(task);
    rcu_read_unlock();

    if (!mm)
        return -EINVAL; // No hay mm_struct (probablemente kernel thread)

    // Obtener vmSize y vmRSS
    mem_info.vmSize = mm->total_vm << PAGE_SHIFT; // Convertir páginas a bytes
    mem_info.vmRSS = get_mm_rss(mm) << PAGE_SHIFT; // Convertir páginas a bytes

    // Asignar memoria virtual con vmalloc
    addr = (unsigned long)vmalloc(size);
    if (!addr)
        return -ENOMEM; // Error al asignar memoria

    memset((void *)addr, 0, size); // Inicializar en 0 (lazy-zeroing garantizado en el primer acceso)

    mem_info.mem_addr = addr; // Dirección virtual asignada

    mmput(mm);

    // Copiar la información al espacio de usuario
    if (copy_to_user(user_mem_info, &mem_info, sizeof(mem_info)))
        return -EFAULT;

    return 0; // Éxito
}
