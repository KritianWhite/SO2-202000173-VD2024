#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/vmstat.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/string.h>

struct memory_snapshot {
    unsigned long total_ram;
    unsigned long free_ram;
    unsigned long swap_total;
    unsigned long swap_free;
    unsigned long cache_ram;
    unsigned long buffer_ram;
    unsigned long active_ram;
    unsigned long inactive_ram;
};

static void fill_memory_snapshot(struct memory_snapshot *snap) {
    struct sysinfo i;
    unsigned long file_pages, shmem, bufferram;
    unsigned long active_file, active_anon;
    unsigned long inactive_file, inactive_anon;

    // Obtener información sobre la memoria
    si_meminfo(&i);
    
    // Guardar valores que se usan varias veces para evitar multiples llamadas
    bufferram = i.bufferram;    
    file_pages = global_node_page_state(NR_FILE_PAGES);
    shmem = global_node_page_state(NR_SHMEM);
    active_file = global_node_page_state(NR_ACTIVE_FILE);
    active_anon = global_node_page_state(NR_ACTIVE_ANON);
    inactive_file = global_node_page_state(NR_INACTIVE_FILE);
    inactive_anon = global_node_page_state(NR_INACTIVE_ANON);

    // Asignación de la información básica de RAM y SWAP
    snap->total_ram   = i.totalram;
    snap->free_ram    = i.freeram;
    snap->swap_total  = i.totalswap;
    snap->swap_free   = i.freeswap;

    // Cálculo de la memoria en caché
    // cache_ram = NR_FILE_PAGES - NR_SHMEM - bufferram
    snap->cache_ram = file_pages - shmem - bufferram;

    // Memoria en buffer
    snap->buffer_ram = bufferram;

    // Memoria activa e inactiva
    // active_ram = NR_ACTIVE_FILE + NR_ACTIVE_ANON
    // inactive_ram = NR_INACTIVE_FILE + NR_INACTIVE_ANON
    snap->active_ram = active_file + active_anon;
    snap->inactive_ram = inactive_file + inactive_anon;
}

SYSCALL_DEFINE1(_202000173_capture_memory_snapshot, struct memory_snapshot __user *, user_snap)
{
    struct memory_snapshot snap;

    // Llenar la estructura de snapshot de memoria
    fill_memory_snapshot(&snap);

    if (copy_to_user(user_snap, &snap, sizeof(snap)))
        return -EFAULT;

    return 0;
}
