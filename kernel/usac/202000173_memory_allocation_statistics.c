#include <linux/syscalls.h>   // Para SYSCALL_DEFINE
#include <linux/sched.h>      // for_each_process, task_struct
#include <linux/mm.h>         // mm->total_vm, get_mm_rss
#include <linux/mm_types.h>
#include <linux/uaccess.h>    // copy_to_user, copy_from_user
#include <linux/rcupdate.h>   // rcu_read_lock, for_each_process
#include <asm/page.h>         // PAGE_SIZE

/*
 * Estructuras de datos para la syscall
 */

// Info por proceso
struct process_mem_stat {
    pid_t pid;
    __u64 reserved_kb;   // Memoria reservada en KB
    __u64 commited_kb;   // Memoria utilizada en KB
    __u8  usage_percent; // (commited_kb / reserved_kb)*100 (si reserved_kb>0)
    __u32 oom_score;     // Placeholder o valor calculado
};

// Resumen global
struct memory_summary {
    __u64 total_reserved_mb;  // Suma total reservada (MB)
    __u64 total_commited_mb;  // Suma total utilizada (MB)
};

// Parámetros de la syscall
struct mem_stats_req {
    /*
     * Buffer en user space donde escribiremos:
     *   - Primero: 'num_procs_used' elementos de struct process_mem_stat
     *   - Luego:   1 elemento de struct memory_summary
     */
    struct process_mem_stat __user *stats_buf;

    // Tamaño máximo en cantidad de 'struct process_mem_stat' que el
    // usuario reservó en su buffer. Si hay más procesos, solo se devuelven
    // 'num_procs_max' y se truncan los restantes.
    __u32  num_procs_max;

    // Devuelto por la syscall: cuántos procesos realmente se escribieron
    // en el buffer (puede ser <= num_procs_max).
    __u32  num_procs_used;

    // PID específico (0 => mostrar todos)
    pid_t  specific_pid;
};

/*
 * Syscall: _202000173_memory_allocation_statistics
 */
SYSCALL_DEFINE1(_202000173_memory_allocation_statistics,
                struct mem_stats_req __user *, arg)
{
    struct mem_stats_req karg;           // Copia local de los parámetros
    struct process_mem_stat st;          // Estructura temporal para cada proceso
    struct memory_summary summary = {0}; // Acumulador de totales
    unsigned int count = 0;             // Cuántos procesos guardamos
    struct task_struct *p;
    int ret = 0;

    // Validar puntero 'arg'
    if (!arg)
        return -EINVAL;

    // Copiar desde user-space a kernel-space
    if (copy_from_user(&karg, arg, sizeof(karg)))
        return -EFAULT;

    // Validar parámetros
    if (!karg.stats_buf || karg.num_procs_max == 0)
        return -EINVAL;

    // Recorremos la lista de procesos
    rcu_read_lock();

    if (karg.specific_pid != 0) {
        /*
         * Caso 1: El usuario pidió un PID específico.
         */
        p = pid_task(find_vpid(karg.specific_pid), PIDTYPE_PID);
        if (p) {
            struct mm_struct *mm = get_task_mm(p);
            if (mm) {
                // Calcular "reserved" y "commited"
                unsigned long reserved_pages = mm->total_vm;   // virtual pages
                unsigned long rss_pages      = get_mm_rss(mm); // resident pages
                mmput(mm);

                // Pasar a KB
                __u64 reserved_kb = ((unsigned long long)reserved_pages * PAGE_SIZE) >> 10;
                __u64 commited_kb = ((unsigned long long)rss_pages      * PAGE_SIZE) >> 10;
                __u8  usage_pct   = 0;

                if (reserved_kb > 0) {
                    if (commited_kb <= reserved_kb) {
                        usage_pct = (commited_kb * 100) / reserved_kb;
                    } else {
                        usage_pct = 100;
                    }
                }

                // Llenar la estructura
                st.pid           = p->pid;
                st.reserved_kb   = reserved_kb;
                st.commited_kb   = commited_kb;
                st.usage_percent = usage_pct;
                st.oom_score     = 0; // placeholder

                // Acumular al summary (en MB: reserved_kb >> 10 => MB)
                summary.total_reserved_mb += (reserved_kb >> 10);
                summary.total_commited_mb += (commited_kb >> 10);

                // Copiar a user-space
                if (copy_to_user(&karg.stats_buf[0], &st, sizeof(st))) {
                    ret = -EFAULT;
                } else {
                    count = 1;
                }
            }
        }
    } else {
        /*
         * Caso 2: El usuario pidió TODOS los procesos (specific_pid == 0).
         */
        for_each_process(p) {
            struct mm_struct *mm = get_task_mm(p);
            if (!mm)
                continue;

            // Calcular pages
            unsigned long reserved_pages = mm->total_vm;
            unsigned long rss_pages      = get_mm_rss(mm);
            mmput(mm);

            // Pasar a KB
            __u64 reserved_kb = ((unsigned long long)reserved_pages * PAGE_SIZE) >> 10;
            __u64 commited_kb = ((unsigned long long)rss_pages      * PAGE_SIZE) >> 10;
            __u8  usage_pct   = 0;

            if (reserved_kb > 0) {
                if (commited_kb <= reserved_kb) {
                    usage_pct = (commited_kb * 100) / reserved_kb;
                } else {
                    usage_pct = 100;
                }
            }

            // Llenar 'st'
            st.pid           = p->pid;
            st.reserved_kb   = reserved_kb;
            st.commited_kb   = commited_kb;
            st.usage_percent = usage_pct;
            st.oom_score     = 0; // placeholder

            summary.total_reserved_mb += (reserved_kb >> 10);
            summary.total_commited_mb += (commited_kb >> 10);

            // Copiar al buffer si hay espacio
            if (count < karg.num_procs_max) {
                if (copy_to_user(&karg.stats_buf[count], &st, sizeof(st))) {
                    ret = -EFAULT;
                    break;
                }
                count++;
            } else {
                // Ya no cabe más
                break;
            }
        }
    }

    rcu_read_unlock();

    // Guardar la cantidad real de procesos escritos
    karg.num_procs_used = count;

    // Copiar el summary al final del array
    if (ret == 0) {
        struct memory_summary __user *summary_ptr;
        // La posición en el array es 'count' (después de los count elementos)
        summary_ptr = (struct memory_summary __user *)&karg.stats_buf[count];

        if (copy_to_user(summary_ptr, &summary, sizeof(summary))) {
            ret = -EFAULT;
        }
    }

    // Copiar la estructura karg actualizada (num_procs_used) de vuelta
    if (ret == 0) {
        if (copy_to_user(arg, &karg, sizeof(karg))) {
            ret = -EFAULT;
        }
    }

    return ret;
}
