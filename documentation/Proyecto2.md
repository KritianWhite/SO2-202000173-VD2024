# Proyecto 2

## Cronograma del Proyecto: "Tamalloc con Lazy-Zeroing"

### Día 1: Investigación de algoritmos de asignación de memoria
- Estudio detallado de:
  - `malloc`, `calloc`, `kmalloc`, `kzalloc`, `mmap`, y conceptos de `lazy-zeroing`.
- Revisión de ejemplos en el kernel relacionados con la gestión de memoria.

### Día 2: Diseño del algoritmo "Tamalloc"
- Definir la estructura del código para implementar lazy-zeroing.
- Identificar funciones del kernel necesarias (e.g., manejo de `page faults` y `syscalls`).

### Día 3-4: Implementación del algoritmo "Tamalloc"
- Codificar la funcionalidad para la inicialización de memoria en 0 al primer acceso.
- Implementar los mecanismos que evitan la reserva inmediata de páginas físicas.

### Día 5: Desarrollo de las syscalls de estadísticas
- Implementar syscalls para recolección de estadísticas:
  - Memoria reservada.
  - Memoria utilizada.
  - `OOM Score`.
- Crear funciones para calcular memoria utilizada por procesos individuales y totales.

### Día 6: Integración y pruebas finales
- Integrar "Tamalloc" en el kernel modificado (`Linux 6.5.0-49-usac1`).
- Realizar pruebas con scripts en C y Bash:
  - Validar que las páginas no se marquen como utilizadas inmediatamente.
  - Mostrar las estadísticas recolectadas.
- Documentar:
  - Diseño, implementación, pruebas y resultados obtenidos.
  - Problemas encontrados, causas, soluciones y conclusiones.
--- 

## **Syscall (_202000173_tamalloc)**

### **Objetivo**
La syscall `_202000173_tamalloc` recopila información sobre el uso de memoria global en el sistema, incluyendo la memoria virtual y física utilizada por todos los procesos en ejecución (excluyendo procesos en estado zombi).

### **Descripción del Código**

#### **1. Estructura `tamalloc_global_info`**
Esta estructura almacena datos globales de memoria:
- **`aggregate_vm_mb`**: Memoria virtual total utilizada por todos los procesos (en MB).
- **`aggregate_rss_mb`**: Memoria física (RSS - Resident Set Size) total utilizada por todos los procesos (en MB).

```c
struct tamalloc_global_info {
    unsigned long aggregate_vm_mb; // Memoria virtual total en MB
    unsigned long aggregate_rss_mb; // Memoria física total en MB
};
```

#### **2. Implementación de la Syscall**
La syscall está definida como:
```c
SYSCALL_DEFINE1(_202000173_tamalloc, struct tamalloc_global_info __user *, info)
```

##### **Flujo de la Syscall**
1. **Inicialización de variables:**
   - Se define un objeto `task_struct` para iterar sobre los procesos.
   - Se crea una instancia de `tamalloc_global_info` para almacenar los datos globales.
   - Se utiliza `mm_struct` para obtener información de memoria de cada proceso.

2. **Bloqueo de estructuras RCU:**
   - Se llama a `rcu_read_lock` para proteger el acceso concurrente a estructuras de datos del kernel durante la iteración de procesos.

3. **Iteración sobre procesos:**
   - Se utiliza el macro `for_each_process` para iterar sobre todos los procesos en el sistema.
   - Si un proceso está en estado zombi (`EXIT_ZOMBIE`), se omite.
   - Si el proceso no tiene estructura `mm_struct` (por ejemplo, hilos del kernel), también se omite.

4. **Cálculo de memoria:**
   - La memoria virtual (`total_vm`) se convierte de páginas a MB: `(mm->total_vm * PAGE_SIZE) >> 20`.
   - La memoria física utilizada (RSS) se calcula usando `get_mm_rss(mm)` y la misma fórmula de conversión.

5. **Liberación de recursos:**
   - Después de procesar cada `mm_struct`, se libera la referencia con `mmput(mm)`.
   - Se llama a `rcu_read_unlock` al final de la iteración.

6. **Copia al espacio de usuario:**
   - La información agregada (`kinfo`) se copia al espacio de usuario utilizando `copy_to_user`.
   - Si la copia falla, se retorna el error `-EFAULT`.

7. **Finalización:**
   - La syscall retorna 0 si se ejecuta correctamente.


## **Implementación del Usuario**

### **1. Estructura `tamalloc_global_info`**
Se define de manera idéntica a la syscall, pero en el espacio de usuario.

```c
struct tamalloc_global_info {
    unsigned long aggregate_vm_mb; // Memoria virtual total en MB
    unsigned long aggregate_rss_mb; // Memoria física total en MB
};
```

