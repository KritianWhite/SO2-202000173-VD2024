#include <linux/kernel.h>
#include <linux/syscalls.h>   // Para SYSCALL_DEFINE, definir nuevas syscalls
#include <linux/mm.h>         // mm_struct, manejo de memoria
#include <linux/mman.h>       // vm_mmap, PROT_READ, PROT_WRITE, MAP_PRIVATE
#include <linux/errno.h>      // Manejo de códigos de error como -EINVAL, -ENOMEM
#include <linux/sched.h>      // Información de tareas/procesos

/*
 * Syscall: _202000173_tamalloc_stats
 *
 * Esta syscall permite al usuario asignar una región de memoria en el espacio
 * de direcciones del proceso llamante, retornando la dirección base del
 * mapeo si tiene éxito.
 *
 * Argumentos:
 *   - size: Tamaño en bytes de la región de memoria solicitada.
 *
 * Retorno:
 *   - Dirección base del mapeo (unsigned long) si tiene éxito.
 *   - -EINVAL si el tamaño solicitado es 0.
 *   - -ENOMEM si no se puede asignar memoria (por ejemplo, si el tamaño está mal alineado o no hay suficiente memoria).
 */
SYSCALL_DEFINE1(_202000173_tamalloc_stats, size_t, size)
{
	/*
	 * aligned_size almacenará el tamaño alineado a múltiplos del tamaño de página
	 * del sistema. addr será la dirección base del mapeo de memoria.
	 */
	unsigned long aligned_size, addr;

	/*
	 * Validamos que el tamaño proporcionado no sea 0. Si lo es, retornamos
	 * -EINVAL para indicar un parámetro inválido.
	 */
	if (size == 0)
		return -EINVAL;

	/*
	 * Alineamos el tamaño solicitado a un múltiplo de PAGE_SIZE (generalmente 4 KB).
	 * Esto es necesario porque la memoria en el sistema se asigna en páginas.
	 * Si aligned_size es 0 después de esta operación, significa que no se
	 * pudo alinear correctamente, y retornamos -ENOMEM indicando falta de memoria.
	 */
	aligned_size = PAGE_ALIGN(size);
	if (!aligned_size)
		return -ENOMEM;

	/*
	 * Solicitamos la asignación de memoria utilizando vm_mmap. Este método crea
	 * un mapeo anónimo en el espacio de direcciones del proceso llamante.
	 *
	 * Argumentos de vm_mmap:
	 *   - NULL: El kernel elige la dirección base del mapeo.
	 *   - 0: Offset inicial (no se usa para mapeos anónimos).
	 *   - aligned_size: Tamaño del mapeo solicitado (en bytes).
	 *   - PROT_READ | PROT_WRITE: Permisos del mapeo (lectura y escritura).
	 *   - MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE: Opciones para el mapeo.
	 *     - MAP_PRIVATE: El mapeo no se comparte con otros procesos.
	 *     - MAP_ANONYMOUS: El mapeo no está asociado a un archivo.
	 *     - MAP_NORESERVE: No se reserva espacio físico hasta que se accede.
	 *   - 0: Offset del archivo (no relevante para mapeos anónimos).
	 */
	addr = vm_mmap(NULL, 0, aligned_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0);

	/*
	 * Verificamos si vm_mmap retornó un valor de error (un código de error negativo).
	 * Si es así, imprimimos un mensaje de error en el log del kernel y
	 * retornamos -ENOMEM indicando que no se pudo asignar la memoria.
	 */
	if (IS_ERR_VALUE(addr)) {
		printk(KERN_ERR "_202000173_tamalloc_stats: Error al mapear memoria.\n");
		return -ENOMEM;
	}

	/*
	 * Retornamos la dirección base del mapeo si la operación se completó con éxito.
	 */
	return addr; 
}
