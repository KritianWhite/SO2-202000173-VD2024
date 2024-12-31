#include <linux/kernel.h>
#include <linux/syscalls.h>   // Para SYSCALL_DEFINE, definir nuevas syscalls
#include <linux/sched.h>      // for_each_process, task_struct, manejo de procesos
#include <linux/mm.h>         // mm->total_vm, get_mm_rss, información de memoria
#include <linux/uaccess.h>    // copy_to_user, para interacción con espacio de usuario
#include <linux/rcupdate.h>   // rcu_read_lock, sincronización para estructuras protegidas por RCU

/*
 * Estructura para la syscall _202000173_tamalloc
 *
 * Esta estructura recopila información global sobre el uso de memoria
 * agregada de todos los procesos en el sistema.
 * Contiene:
 *   - aggregate_vm_mb: Suma de la memoria virtual utilizada por todos los procesos, en MB
 *   - aggregate_rss_mb: Suma de la memoria física utilizada por todos los procesos, en MB
 */
struct tamalloc_global_info {
	unsigned long aggregate_vm_mb; // Memoria virtual total agregada en MB
	unsigned long aggregate_rss_mb; // Memoria física total agregada en MB
};

/*
 * Syscall: _202000173_tamalloc
 *
 * Esta syscall recopila información sobre el uso de memoria global de todos
 * los procesos que se están ejecutando en el sistema (excepto los procesos zombi).
 * Argumentos:
 *   - info: Puntero a una estructura en el espacio de usuario donde se
 *           almacenarán los datos globales sobre memoria.
 */
SYSCALL_DEFINE1(_202000173_tamalloc, struct tamalloc_global_info __user *, info)
{
	/*
	 * task_struct representa a cada proceso dentro del kernel.
	 * Se utiliza para iterar sobre todos los procesos en el sistema.
	 */
	struct task_struct *task;                       

	/*
	 * Estructura temporal en el espacio del kernel donde se almacenará
	 * la información agregada de memoria antes de copiarla al espacio
	 * de usuario.
	 */
	struct tamalloc_global_info kinfo = {0};        

	/*
	 * mm_struct contiene la información de memoria asociada a cada proceso.
	 * Se utiliza para acceder a los datos de memoria virtual y física.
	 */
	struct mm_struct *mm;                          

	/*
	 * Bloqueamos el acceso concurrente a las estructuras protegidas por
	 * RCU para poder iterar de manera segura sobre los procesos activos.
	 */
	rcu_read_lock();                                

	/*
	 * Iteramos sobre todos los procesos en el sistema utilizando el macro
	 * for_each_process, que proporciona cada proceso en forma de task_struct.
	 */
	for_each_process(task) {
		/*
		 * Si el proceso está en estado zombi (EXIT_ZOMBIE), lo omitimos ya que
		 * no tiene información de memoria válida.
		 */
		if (task->exit_state == EXIT_ZOMBIE)
			continue;

		/*
		 * Obtenemos la información de memoria del proceso a través de mm_struct.
		 * Si el proceso no tiene información de memoria (por ejemplo, hilos del kernel),
		 * lo omitimos y continuamos con el siguiente proceso.
		 */
		mm = get_task_mm(task);
		if (!mm)
			continue;

		/*
		 * Convertimos el total de páginas de memoria virtual a MB.
		 * La fórmula utilizada es:
		 *   (mm->total_vm * PAGE_SIZE) >> 20
		 * Donde >> 20 equivale a dividir por 1024 dos veces (para convertir a MB).
		 */
		kinfo.aggregate_vm_mb +=
			(mm->total_vm * PAGE_SIZE) >> 20;

		/*
		 * Convertimos el total de páginas de memoria física utilizada (RSS) a MB
		 * utilizando la misma fórmula de conversión a MB que para la memoria virtual.
		 */
		kinfo.aggregate_rss_mb +=
			(get_mm_rss(mm) * PAGE_SIZE) >> 20;

		/*
		 * Liberamos la referencia a la estructura mm_struct para evitar
		 * fugas de memoria.
		 */
		mmput(mm);
	}

	/*
	 * Liberamos el bloqueo RCU ya que hemos terminado de iterar sobre
	 * los procesos.
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
