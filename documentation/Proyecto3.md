### Universidad de San Carlos de Guatemala  
### Facultad de Ingeniería  
#### Escuela de Ciencias y Sistemas  
**Curso:** Sistemas Operativos 2  
**Catedrático:** Ing. René Ornelyz  
**Auxiliar:** Brian Matus

---

## Cronograma para Implementación y Pruebas de Syscalls

### **Domingo 29 de Diciembre**
#### **Objetivo: Configurar entorno y crear la base del código**
1. **Configuración inicial:**
   - Configurar el entorno de desarrollo (asegurarte de tener el kernel 6.5.0 preparado para compilar).
   - Revisión de los archivos necesarios para implementar las syscalls (e.g., `syscall_64.tbl`, directorio `kernel/`).
   - Preparar y documentar las herramientas de pruebas (e.g., `gcc`, permisos de usuario, acceso a sudo).

2. **Implementación inicial:**
   - Implementar la syscall **_202000173_add_memory_limit** (Agregar límite de memoria).
   - Probar esta syscall con casos simples (e.g., añadir un proceso con límite válido).

### **Lunes 30 de Diciembre**
#### **Objetivo: Completar la implementación de las syscalls**
1. **Implementación de Syscalls:**
   - Completar la implementación de las siguientes syscalls:
     - **_202000173_get_memory_limits** (Obtener lista de procesos limitados).
     - **_202000173_update_memory_limit** (Actualizar límite de memoria).
     - **_202000173_remove_memory_limit** (Eliminar límite de memoria).

2. **Validaciones y Manejo de Errores:**
   - Verificar el manejo de errores para cada syscall (e.g., PID inválido, permisos insuficientes).
   - Implementar pruebas de casos límite como:
     - Límite negativo.
     - PID inexistente.
     - Procesos no registrados.

3. **Compilación del kernel:**
   - Compilar el kernel con las nuevas syscalls.
   - Instalar y reiniciar en el kernel modificado.

### **Martes 31 de Diciembre**
#### **Objetivo: Realizar pruebas y documentar**
1. **Pruebas:**
   - Ejecutar el programa en espacio de usuario para interactuar con las syscalls.
   - Verificar las siguientes operaciones:
     - Agregar, obtener, actualizar y eliminar límites de memoria.

2. **Validación de resultados:**
   - Confirmar que las syscalls funcionan según lo esperado:
     - Lista de procesos limitada correctamente.
     - Límite de memoria actualizado y eliminado según sea necesario.

3. **Documentación:**
   - Describir la implementación final y las pruebas realizadas.
   - Documentar cualquier problema encontrado y su solución.
   - Preparar el mensaje de conclusión personal, como se solicita en el proyecto.
---

## **Syscall: _202000173_add_memory_limit**

### **Descripción**
Esta syscall agrega un límite de memoria para un proceso específico. Se utiliza para evitar que ciertos procesos consuman más memoria de la asignada por el administrador del sistema. La información del proceso y su límite de memoria se almacenan en una lista global dentro del kernel.

### **Prototipo**
```c
SYSCALL_DEFINE2(_202000173_add_memory_limit, pid_t, process_pid, size_t, memory_limit);
```

### **Parámetros**
1. **`process_pid`** (`pid_t`):
   - El ID del proceso (PID) al que se le desea asignar un límite de memoria.
   - Debe ser un número mayor que 0.

2. **`memory_limit`** (`size_t`):
   - El límite de memoria en bytes que se desea asignar al proceso.
   - Debe ser un valor mayor que 0.

### **Retornos**
La syscall devuelve un valor entero para indicar el resultado de la operación:
- `0`: Operación exitosa.
- `-EINVAL`: PID o límite de memoria inválido (valores no positivos).
- `-EPERM`: El usuario no tiene permisos de administrador (`CAP_SYS_ADMIN` requerido).
- `-EEXIST`: El proceso ya tiene un límite definido en la lista.
- `-ENOMEM`: No hay suficiente memoria en el kernel para crear una nueva entrada.