### **2. Llamada a la Syscall**
La función `tamalloc_get_global_stats` invoca la syscall mediante `syscall(__NR__202000173_tamalloc, info)`.

```c
static inline long tamalloc_get_global_stats(struct tamalloc_global_info *info)
{
    return syscall(__NR__202000173_tamalloc, info);
}
```

### **3. Impresión de Información**
La función `print_global_info` formatea los datos de memoria y los imprime de manera estilizada en la consola.

```c
static void print_global_info(const struct tamalloc_global_info *info)
{
    printf("\033[1;34m==================================================================\033[0m\n");
    printf("\033[1;36m                      MONITOREO DE MEMORIA                        \033[0m\n");
    printf("\033[1;34m==================================================================\033[0m\n");
    printf("\033[1;37m| %-36s | \033[1;32m%-20lu MB\033[1;37m |\033[0m\n", "vmSize", info->aggregate_vm_mb);
    printf("\033[1;37m| %-36s | \033[1;32m%-20lu MB\033[1;37m |\033[0m\n", "vmRSS", info->aggregate_rss_mb);
    printf("\033[1;34m==================================================================\033[0m\n");
}
```

### **4. Programa Principal**
El programa principal implementa un monitor de memoria en tiempo real:
1. **Captura de señales:**
   - Usa `signal(SIGINT, signal_handler)` para manejar interrupciones (Ctrl+C) y salir limpiamente.

2. **Bucle infinito:**
   - Se llama a la syscall para obtener estadísticas globales de memoria.
   - Se imprime la información actualizada utilizando `print_global_info`.
   - Se limpia la pantalla (`\033[H\033[J`) antes de cada actualización.
   - Se agrega un retardo de 1 segundo (`usleep(1000000)`).

3. **Finalización:**
   - Si la syscall falla, se imprime un error con `perror` y el programa termina.
--- 

## **Syscall (_202000173_memory_allocation_statistics)**

### **Objetivo**
La syscall `_202000173_memory_allocation_statistics` permite recopilar estadísticas detalladas de memoria para un proceso específico identificado por su PID. Proporciona información sobre la memoria virtual, memoria física utilizada, porcentaje de uso de RSS sobre memoria virtual y el ajuste del proceso respecto al OOM killer.

### **Descripción del Código**

#### **1. Estructura `tamalloc_proc_info`**
Esta estructura se utiliza para almacenar estadísticas relacionadas con el uso de memoria de un proceso individual:
- **`vm_kb`**: Memoria virtual total en KB.
- **`rss_kb`**: Memoria física utilizada (Resident Set Size - RSS) en KB.
- **`rss_percent_of_vm`**: Porcentaje de memoria física (RSS) sobre la memoria virtual.
- **`oom_adjustment`**: Ajuste del proceso respecto al OOM killer.

```c
struct tamalloc_proc_info {
    unsigned long vm_kb;                // Memoria virtual en KB
    unsigned long rss_kb;               // Memoria física en KB
    unsigned int rss_percent_of_vm;     // Porcentaje de RSS sobre memoria virtual
    int oom_adjustment;                 // Ajuste del OOM killer
};
```

#### **2. Implementación de la Syscall**
La syscall está definida como:
```c
SYSCALL_DEFINE2(_202000173_memory_allocation_statistics, pid_t, pid, struct tamalloc_proc_info __user *, info)
```

##### **Flujo de la Syscall**
1. **Inicialización de variables:**
   - Se declara un puntero `task_struct` para identificar al proceso por su PID.
   - Se utiliza un objeto `tamalloc_proc_info` para recopilar las estadísticas de memoria.

2. **Protección con RCU:**
   - Se usa `rcu_read_lock` para garantizar un acceso seguro a las estructuras de datos compartidas del kernel.

3. **Búsqueda del proceso:**
   - Se utiliza `pid_task(find_vpid(pid), PIDTYPE_PID)` para localizar el proceso con el PID especificado.
   - Si el proceso no existe, se retorna el error `-ESRCH`.

4. **Acceso a la memoria del proceso:**
   - Se obtiene la estructura `mm_struct` del proceso, que contiene información de memoria virtual y física.
   - Si `mm_struct` no está disponible (por ejemplo, hilos del kernel), se retorna el error `-EINVAL`.

5. **Cálculo de memoria:**
   - **Memoria virtual (`vm_kb`)**: Se convierte de páginas a KB: `(mm->total_vm * PAGE_SIZE) >> 10`.
   - **Memoria física (`rss_kb`)**: Se calcula con `get_mm_rss(mm)` usando la misma fórmula.
   - **Porcentaje de uso de RSS**: Si la memoria virtual es mayor a 0, se calcula: `(rss_kb * 100) / vm_kb`.

