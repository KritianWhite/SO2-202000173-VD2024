#include <linux/kernel.h>
#include <linux/syscalls.h>   // Para SYSCALL_DEFINE, definir nuevas syscalls
#include <linux/sched.h>      // for_each_process, task_struct, manejo de procesos
#include <linux/mm.h>         // mm->total_vm, get_mm_rss, información de memoria
#include <linux/mm_types.h>   // Tipos asociados con la memoria
#include <linux/uaccess.h>    // copy_to_user, copy_from_user, para interacción con espacio de usuario
#include <linux/rcupdate.h>   // rcu_read_lock, sincronización para estructuras protegidas por RCU
#include <linux/pid.h>        // find_vpid, pid_task, búsqueda de procesos por PID

/*
 * Estructuras de datos para la syscall
 *
 * Esta estructura se utiliza para proporcionar información sobre el proceso
 * que el usuario especifica mediante el PID. Contiene:
 *   - vm_kb: Memoria virtual total en KB
 *   - rss_kb: Resident Set Size en KB (memoria física utilizada)
 *   - rss_percent_of_vm: Porcentaje de RSS sobre la memoria virtual
 *   - oom_adjustment: Ajuste de prioridad respecto al OOM killer
 */
struct tamalloc_proc_info {
	unsigned long vm_kb;                // Memoria virtual en KB
	unsigned long rss_kb;               // Resident Set Size en KB (memoria física utilizada)
	unsigned int rss_percent_of_vm;     // Porcentaje de RSS sobre la memoria virtual
	int oom_adjustment;                 // Ajuste del proceso respecto al OOM killer
};

/*
 * Syscall: _202000173_memory_allocation_statistics
 *
 * Esta syscall proporciona estadísticas de memoria para un proceso dado por su PID.
 * Argumentos:
 *   - pid: Identificador de proceso (PID) del cual se quieren obtener las estadísticas
 *   - info: Puntero a una estructura en el espacio de usuario donde se almacenarán
 *           las estadísticas recopiladas
 */
SYSCALL_DEFINE2(_202000173_memory_allocation_statistics, pid_t, pid,  struct tamalloc_proc_info __user *, info)
{
	/*
	 * task_struct representa el proceso dentro del kernel.
	 * Se utiliza para acceder a toda la información relacionada con el proceso.
	 */
	struct task_struct *task;                      

	/*
	 * mm_struct almacena toda la información de memoria de un proceso.
	 * Contiene detalles sobre la memoria virtual y física asignada al proceso.
	 */
	struct mm_struct *mm;                          

	/*
	 * Estructura temporal en el espacio del kernel donde se recopilarán
	 * las estadísticas de memoria del proceso antes de copiarlas al
	 * espacio de usuario.
	 */
	struct tamalloc_proc_info kinfo;               

	/*
	 * Bloqueamos el acceso concurrente a las estructuras protegidas por
	 * RCU (Read-Copy-Update), lo que permite acceder de manera segura a
	 * datos compartidos como las estructuras de procesos.
	 */
	rcu_read_lock();                               

	/*
	 * Buscamos el proceso utilizando su PID.
	 * Si no se encuentra el proceso, devolvemos un error indicando que
	 * el proceso no existe (ESRCH).
	 */
	task = pid_task(find_vpid(pid), PIDTYPE_PID); 
	if (!task) {
		rcu_read_unlock();
		return -ESRCH;
	}

	/*
	 * Obtenemos la estructura mm_struct del proceso.
	 * Si no tiene memoria asociada (por ejemplo, en el caso de hilos del kernel),
	 * devolvemos un error indicando parámetros inválidos (EINVAL).
	 */
	mm = get_task_mm(task);                       
	if (!mm) {
		rcu_read_unlock();
		return -EINVAL;
	}

	/*
	 * Convertimos las páginas a KB utilizando:
	 *   (valor * PAGE_SIZE) >> 10
	 * Esto nos da el total de memoria virtual (vm_kb) y memoria física (rss_kb)
	 * en kilobytes.
	 */
	kinfo.vm_kb  = (mm->total_vm     * PAGE_SIZE) >> 10;  
	kinfo.rss_kb = (get_mm_rss(mm)   * PAGE_SIZE) >> 10;  

	/*
	 * Calculamos el porcentaje de memoria física (RSS) sobre la memoria virtual.
	 * Si la memoria virtual es 0, asignamos el porcentaje como 0 para evitar
	 * divisiones por cero.
	 */
	if (kinfo.vm_kb > 0)
		kinfo.rss_percent_of_vm = (kinfo.rss_kb * 100) / kinfo.vm_kb;
	else
		kinfo.rss_percent_of_vm = 0;                  

	/*
	 * Extraemos el valor de ajuste respecto al OOM killer desde la
	 * estructura de señales del proceso y lo almacenamos en la estructura
	 * de salida (kinfo).
	 */
	kinfo.oom_adjustment = task->signal->oom_score_adj;  

	/*
	 * Liberamos la referencia a la estructura mm_struct que habíamos obtenido
	 * previamente para evitar fugas de memoria.
	 */
	mmput(mm);                                       

	/*
	 * Liberamos el bloqueo RCU para indicar que hemos terminado de
	 * acceder a las estructuras protegidas.
	 */
	rcu_read_unlock();                               

	/*
	 * Copiamos la información recopilada desde el espacio del kernel (kinfo)
	 * al espacio de usuario (info). Si la operación falla, devolvemos un
	 * error de fallo de memoria (EFAULT).
	 */
	if (copy_to_user(info, &kinfo, sizeof(kinfo)))
		return -EFAULT;                             

	/*
	 * Retornamos 0 para indicar que la operación se completó con éxito.
	 */
	return 0;                                       
}