### **Estructura Usada**
```c
struct memory_limitation {
    pid_t pid;               // ID del proceso
    size_t memory_limit;     // Límite de memoria en bytes
    struct list_head list;   // Nodo para la lista enlazada
};
```

### **Funcionamiento**
1. **Validación de parámetros:**
   - Se verifica que el `process_pid` sea mayor a 0.
   - Se asegura que el `memory_limit` sea mayor a 0.
   - Si alguno de estos parámetros es inválido, se retorna `-EINVAL`.

2. **Validación de permisos:**
   - Se utiliza la función `capable(CAP_SYS_ADMIN)` para asegurarse de que el usuario tiene privilegios de administrador. Si no los tiene, se retorna `-EPERM`.

3. **Verificación de duplicados:**
   - Se recorre la lista global `memory_limited_processes` para comprobar si el `process_pid` ya tiene un límite asignado. Si existe, se retorna `-EEXIST`.

4. **Creación de la entrada:**
   - Se utiliza `kmalloc` para asignar memoria en el espacio del kernel para un nuevo nodo de la lista.
   - Si no hay suficiente memoria disponible, se retorna `-ENOMEM`.

5. **Adición a la lista:**
   - Se inicializa la estructura con el PID y el límite de memoria proporcionados.
   - Se añade el nodo a la lista global `memory_limited_processes` utilizando `list_add`.

6. **Retorno:**
   - Si todo el proceso es exitoso, se retorna `0`.

---

## **Syscall: _202000173_get_memory_limits**

### **Descripción**
Esta syscall permite obtener una lista de procesos que tienen límites de memoria configurados. Los datos son copiados desde una lista global en el kernel hacia un buffer en el espacio de usuario, incluyendo información sobre el `PID` y el límite de memoria asociado.

### **Prototipo**
```c
SYSCALL_DEFINE3(_202000173_get_memory_limits, 
                struct memory_limitation __user *, user_buffer, 
                size_t, max_entries, 
                int __user *, processes_returned);
```

### **Parámetros**
1. **`user_buffer`** (`struct memory_limitation __user *`):
   - Un puntero al buffer en el espacio de usuario donde se copiarán los datos de la lista global.
   - Debe ser capaz de almacenar hasta `max_entries` elementos.

2. **`max_entries`** (`size_t`):
   - Número máximo de elementos que el buffer del usuario puede almacenar.

3. **`processes_returned`** (`int __user *`):
   - Puntero a un entero en el espacio de usuario donde se almacenará el número real de procesos copiados al buffer.

### **Retornos**
La syscall devuelve un valor entero para indicar el resultado de la operación:
- `0`: Operación exitosa.
- `-EFAULT`: Error al acceder al buffer del usuario o al copiar datos hacia él.
- `-ENOMEM`: No hay suficiente memoria en el kernel para el buffer temporal.

### **Estructura Usada**
```c
struct memory_limitation {
    pid_t pid;               // ID del proceso
    size_t memory_limit;     // Límite de memoria en bytes
    struct list_head list;   // Nodo para la lista enlazada
};
```

### **Funcionamiento**
1. **Validación del buffer del usuario:**
   - Se verifica que el puntero `user_buffer` sea accesible y pueda almacenar `max_entries` elementos usando `access_ok`.

2. **Asignación de memoria temporal:**
   - Se utiliza `kmalloc_array` para asignar memoria en el espacio del kernel que actúa como un buffer intermedio. Esta memoria será liberada al final de la syscall.

3. **Copiado de datos:**
   - Se recorre la lista global `memory_limited_processes` utilizando `list_for_each_entry`.
   - Por cada entrada en la lista, los valores de `PID` y `memory_limit` se copian al buffer temporal.
   - Se detiene el recorrido si se alcanza el límite de `max_entries`.