6. **Ajuste del OOM killer:**
   - Se obtiene de la estructura `signal->oom_score_adj` del proceso.

7. **Liberación de recursos:**
   - Se libera la referencia a `mm_struct` con `mmput`.
   - Se desbloquea RCU con `rcu_read_unlock`.

8. **Copia al espacio de usuario:**
   - Los datos recopilados en `tamalloc_proc_info` se copian al espacio de usuario mediante `copy_to_user`.
   - Si la copia falla, se retorna el error `-EFAULT`.

9. **Finalización:**
   - La syscall retorna 0 si se ejecuta correctamente.

## **Implementación del Usuario**

### **1. Estructura `tamalloc_proc_info`**
La estructura se define de manera idéntica a la utilizada en la syscall para recibir los datos en el espacio de usuario.

```c
struct tamalloc_proc_info {
    unsigned long vm_kb;
    unsigned long rss_kb;
    unsigned int rss_percent_of_vm;
    int oom_adjustment;
};
```

### **2. Llamada a la Syscall**
La función `tamalloc_get_indiviual_stats` invoca la syscall utilizando:
```c
return syscall(__NR__202000173_memory_allocation_statistics, pid, info);
```

### **3. Formateo de Datos**
Se proporcionan funciones para imprimir la información en formato de tabla:

#### **Encabezado de Tabla**
Imprime las columnas de la tabla:
```c
static void header_table(void)
{
    printf("\033[1;37m__________________________________________________________________\n");
    printf("\033[1;37m│ \033[1;31m%-6s \033[1;37m│ \033[1;31m%-15s \033[1;37m│ \033[1;31m%-15s \033[1;37m│ \033[1;31m%-8s \033[1;37m│ \033[1;31m%-5s \033[1;37m│\n", 
           "PID", "Reserved (KB)", "Commited (KB)", "Usage (%)", "OOM");
    printf("\033[1;37m__________________________________________________________________\n");
}
```

#### **Filas de Tabla**
Imprime los datos de un proceso específico:
```c
static void body_table(pid_t pid, const struct tamalloc_proc_info *pinfo)
{
    printf("\033[1;37m│ \033[1;34m%-6d \033[1;37m│ \033[1;34m%-15lu \033[1;37m│ \033[1;34m%-15lu \033[1;37m│ \033[1;34m%-8u \033[1;37m│ \033[1;34m%-6d \033[1;37m│\n",
           pid,
           pinfo->vm_kb,
           pinfo->rss_kb,
           pinfo->rss_percent_of_vm,
           pinfo->oom_adjustment);
    printf("\033[1;37m__________________________________________________________________\n");
}
```

### **4. Programa Principal**
El programa utiliza la syscall para mostrar estadísticas de memoria de procesos individuales o de todos los procesos.

#### **Flujo del Programa**
1. **Modo específico (PID):**
   - Si se proporciona un PID como argumento, se llama a la syscall para obtener estadísticas del proceso correspondiente.
   - Los datos se imprimen en formato de tabla.

2. **Modo general (todos los procesos):**
   - Si no se proporciona un PID, se itera sobre los directorios numéricos en `/proc` (cada uno representa un proceso).
   - Para cada proceso válido, se llama a la syscall y se imprimen los datos si la llamada es exitosa.

3. **Iteración sobre `/proc`:**
   - Se verifica si el nombre del directorio es numérico (PID).
   - Se llama a la syscall para cada proceso y se imprimen los datos en la tabla.

4. **Finalización:**
   - El programa cierra el directorio `/proc` y finaliza.
---

## Syscall (_202000173_tamalloc_stats)**

### **Objetivo**
La syscall `_202000173_tamalloc_stats` permite al usuario asignar una región de memoria en el espacio de direcciones del proceso llamante. La dirección base del mapeo se devuelve como resultado, permitiendo interactuar con la memoria asignada.

### **Descripción del Código**

#### **1. Validación del Tamaño**
El tamaño solicitado por el usuario (`size`) se valida antes de cualquier operación:
- Si el tamaño es `0`, la syscall retorna el error `-EINVAL`, indicando un parámetro inválido.

```c
if (size == 0)
    return -EINVAL;
```

#### **2. Alineación del Tamaño**
El tamaño solicitado se alinea a un múltiplo del tamaño de página (`PAGE_SIZE`, generalmente 4 KB):
- Se utiliza la macro `PAGE_ALIGN(size)` para realizar esta operación.
- Si el tamaño alineado no es válido, se retorna `-ENOMEM` indicando que no se puede asignar memoria.

