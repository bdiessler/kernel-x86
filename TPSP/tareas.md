# System Programming: Tareas.

Vamos a continuar trabajando con el kernel que estuvimos programando en
los talleres anteriores. La idea es incorporar la posibilidad de
ejecutar algunas tareas específicas. Para esto vamos a precisar:

-   Definir las estructuras de las tareas disponibles para ser
    ejecutadas

-   Tener un scheduler que determine la tarea a la que le toca
    ejecutase en un período de tiempo, y el mecanismo para el
    intercambio de tareas de la CPU

-   Iniciar el kernel con una *tarea inicial* y tener una *tarea idle*
    para cuando no haya tareas en ejecución

Recordamos el mapeo de memoria con el que venimos trabajando. Las tareas
que vamos a crear en este taller van a ser parte de esta organización de
la memoria:

![](img/mapa_fisico.png)

![](img/mapeo_detallado.png)

## Archivos provistos

A continuación les pasamos la lista de archivos que forman parte del
taller de hoy junto con su descripción:

-   **Makefile** - encargado de compilar y generar la imagen del
    floppy disk.

-   **idle.asm** - código de la tarea Idle.

-   **shared.h** -- estructura de la página de memoria compartida

-   **tareas/syscall.h** - interfaz para realizar llamadas al sistema
    desde las tareas

-   **tareas/task_lib.h** - Biblioteca con funciones útiles para las
    tareas

-   **tareas/task_prelude.asm**- Código de inicialización para las
    tareas

-   **tareas/taskPong.c** -- código de la tarea que usaremos
    (**tareas/taskGameOfLife.c, tareas/taskSnake.c,
    tareas/taskTipear.c **- código de otras tareas de ejemplo)

-   **tareas/taskPongScoreboard.c** -- código de la tarea que deberán
    completar

-   **tss.h, tss.c** - definición de estructuras y funciones para el
    manejo de las TSSs

-   **sched.h, sched.c** - scheduler del kernel

-   **tasks.h, tasks.c** - Definición de estructuras y funciones para
    la administración de tareas

-   **isr.asm** - Handlers de excepciones y interrupciones (en este
    caso se proveen las rutinas de atención de interrupciones)

-   **task\_defines.h** - Definiciones generales referente a tareas

## Ejercicios

### Primera parte: Inicialización de tareas

**1.** Si queremos definir un sistema que utilice sólo dos tareas, ¿Qué
nuevas estructuras, cantidad de nuevas entradas en las estructuras ya
definidas, y registros tenemos que configurar?¿Qué formato tienen?
¿Dónde se encuentran almacenadas?
```

Las nuevas estructuras son el Task Register, el Task State Segment y vamos a
volver a usar los descriptores de la GDT para apuntar a las TSS con sus
selectores también para ingresar a los índices de la GDT correspondientes. Las
TSS se encuentran almacenadas en el segmento de memoria del kernel, el TR es un
registro similar a GDTR o IDTR.
```


**2.** ¿A qué llamamos cambio de contexto? ¿Cuándo se produce? ¿Qué efecto
tiene sobre los registros del procesador? Expliquen en sus palabras que
almacena el registro **TR** y cómo obtiene la información necesaria para
ejecutar una tarea después de un cambio de contexto.
```

El cambio de contexto es el cambio de una tarea a otra con el far jump y el
selector de segmento de la próxima tarea, se guardan los registros de la tarea
actual en la TSS y se restauran los de la siguiente. Se produce cuando el
scheduler decida, puede ser que se haya cumplido el tiempo asignado o cualquier
otro algoritmo que implemente el scheduler.
El TR es la forma de recuperar el TSS de la tarea actual, es el registro que
guarda el selector de segmento que nos dirige al descriptor de la GDT que
buscamos, por ej, al guardar los registros en un cambio de contexto.
```


