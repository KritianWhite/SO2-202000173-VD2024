#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/types.h>

/**
 * Estructura para pasar información de I/O al espacio de usuario.
 * Representa estadísticas I/O del proceso.
 */
struct io_stats_user {
    unsigned long long rchar;        // Cantidad total de caracteres leídos
    unsigned long long wchar;        // Cantidad total de caracteres escritos
    unsigned long long syscr;        // Cantidad de lecturas vía syscall
    unsigned long long syscw;        // Cantidad de escrituras vía syscall
    unsigned long long read_bytes;   // Bytes leídos del almacenamiento
    unsigned long long write_bytes;  // Bytes escritos al almacenamiento
};

/**
 * Función auxiliar para obtener las estadísticas I/O de un proceso dado su PID.
 * Retorna 0 en caso de éxito, o un código de error apropiado.
 */
static int get_io_stats_for_pid(int pid, struct io_stats_user *stats)
{
    struct task_struct *task;

    // Bloqueo de lectura RCU para acceder a estructuras del kernel de forma segura
    rcu_read_lock();
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        rcu_read_unlock();
        return -ESRCH; // No se encontró el proceso, retorna error
    }

    // Copiamos las estadísticas I/O del proceso
    stats->rchar       = task->ioac.rchar;
    stats->wchar       = task->ioac.wchar;
    stats->syscr       = task->ioac.syscr;
    stats->syscw       = task->ioac.syscw;
    stats->read_bytes  = task->ioac.read_bytes;
    stats->write_bytes = task->ioac.write_bytes;

    rcu_read_unlock();

    return 0;
}

SYSCALL_DEFINE2(_202000173_get_io_throttle, int, pid, struct io_stats_user __user *, user_stats)
{
    struct io_stats_user kernel_stats;
    int ret;

    // Obtener las estadísticas en el espacio de kernel
    ret = get_io_stats_for_pid(pid, &kernel_stats);
    if (ret < 0)
        return ret; // Retornar el error adecuado (p.ej: -ESRCH)

    // Copiar los datos obtenidos hacia el espacio de usuario
    if (copy_to_user(user_stats, &kernel_stats, sizeof(kernel_stats)))
        return -EFAULT;

    return 0;
}
