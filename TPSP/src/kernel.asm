; ** por compatibilidad se omiten tildes **
; ==============================================================================
; TALLER System Programming - Arquitectura y Organizacion de Computadoras - FCEN
; ==============================================================================

%include "print.mac"

global start


; COMPLETAR - Agreguen declaraciones extern según vayan necesitando
extern GDT_DESC
extern screen_draw_layout
extern IDT_DESC
extern idt_init
extern pic_reset
extern pic_enable
extern mmu_init_kernel_dir
extern test_copy
extern mmu_init_task_dir
extern tss_init
extern tasks_screen_draw
extern sched_init
extern tasks_init

on_demand: dd 0x07000000
; COMPLETAR - Definan correctamente estas constantes cuando las necesiten
%define CS_RING_0_SEL 0x0008
%define DS_RING_0_SEL 0x0018
%define SELECTOR_TAREA_INICIAL 11 << 3
;0x58
%define SELECTOR_TAREA_IDLE 12 << 3
;0x60


BITS 16
;; Saltear seccion de datos
jmp start

;;
;; Seccion de datos.
;; -------------------------------------------------------------------------- ;;
start_rm_msg db     'Iniciando kernel en Modo Real'
start_rm_len equ    $ - start_rm_msg

start_pm_msg db     'Iniciando kernel en Modo Protegido'
start_pm_len equ    $ - start_pm_msg

;;
;; Seccion de código.
;; -------------------------------------------------------------------------- ;;

;; Punto de entrada del kernel.
BITS 16
start:
    ; COMPLETAR - Deshabilitar interrupciones
    cli

    ; Cambiar modo de video a 80 X 50
    mov ax, 0003h
    int 10h ; set mode 03h
    xor bx, bx
    mov ax, 1112h
    int 10h ; load 8x8 font

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO REAL
    ; (revisar las funciones definidas en print.mac y los mensajes se encuentran en la
    ; sección de datos)
    print_text_rm start_rm_msg, start_rm_len, 0x6F, 40, 25

    ; COMPLETAR - Habilitar A20
    ; (revisar las funciones definidas en a20.asm)
    call A20_enable


    ; COMPLETAR - Cargar la GDT
    lgdt [GDT_DESC]

    ; COMPLETAR - Setear el bit PE del registro CR0
    mov EDI, CR0
    or EDI, 0x01
    mov CR0, EDI
    ; COMPLETAR - Saltar a modo protegido (far jump)
    ; (recuerden que un far jmp se especifica como jmp CS_selector:address)
    ; Pueden usar la constante CS_RING_0_SEL definida en este archivo
    jmp CS_RING_0_SEL:modo_protegido

BITS 32
modo_protegido:
    ; COMPLETAR - A partir de aca, todo el codigo se va a ejectutar en modo protegido
    ; Establecer selectores de segmentos DS, ES, GS, FS y SS en el segmento de datos de nivel 0
    ; Pueden usar la constante DS_RING_0_SEL definida en este archivo

    ;  Aca 'Todos se van a iniciar en el segmento de datos de nivel 0
    mov DI, DS_RING_0_SEL
    mov DS, DI
    mov ES, DI
    mov GS, DI
    mov FS, DI
    mov SS, DI
    ; COMPLETAR - Establecer el tope y la base de la pila
    ;  Aca 'se establece la pila en 0x25000' es una pila vacia asique el tope y la base son iguales
    mov EBP, 0x25000 ; EBP stack base pointer
    mov ESP, EBP ; ESP stack pointer
    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO PROTEGIDO
    print_text_pm start_pm_msg, start_pm_len, 0x0C, 40, 25
    ; COMPLETAR - Inicializar pantalla
    call screen_draw_layout
   
    ; Inicializar el directorio de paginas
    call mmu_init_kernel_dir
    ; Cargar directorio de paginas
    mov EDI, 0x25000
    mov CR3, EDI
    ; Habilitar paginacion
    mov EDI, CR0
    ; aca se modifica el bit 31 de cr0 que es PG, paging enable
    or EDI, 0x80000000
    mov CR0, EDI


    
    ; COMPLETAR - Inicializar y cargar la IDT
    lidt [IDT_DESC]
    call idt_init
    ; COMPLETAR - Reiniciar y habilitar el controlador de interrupciones
    call pic_reset
    call pic_enable

    ; Inicializar tss
    ;  aca cargamos la tss de la tarea inicial y la de idle
    call tss_init
    ; Inicializar el scheduler
    call sched_init
    ; Inicializar las tareas
    ; aca preparamos la pantalla para las tareas
    call tasks_screen_draw
    ; aca iniciamos las tareas
    call tasks_init
    
    ; Cargar tarea inicial
    ; 11 es el indice en la gdt
    ; aca selector es 11 << 3 ya que el selector tiene que dejar espacio para el TI y RPL
    ;  11|0|00
    mov ax, SELECTOR_TAREA_INICIAL
    ; cargamos el Task Register
    LTR ax
    ; COMPLETAR - Habilitar interrupciones
    ;sti
    ; NOTA: Pueden chequear que las interrupciones funcionen forzando a que se
    ;       dispare alguna excepción (lo más sencillo es usar la instrucción
    ;       `int3`)
    ;int3

    ; Probar Sys_call
    int 0x58
    ; Probar generar una excepción

    ;call test_copy

    ; Tanteando el reloj:
    mov ax, 11938
    out 0x40, al
    rol ax, 8
    out 0x40, al


    ; Guardando el CR3 actual
    ;mov EAX, CR3
    ;push EAX
    ; Inicializar el directorio de paginas de la tarea de prueba
    ;mov EBX, 0x18000
    ;push EBX
    ;call mmu_init_task_dir ; En EAX tengo los datos del CR3 de la "tarea"
    ;pop EBX ; restauro la pila
    ; Cargar directorio de paginas de la tarea
    ;mov CR3, EAX

    ; mov DWORD [0x07000000], 1
    ; mov DWORD [0x07000000], 0x1234
    ; mov DWORD [0x07000000], 0x5678
    ; Restaurar directorio de paginas del kernel
    ;pop EAX
    ;mov CR3, EAX

    ; Saltar a la primera tarea: Idle
    ; 12 es el indice en la gdt
    ; aca selector es 12 << 3 ya que el selector tiene que dejar espacio para el TI y RPL
    ;  12|0|00
    jmp SELECTOR_TAREA_IDLE:0

    ; Ciclar infinitamente 
    mov eax, 0xFFFF
    mov ebx, 0xFFFF
    mov ecx, 0xFFFF
    mov edx, 0xFFFF
    jmp $

;; -------------------------------------------------------------------------- ;;

%include "a20.asm"