4. **Transferencia al espacio de usuario:**
   - Los datos del buffer temporal se copian al buffer proporcionado por el usuario (`user_buffer`) utilizando `copy_to_user`.
   - Se copia también el número total de procesos transferidos al puntero `processes_returned`.

5. **Liberación de memoria:**
   - La memoria asignada en el espacio del kernel se libera con `kfree`.

6. **Retorno:**
   - Si todo es exitoso, la syscall retorna `0`.
---

## **Syscall: _202000173_remove_memory_limit**

### **Descripción**
Esta syscall elimina el límite de memoria asignado a un proceso específico. Si el proceso no está en la lista de procesos limitados, devuelve un error indicando que no se encontró.

### **Prototipo**
```c
SYSCALL_DEFINE1(_202000173_remove_memory_limit, pid_t, process_pid);
```

### **Parámetros**
1. **`process_pid`** (`pid_t`):
   - El ID del proceso (PID) cuyo límite de memoria se desea eliminar.
   - Debe ser un número mayor que 0.

### **Retornos**
La syscall devuelve un valor entero para indicar el resultado de la operación:
- `0`: Operación exitosa, el proceso fue eliminado de la lista.
- `-EINVAL`: El PID proporcionado es inválido (valor no positivo).
- `-EPERM`: El usuario no tiene permisos de administrador (`CAP_SYS_ADMIN` requerido).
- `-ESRCH`: El proceso no fue encontrado en la lista de procesos limitados.

### **Estructura Usada**
```c
struct memory_limitation {
    pid_t pid;               // ID del proceso
    size_t memory_limit;     // Límite de memoria en bytes
    struct list_head list;   // Nodo para la lista enlazada
};
```

### **Funcionamiento**
1. **Validación del PID:**
   - Verifica que el `process_pid` sea mayor que 0. Si no lo es, devuelve `-EINVAL`.

2. **Validación de permisos:**
   - Verifica que el usuario tenga privilegios de administrador usando la función `capable(CAP_SYS_ADMIN)`. Si no tiene permisos, devuelve `-EPERM`.

3. **Búsqueda en la lista:**
   - Recorre la lista global `memory_limited_processes` utilizando `list_for_each_entry_safe`. Esta función es ideal porque permite eliminar elementos mientras se recorre la lista.

4. **Eliminación del proceso:**
   - Si encuentra una entrada con el `process_pid` proporcionado:
     - Se elimina el nodo de la lista con `list_del`.
     - Se libera la memoria asociada a la entrada utilizando `kfree`.
     - Devuelve `0` indicando éxito.

5. **Si no se encuentra el proceso:**
   - Devuelve `-ESRCH`, indicando que el proceso no fue encontrado en la lista.

---

## **Syscall: _202000173_update_memory_limit**

### **Descripción**
Esta syscall actualiza el límite de memoria de un proceso que ya está registrado en la lista global de procesos limitados. Si el proceso no se encuentra en la lista, se devuelve un error.

### **Prototipo**
```c
SYSCALL_DEFINE2(_202000173_update_memory_limit, pid_t, process_pid, size_t, memory_limit);
```

### **Parámetros**
1. **`process_pid`** (`pid_t`):
   - El ID del proceso (PID) cuyo límite de memoria se desea actualizar.
   - Debe ser un número mayor que 0.

2. **`memory_limit`** (`size_t`):
   - El nuevo límite de memoria en bytes que se desea asignar al proceso.
   - Debe ser un valor mayor que 0.

### **Retornos**
La syscall devuelve un valor entero para indicar el resultado de la operación:
- `0`: Operación exitosa, el límite de memoria del proceso fue actualizado.
- `-EINVAL`: El PID o el nuevo límite de memoria es inválido (valores no positivos).
- `-EPERM`: El usuario no tiene permisos de administrador (`CAP_SYS_ADMIN` requerido).
- `-ESRCH`: El proceso no fue encontrado en la lista de procesos limitados.