**3.** Al momento de realizar un cambio de contexto el procesador va
almacenar el estado actual de acuerdo al selector indicado en el
registro **TR** y ha de restaurar aquel almacenado en la TSS cuyo
selector se asigna en el *jmp* far. ¿Qué consideraciones deberíamos
tener para poder realizar el primer cambio de contexto? ¿Y cuáles cuando
no tenemos tareas que ejecutar o se encuentran todas suspendidas?
```

Para el primer cambio de contexto hay que cargar la Tarea Inicial, usamos la
operacion LTR que toma como parametro el selector de la tarea en la GDT, luego
hay que cargar la Tarea Idle saltando al selector con JMP SELECTOR_TAREA_IDLE:0
(el offset se ignora y puede ser 0)
Cuando no hay tarea que ejecutar, se ejecuta la idle.
```

**4.** ¿Qué hace el scheduler de un Sistema Operativo? ¿A qué nos
referimos con que usa una política?
```

El scheduler administra las tareas, define cuándo y bajo qué condiciones se
divide el tiempo dedicado a cada tarea y se encarga de hacer el context switch.
Que use uan politica se refiere a que hay un algoritmo que sirve para definir
los criterios para cuando llamar al context switch segun una serie de variables.
```


**5.** En un sistema de una única CPU, ¿cómo se hace para que los
programas parezcan ejecutarse en simultáneo?
```

Se intercala el procesamiento de las tareas muy rápido.
```

**6.** En **tss.c** se encuentran definidas las TSSs de la Tarea
**Inicial** e **Idle**. Ahora, vamos a agregar el *TSS Descriptor*
correspondiente a estas tareas en la **GDT**.
    
a) Observen qué hace el método: ***tss_gdt_entry_for_task***

b) Escriban el código del método ***tss_init*** de **tss.c** que
agrega dos nuevas entradas a la **GDT** correspondientes al
descriptor de TSS de la tarea Inicial e Idle.

c) En **kernel.asm**, luego de habilitar paginación, agreguen una
llamada a **tss_init** para que efectivamente estas entradas se
agreguen a la **GDT**.

d) Correr el *qemu* y usar **info gdt** para verificar que los
***descriptores de tss*** de la tarea Inicial e Idle esten
efectivamente cargadas en la GDT

**7.** Como vimos, la primer tarea que va a ejecutar el procesador
cuando arranque va a ser la **tarea Inicial**. Se encuentra definida en
**tss.c** y tiene todos sus campos en 0. Antes de que comience a ciclar
infinitamente, completen lo necesario en **kernel.asm** para que cargue
la tarea inicial. Recuerden que la primera vez tenemos que cargar el registro
**TR** (Task Register) con la instrucción **LTR**.
Previamente llamar a la función tasks_screen_draw provista para preparar
la pantalla para nuestras tareas.

Si obtienen un error, asegurense de haber proporcionado un selector de
segmento para la tarea inicial. Un selector de segmento no es sólo el
indice en la GDT sino que tiene algunos bits con privilegios y el *table
indicator*.

**8.** Una vez que el procesador comenzó su ejecución en la **tarea Inicial**, 
le vamos a pedir que salte a la **tarea Idle** con un
***JMP***. Para eso, completar en **kernel.asm** el código necesario
para saltar intercambiando **TSS**, entre la tarea inicial y la tarea
Idle.

**9.** Utilizando **info tss**, verifiquen el valor del **TR**.
También, verifiquen los valores de los registros **CR3** con **creg** y de los registros de segmento **CS,** **DS**, **SS** con
***sreg***. ¿Por qué hace falta tener definida la pila de nivel 0 en la
tss?
```

Si hay una interrupción que produzca un cambio de privilegio el procesador va a
tener que buscar la base de la pila nivel 0 para usarla en lugar de la de nivel
3, que no le corresponde.  
```

**10.** En **tss.c**, completar la función ***tss_create_user_task***
para que inicialice una TSS con los datos correspondientes a una tarea
cualquiera. La función recibe por parámetro la dirección del código de
una tarea y será utilizada más adelante para crear tareas.

Las direcciones físicas del código de las tareas se encuentran en
**defines.h** bajo los nombres ***TASK_A_CODE_START*** y
***TASK_B_CODE_START***.

El esquema de paginación a utilizar es el que hicimos durante la clase
anterior. Tener en cuenta que cada tarea utilizará una pila distinta de
nivel 0.

### Segunda parte: Poniendo todo en marcha