```c
aligned_size = PAGE_ALIGN(size);
if (!aligned_size)
    return -ENOMEM;
```

#### **3. Asignación de Memoria**
Se utiliza la función del kernel `vm_mmap` para asignar una región de memoria:
- **Argumentos de `vm_mmap`:**
  - **NULL**: Permite al kernel elegir automáticamente la dirección base del mapeo.
  - **0**: Offset inicial (irrelevante para mapeos anónimos).
  - **aligned_size**: Tamaño de la memoria alineada solicitada.
  - **PROT_READ | PROT_WRITE**: Permisos del mapeo (lectura y escritura).
  - **MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE**:
    - **MAP_PRIVATE**: El mapeo es privado para el proceso.
    - **MAP_ANONYMOUS**: El mapeo no está asociado a ningún archivo.
    - **MAP_NORESERVE**: No se reserva memoria física hasta que se acceda.
  - **0**: Offset del archivo (irrelevante para mapeos anónimos).

```c
addr = vm_mmap(NULL, 0, aligned_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0);
```

- Si la operación falla, `vm_mmap` retorna un valor negativo indicando el error. Se registra el fallo en el log del kernel y la syscall retorna `-ENOMEM`.

```c
if (IS_ERR_VALUE(addr)) {
    printk(KERN_ERR "_202000173_tamalloc_stats: Error al mapear memoria.\n");
    return -ENOMEM;
}
```

#### **4. Retorno de la Dirección Base**
Si la operación es exitosa, la syscall retorna la dirección base de la memoria asignada.

```c
return addr;
```

## Implementación del Usuario**

### **1. Llamada a la Syscall**
La función `tamalloc_get_stat` invoca la syscall mediante:
```c
static inline long tamalloc_get_stat(size_t size)
{
    return syscall(__NR__202000173_tamalloc_stats, size);
}
```

### **2. Programa Principal**
El programa principal interactúa con la syscall para asignar memoria en tiempo real y demostrar su uso.

#### **Flujo del Programa**
1. **Entrada de Tamaño:**
   - Si se proporciona un argumento en la línea de comandos, este se interpreta como el tamaño de la memoria a asignar. Si no, se utiliza un tamaño predeterminado de 1 MB.

```c
size_t size = 1024 * 1024; // 1 MB por defecto
if (argc > 1)
    size = (size_t)atol(argv[1]);
```

2. **Bucle Infinito:**
   - Se llama a la syscall para asignar memoria y se imprime la dirección base del mapeo.
   - Se limpia la pantalla (`system("clear")`) en cada iteración para simular un monitoreo en tiempo real.

```c
long addr = tamalloc_get_stat(size);
if (addr < 0) {
    perror("tamalloc_get_stat syscall");
    return 1;
}

printf("[Tiempo Real] Se asignaron %zu bytes en 0x%lx\n", size, addr);
```

3. **Interacción con la Memoria:**
   - Se realiza una escritura en la memoria asignada para demostrar la interacción.
   - Se escriben los caracteres `'H'` y `'Z'` al inicio y final del bloque de memoria, respectivamente.

```c
char *ptr = (char *)addr;
if (addr != 0) {
    ptr[0] = 'H';
    ptr[size - 1] = 'Z';
    printf("Datos escritos: Inicio = '%c', Final = '%c'\n", ptr[0], ptr[size - 1]);
}
```

4. **Retardo en la Actualización:**
   - Se agrega un retardo de 1 segundo antes de la siguiente iteración usando `nanosleep`.

```c
sleep_ms(1000);
```
---

## Resumen de las Syscalls Agregadas al Archivo `tbl`

### **Listado de Syscalls**
1. **`_202000173_tamalloc`** (ID: `550`)
   - Proporciona estadísticas de uso de memoria global (virtual y física) para todos los procesos en el sistema.

2. **`_202000173_memory_allocation_statistics`** (ID: `551`)
   - Recopila estadísticas de uso de memoria para un proceso específico identificado por su PID, incluyendo memoria virtual, memoria física utilizada, porcentaje de uso de RSS y ajuste de OOM killer.

3. **`_202000173_tamalloc_stats`** (ID: `552`)
   - Permite asignar dinámicamente una región de memoria en el espacio de direcciones del proceso llamante, retornando la dirección base del mapeo.

### **Detalles Técnicos**
- El formato es: 
  - **ID (Número de Syscall)**: Identificador único asignado.
  - **Tipo (`common`)**: Indica que la syscall es común para todas las arquitecturas compatibles.
  - **Nombre (`_202000173_*`)**: Identifica la syscall en el espacio de usuario.
  - **Función (`sys__202000173_*`)**: Nombre de la implementación de la syscall en el espacio del kernel.