### **Estructura Usada**
```c
struct memory_limitation {
    pid_t pid;               // ID del proceso
    size_t memory_limit;     // Límite de memoria en bytes
    struct list_head list;   // Nodo para la lista enlazada
};
```

### **Funcionamiento**
1. **Validación de Parámetros:**
   - Verifica que el `process_pid` sea mayor que 0.
   - Verifica que el `memory_limit` sea mayor que 0.
   - Si alguno de estos valores no es válido, se devuelve `-EINVAL`.

2. **Validación de Permisos:**
   - Verifica que el usuario tenga privilegios de administrador utilizando `capable(CAP_SYS_ADMIN)`. Si no los tiene, se devuelve `-EPERM`.

3. **Búsqueda en la Lista:**
   - Recorre la lista global `memory_limited_processes` utilizando `list_for_each_entry`.
   - Si encuentra una entrada con el `process_pid` proporcionado, actualiza el campo `memory_limit` de esa entrada y devuelve `0`.

4. **Proceso No Encontrado:**
   - Si después de recorrer toda la lista no se encuentra el proceso, se devuelve `-ESRCH`.

---

## **Conclusión**
La implementación de las syscalls para gestionar límites de memoria en procesos permite un control eficiente de recursos en el kernel. El uso de estructuras dinámicas como listas enlazadas facilita la manipulación de los procesos limitados.

## **Reflexión**
Este proyecto refuerza la importancia de comprender la interacción entre el espacio de usuario y el kernel, así como la necesidad de implementar controles robustos para garantizar la seguridad y estabilidad del sistema. La experiencia también subraya el valor del manejo eficiente de errores y validaciones.

## **Comentario personal**
Esta implementación de las syscalls fue un proceso de aprendizaje intensivo y creativo. Después de trabajar en ello durante varios días, he aprendido a trabajar con lista enlazada, manejar enlaces y a utilizar las herramientas de desarrollo de Linux para del desarrollo de limitantes de memoria.

## **Resolución de problemas**

#### **1. Error `-EINVAL` en Parámetros Válidos**
- La validación inicial de parámetros no maneja correctamente los valores de entrada.
- **Solución:**
  - Validación de parámetros en cada syscall.
    ```c
    if (process_pid <= 0 || memory_limit <= 0) {
        return -EINVAL; // PID o límite inválido
    }
    ```

#### **2. Error `-EPERM` por Falta de Permisos**
- El programa no se ejecuta con privilegios de administrador.
- **Solución:**
  - Ejecutar el programa como superusuario (`sudo`):
    ```bash
    sudo ./1_admin_syscalls
    sudo ./2_allocator
    sudo ./3_error_handling
    ```
  - Luego en el kernel se valida que se ejecute correctamente ya con permisos:
    ```c
    if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
    }
    ```

#### **3. Error `-EFAULT` al Copiar Datos al Usuario**
- Las syscalls que copian datos al espacio de usuario fallan con `-EFAULT`. El puntero del usuario (`user_buffer`) es inválido o inaccesible.
- **Solución:**
  - Se validó el acceso al buffer del usuario antes de realizar la operación:
    ```c
    if (!access_ok(user_buffer, max_entries * sizeof(struct memory_limitation))) {
        return -EFAULT;
    }
    ```
  - Usamos `copy_to_user` para transferir datos al espacio de usuario y verifica posibles errores:
    ```c
    if (copy_to_user(user_buffer, buffer, count * sizeof(struct memory_limitation))) {
        return -EFAULT;
    }
    ```
#### **Fallo del kernel con kmalloc**
- Al intentar asignar una región contigua de memoria personalizada, ya que al asignar no se estaba manejando de manera correcta el error de asignación.
- **Solucion:**
  - Se validó que la memoria sea suficiente antes de intentar reservar:
    ```c
    if (size > PAGE_SIZE) {
      return -ENOMEM;
    }
    ```
   - Esto permitió que la asignación de la memoria fuera eficaz y estar evitando que el kernel entrara en panic en algunos casos.