**11.** Estando definidas **sched_task_offset** y **sched_task_selector**:
```
  sched_task_offset: dd 0xFFFFFFFF
  sched_task_selector: dw 0xFFFF
```

Y siendo la siguiente una implementación de una interrupción del reloj:

```
global _isr32
  
global _isr32
  
_isr32:
  pushad  ; Pushea todos los registros
  call pic_finish1  ; Avisa al pic que recibió la interrupción
  
  call sched_next_task  ; llama al scheduler para la proxima tarea y en ax pone el selector de segmento de la proxima tarea a ejecutar
  
  str cx  ; Lee el registro TR y lo guarda en cx
  cmp ax, cx  ; Compara el selector de sched_next_task y el de TR
  je .fin  ; si son iguales, salta a .fin y no cambia de tarea
  
  mov word [sched_task_selector], ax  ; Mueve a [sched_task_selector], que es la posicion de memoria reservada para el selector, el ax (selector de tarea siguiente)
  jmp far [sched_task_offset]  ; es un salto al selector de la próxima tarea correspondiente a ese selector en la GDT y produce un cambio de contexto
  
  .fin:
  popad ; restaura los registros
  iret  ; retorna a la ubicación previa a la interrupción

```
a)  Expliquen con sus palabras que se estaría ejecutando en cada tic
    del reloj línea por línea

b)  En la línea que dice ***jmp far \[sched_task_offset\]*** ¿De que
    tamaño es el dato que estaría leyendo desde la memoria? ¿Qué
    indica cada uno de estos valores? ¿Tiene algún efecto el offset
    elegido?
```

El tamaño del dato leído en el jump es de 48 bits, 32 de offset y 16 de
selector. El offset en este caso se ignora porque es un cambio de tarea y no nos
importa el offset que se defina en el salto, sino el que está guardado en la
pila dentro de la interrupción.
```

c)  ¿A dónde regresa la ejecución (***eip***) de una tarea cuando
    vuelve a ser puesta en ejecución?
```

Vuelve a la posición donde se interrumpió la tarea actual.
```



**12.** Para este Taller la cátedra ha creado un scheduler que devuelve
la próxima tarea a ejecutar.

a)  En los archivos **sched.c** y **sched.h** se encuentran definidos
    los métodos necesarios para el Scheduler. Expliquen cómo funciona
    el mismo, es decir, cómo decide cuál es la próxima tarea a
    ejecutar. Pueden encontrarlo en la función ***sched_next_task***.
```

sched_next_task recorre la lista de tareas hasta encontrar una que esté
disponible para ejecutarse o que sea la misma que está siendo ejecutada
actualmente. Cuando la encuentra, la define como la actual y devuelve el
selector de la GDT para la TSS de esa tarea.
```

b)  Modifiquen **kernel.asm** para llamar a la función
    ***sched_init*** luego de iniciar la TSS

c)  Compilen, ejecuten ***qemu*** y vean que todo sigue funcionando
    correctamente.

### Tercera parte: Tareas? Qué es eso?

**14.** Como parte de la inicialización del kernel, en kernel.asm se
pide agregar una llamada a la función **tasks\_init** de
**task.c** que a su vez llama a **create_task**. Observe las
siguientes líneas:
```C
int8_t task_id = sched_add_task(gdt_id << 3);

tss_tasks[task_id] = tss_create_user_task(task_code_start[tipo]);

gdt[gdt_id] = tss_gdt_entry_for_task(&tss_tasks[task_id]);
```
a)  ¿Qué está haciendo la función ***tss_gdt_entry_for_task***?
```

La función inicializa el descriptor de segmento de la tss dada como parámetro y
lo retorna
```

b)  ¿Por qué motivo se realiza el desplazamiento a izquierda de
    **gdt_id** al pasarlo como parámetro de ***sched_add_task***?
```

Se desplaza 3 bits a izquierda porque gdt_id es el índice de 13 bits de la gdt
para la tarea y para convertirlo en selector hace falta añadir los 3 bits menos
significativos de parámetros, que son todos cero
```

**15.** Ejecuten las tareas en *qemu* y observen el código de estas
superficialmente.

