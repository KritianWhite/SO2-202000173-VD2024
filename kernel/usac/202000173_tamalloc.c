#include <linux/syscalls.h>   // Para SYSCALL_DEFINE
#include <linux/mm.h>         // Para do_mmap, find_vma, vm_fault, etc.
#include <linux/mman.h>       // Para MAP_*, PROT_*
#include <linux/slab.h>       // Para alloc_page, kzalloc, etc. (si lo requieres)
#include <linux/uaccess.h>    // Para copy_to_user, etc. (si fuera necesario)
#include <linux/mm_types.h>
#include <linux/sched.h>      // current->mm
#include <asm/page.h>         // PAGE_ALIGN, PAGE_SIZE, etc.

/* 
* --------------------------------------------------------------------
* Función tamalloc_fault:
* Se encarga de atender el page fault, asignando una página y
* limpiándola en cero la PRIMERA vez que se accede a ella.
* --------------------------------------------------------------------
*/
static vm_fault_t tamalloc_fault(struct vm_fault *vmf)
{
    struct page *page;

    // Intentar asignar una página física
    page = alloc_page(GFP_KERNEL);
    if (!page)
        return VM_FAULT_OOM;  // Indica al kernel que no pudo asignarse

    /*
     * Limpiamos (en cero) la página asignada.
     * clear_user_page() es una forma “segura” de hacer memset a 0.
     * También podríamos usar: memset(page_address(page), 0, PAGE_SIZE).
     */
    clear_user_page(page_address(page), vmf->address, page);

    // Aumentamos la cuenta de referencia para evitar que el kernel libere la página
    get_page(page);

    // Retornamos la página para que el kernel la mapee
    vmf->page = page;
    return VM_FAULT_NOPAGE;
}

/* 
* --------------------------------------------------------------------
* Estructura vm_operations para vincular la función de fallo
* --------------------------------------------------------------------
*/
static const struct vm_operations_struct tamalloc_vm_ops = {
    .fault = tamalloc_fault,  // Nuestra función de page fault perezoso
};

/* --------------------------------------------------------------------
* Syscall _202000173_tamalloc
* Recibe: size_t size => cantidad de bytes a mapear perezosamente
*--------------------------------------------------------------------
*/
SYSCALL_DEFINE1(_202000173_tamalloc, size_t, size)
{
    struct mm_struct *mm = current->mm;
    struct vm_area_struct *vma;
    unsigned long addr;
    unsigned long populate = 0; // do_mmap() lo usa si queremos “forzar” pre-fault
    long ret;

    // Alinear el tamaño al tamaño de página
    size = PAGE_ALIGN(size);
    if (!size)
        return -EINVAL;

    /*
    * ----------------------------------------------------------------
    * do_mmap() en kernel 6.5 recibe 8 argumentos:
    * 1) struct file *file      => NULL => mapeo anónimo
    * 2) unsigned long addr     => 0 => el kernel elige la base
    * 3) unsigned long len      => tamaño en bytes
    * 4) unsigned long prot     => PROT_READ | PROT_WRITE
    * 5) unsigned long flags    => MAP_PRIVATE | MAP_ANONYMOUS
    * 6) unsigned long pgoff    => 0 => no aplica a anónimos
    * 7) unsigned long *populate => &populate o NULL
    * 8) struct list_head *uf   => NULL
    * ----------------------------------------------------------------
    */ 
    addr = do_mmap(NULL, 0, size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   0,
                   &populate,
                   NULL);

    // Si do_mmap falla, retorna un valor “IS_ERR_VALUE”
    if (IS_ERR_VALUE(addr))
        return (long)addr; // Por ejemplo -ENOMEM, -EFAULT, etc.

    // Tomamos el lock de mm para modificar la VMA
    down_write(&mm->mmap_lock);
    vma = find_vma(mm, addr);
    if (!vma) {
        up_write(&mm->mmap_lock);
        return -EFAULT;
    }

    // Vinculamos nuestro vm_ops para el page fault perezoso
    vma->vm_ops = &tamalloc_vm_ops;

    // Si deseas asociar datos privados: vma->vm_private_data = ...
    up_write(&mm->mmap_lock);

    // Retornamos la dirección de usuario en que se mapeó
    return addr;
}