a) ¿Qué mecanismos usan para comunicarse con el kernel?
```

Se comunican con el kernel a través de syscalls, en particular la única función
que necesitan para dibujar en pantalla es syscall_draw.
```

b) ¿Por qué creen que no hay uso de variables globales? ¿Qué pasaría si
    una tarea intentase escribir en su `.data` con nuestro sistema?
```

En principio, no tenemos definido en la memoria de la tarea un lugar donde se
van a guardar variables globales. Si estas estuviesen por algún motivo en las
mismas páginas que el código, entonces una tarea no tendría permisos de
escritura y no podría modificar esas variables. Entonces, si intentamos escribir
en .data no habría un lugar donde guardar esas variables o no podrían escribirse.
```

c) Cambien el divisor del PIT para \"acelerar\" la ejecución de las tareas:
```
    ; El PIT (Programmable Interrupt Timer) corre a 1193182Hz.

    ; Cada iteracion del clock decrementa un contador interno, cuando
    éste llega

    ; a cero se emite la interrupción. El valor inicial es 0x0 que
    indica 65536,

    ; es decir 18.206 Hz

    mov ax, DIVISOR

    out 0x40, al

    rol ax, 8

    out 0x40, al
```

**16.** Observen **tareas/task_prelude.asm**. El código de este archivo
se ubica al principio de las tareas.

a. ¿Por qué la tarea termina en un loop infinito?
```

Porque el procesador no puede quedarse sin hacer nada, cuando una tarea esta
corriendo tiene que tener algo que hacer o sino esta comenzara a leer basura
como código. También es porque no hay forma de finalizar una tarea, sacarla del
scheduler, desalojarla de memoria, etc, en este tp.
```

b. \[Opcional\] ¿Qué podríamos hacer para que esto no sea necesario?

Modificar el scheduler para permitir el desalojo de tareas.

### Cuarta parte: Hacer nuestra propia tarea

Ahora programaremos nuestra tarea. La idea es disponer de una tarea que
imprima el *score* (puntaje) de todos los *Pongs* que se están
ejecutando. Para ello utilizaremos la memoria mapeada *on demand* del
taller anterior.

#### Análisis:

**18.** Analicen el *Makefile* provisto. ¿Por qué se definen 2 "tipos"
de tareas? ¿Como harían para ejecutar una tarea distinta? Cambien la
tarea S*nake* por una tarea *PongScoreboard*.
```

Se definen dos tipos de tareas porque el sistema está limitado para poder
ejecutar 2 programas distintos, aunque puedan ejecutarse 4 tareas, tendrían que
compartir código. Modificamos el makefile y en lugar de que ponga el Snake
ejecute PongScoreboard.
```

**19.** Mirando la tarea *Pong*, ¿En que posición de memoria escribe
esta tarea el puntaje que queremos imprimir? ¿Cómo funciona el mecanismo
propuesto para compartir datos entre tareas?
```

Escribe en la posición de memoria designada como On demand, que está orientada a
ser compartida por las tareas. Define la posición en que debe escribir el score
según la id de la tarea y la idea es que otra tarea puede acceder a esa misma
parte de memoria y leerlo/escribirlo para poder así interactuar de algún modo
entre tareas.
```

#### Programando:

**20.** Completen el código de la tarea *PongScoreboard* para que
imprima en la pantalla el puntaje de todas las instancias de *Pong* usando los datos que nos dejan en la página compartida.

**21.** \[Opcional\] Resuman con su equipo todas las estructuras vistas
desde el Taller 1 al Taller 4. Escriban el funcionamiento general de
segmentos, interrupciones, paginación y tareas en los procesadores
Intel. ¿Cómo interactúan las estructuras? ¿Qué configuraciones son
fundamentales a realizar? ¿Cómo son los niveles de privilegio y acceso a
las estructuras?

**22.** \[Opcional\] ¿Qué pasa cuando una tarea dispara una
excepción? ¿Cómo podría mejorarse la respuesta del sistema ante estos
eventos?
```

Cuando una tarea tiene una excepción entra en un loop infinito. Para mejorarse
podría atajarse el loop infinito creando una interrupción que termine con la
tarea y pase a la siguiente.
```
