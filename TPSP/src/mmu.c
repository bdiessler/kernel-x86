/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================

  Definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "i386.h"

#include "kassert.h"

static pd_entry_t* kpd = (pd_entry_t*)KERNEL_PAGE_DIR;
static pt_entry_t* kpt = (pt_entry_t*)KERNEL_PAGE_TABLE_0;

static const uint32_t identity_mapping_end = 0x003FFFFF;
static const uint32_t user_memory_pool_end = 0x02FFFFFF;

static paddr_t next_free_kernel_page = 0x100000;
static paddr_t next_free_user_page = 0x400000;

/**
 * kmemset asigna el valor c a un rango de memoria interpretado
 * como un rango de bytes de largo n que comienza en s
 * @param s es el puntero al comienzo del rango de memoria
 * @param c es el valor a asignar en cada byte de s[0..n-1]
 * @param n es el tamaño en bytes a asignar
 * @return devuelve el puntero al rango modificado (alias de s)
*/
static inline void* kmemset(void* s, int c, size_t n) {
  uint8_t* dst = (uint8_t*)s;
  for (size_t i = 0; i < n; i++) {
    dst[i] = c;
  }
  return dst;
}

/**
 * zero_page limpia el contenido de una página que comienza en addr
 * @param addr es la dirección del comienzo de la página a limpiar
*/
static inline void zero_page(paddr_t addr) {
  kmemset((void*)addr, 0x00, PAGE_SIZE);
}


void mmu_init(void) {}


/**
 * mmu_next_free_kernel_page devuelve la dirección física de la próxima página de kernel disponible. 
 * Las páginas se obtienen en forma incremental, siendo la primera: next_free_kernel_page
 * @return devuelve la dirección de memoria de comienzo de la próxima página libre de kernel
 */
 // -------------------------------------------------------------------
paddr_t mmu_next_free_kernel_page(void) {
  //Aca se obtiene la direccion fisica de la pagina libre de kernel
  paddr_t address = next_free_kernel_page;
  //Aca se incrementa la direccion fisica de la pagina libre de kernel
  next_free_kernel_page += PAGE_SIZE;
  return address;
}
 // -------------------------------------------------------------------

/**
 * mmu_next_free_user_page devuelve la dirección de la próxima página de usuarix disponible
 * @return devuelve la dirección de memoria de comienzo de la próxima página libre de usuarix
 */
 // -------------------------------------------------------------------
paddr_t mmu_next_free_user_page(void) {
  //Aca se obtiene la direccion fisica de la pagina libre de usuario
  paddr_t address = next_free_user_page;
  //Aca se incrementa la direccion fisica de la pagina libre de usuario
  next_free_user_page += PAGE_SIZE;
  return address;
}
 // -------------------------------------------------------------------
/**
 * mmu_init_kernel_dir inicializa las estructuras de paginación vinculadas al kernel y
 * realiza el identity mapping
 * @return devuelve la dirección de memoria de la página donde se encuentra el directorio
 * de páginas usado por el kernel
 */
paddr_t mmu_init_kernel_dir(void) {
  // Necesitamos agarrar un puntero a la parte de memoria donde van los directorios/ las tablas de pagina
  // ahi vamos a ciclar 1024 veces para llenar cada entry de pagina en la tabla,
  // la base va a ser igual a la direccion fisica... ni idea
  // Tendriamos que mapear las direcciones desde 0x0 hasta 0x3FFFFF y no deberiamos limpiar lo que esta usandose
  // porque vamos a mapear cosas que ya deberian estar escritas (codigo del kernel) o estamos escribiendo (como las tablas)
  // Además, tenemos que escribir los directorios/las tablas en las direcciones 0x25000 y 0x26000 respectivamente
  zero_page(KERNEL_PAGE_DIR);
  zero_page(KERNEL_PAGE_TABLE_0);
  paddr_t *page_dir = (paddr_t*)KERNEL_PAGE_DIR;
  *page_dir = KERNEL_PAGE_TABLE_0 | MMU_P | MMU_W;
  paddr_t obj_dir = 0;
  for (page_dir = (paddr_t*)KERNEL_PAGE_TABLE_0; page_dir < (paddr_t*)0x27000; page_dir++)
  {
    *page_dir = obj_dir | MMU_P | MMU_W; // algo = 0 p/ el primero y algo = p*4096 para el resto con p numero de pagina
    obj_dir += PAGE_SIZE;
  }
  return KERNEL_PAGE_DIR;
}

/**
 * mmu_map_page agrega las entradas necesarias a las estructuras de paginación de modo de que
 * la dirección virtual virt se traduzca en la dirección física phy con los atributos definidos en attrs
 * @param cr3 el contenido que se ha de cargar en un registro CR3 al realizar la traducción
 * @param virt la dirección virtual que se ha de traducir en phy
 * @param phy la dirección física que debe ser accedida (dirección de destino)
 * @param attrs los atributos a asignar en la entrada de la tabla de páginas
 */
void mmu_map_page(uint32_t cr3, vaddr_t virt, paddr_t phy, uint32_t attrs) {
  pd_entry_t *pd_base = (pd_entry_t *)CR3_TO_PAGE_DIR(cr3);
  pt_entry_t *pt_base;
  uint32_t i = (uint32_t)VIRT_PAGE_DIR(virt);
  if ((pd_base[i].attrs & MMU_P) == 1){ //Aca me fijo si ya esta presente en el PD
    pt_base = (pt_entry_t*)MMU_ENTRY_PADDR(pd_base[i].pt); // Si ya esta presente, agarro la tabla de paginas que ya estaba mapeada
    i = (uint32_t)VIRT_PAGE_TABLE(virt); 
    if ((pt_base[i].attrs & MMU_P) == 1){ //Aca me fijo si ya esta presente en la PT
      pt_base[i].page = ((phy & 0xFFFFF000) >> 12); // Si ya estaba presente, actualizo la pagina
      pt_base[i].attrs = attrs | MMU_P; // y actualizo los atributos
    } else
    {
      pt_base[i].page = ((phy & 0xFFFFF000) >> 12) & 0xFFFFF; // Si no estaba presente, asigno la pagina
      pt_base[i].attrs = attrs | MMU_P; // y los atributos
    }
    
  } else
  { // Si no estaba presente, tengo que crear una nueva entrada en el PD
    pd_base[i].pt = (mmu_next_free_kernel_page() >> 12) & 0xFFFFF; // Consigo una nueva pagina para la tabla de paginas
    pd_base[i].attrs = MMU_U | MMU_W | MMU_P; // y le asigno los atributos correspondientes
    pt_base = (pt_entry_t*)MMU_ENTRY_PADDR(pd_base[i].pt); // obtengo la dir fisica de la tabla de paginas
    zero_page(pt_base); // limpio la tabla de paginas
    i = VIRT_PAGE_TABLE(virt);
    pt_base[i].attrs = attrs | MMU_P; // asigno los atributos a la entrada de la tabla de paginas
    pt_base[i].page = ((phy & 0xFFFFF000) >> 12) & 0xFFFFF; // asigno la pagina fisica a la entrada de la tabla de paginas
    
  }
  tlbflush(); // Limpiamos la TLB para que tome los cambios
}

/**
 * mmu_unmap_page elimina la entrada vinculada a la dirección virt en la tabla de páginas correspondiente
 * @param virt la dirección virtual que se ha de desvincular
 * @return la dirección física de la página desvinculada
 */
paddr_t mmu_unmap_page(uint32_t cr3, vaddr_t virt) {
  pd_entry_t *pd_base = (pd_entry_t*)CR3_TO_PAGE_DIR(cr3); // Obtenemos el PD desde el CR3
  pt_entry_t *pt_base = (pt_entry_t*)pd_base[VIRT_PAGE_DIR(virt)].pt; // Accedemos a la PT
  paddr_t addr = (paddr_t)(MMU_ENTRY_PADDR(pt_base[VIRT_PAGE_TABLE(virt)].page)); // Encontramos la direccion fisica que queremos desmappear
  pt_base[VIRT_PAGE_TABLE(virt)].attrs = 0; // desasignamos los atributos
  pt_base[VIRT_PAGE_TABLE(virt)].page = 0; // desasignamos la pagina
  tlbflush(); // Limpiamos la TLB para que tome los cambios
  return addr;
}

#define DST_VIRT_PAGE 0xA00000
#define SRC_VIRT_PAGE 0xB00000

/**
 * copy_page copia el contenido de la página física localizada en la dirección src_addr a la página física ubicada en dst_addr
 * @param dst_addr la dirección a cuya página queremos copiar el contenido
 * @param src_addr la dirección de la página cuyo contenido queremos copiar
 *
 * Esta función mapea ambas páginas a las direcciones SRC_VIRT_PAGE y DST_VIRT_PAGE, respectivamente, realiza
 * la copia y luego desmapea las páginas. Usar la función rcr3 definida en i386.h para obtener el cr3 actual
 */
void copy_page(paddr_t dst_addr, paddr_t src_addr) {
  //rcr3()
  mmu_map_page(rcr3(), (vaddr_t)DST_VIRT_PAGE, dst_addr, MMU_W | MMU_P); // Mapeamos la pagina de destino
  mmu_map_page(rcr3(), (vaddr_t)SRC_VIRT_PAGE, src_addr, MMU_W | MMU_P); // Mapeamos la pagina fuente

  uint32_t *dst = DST_VIRT_PAGE; // puntero a pagina destino
  uint32_t *src = SRC_VIRT_PAGE; // puntero a pagina fuente
  for (uint32_t i = 0; i < 1024; i++) // Recorremos las 1024 entradas de la pagina
  {
    dst[i] = src[i]; // Copiamos el contenido de la pagina fuente a la pagina destino
  }
  
  mmu_unmap_page(rcr3(), (vaddr_t)DST_VIRT_PAGE); //desmapeamos la pagina destino
  mmu_unmap_page(rcr3(), (vaddr_t)SRC_VIRT_PAGE); //desmapeamos la pagina fuente
}

void test_copy(){
  vaddr_t *virt = 0x400000; // Probar con otras direcciones
  paddr_t datos1 = mmu_next_free_user_page();
  mmu_map_page(rcr3(), (vaddr_t)virt, datos1, MMU_W | MMU_U | MMU_P);
  //char *s = "cosas escritas, etc";
  int i = 1234;
  *virt = i;
  
  
  vaddr_t *virt2 = 0x401000;
  paddr_t datos2 = mmu_next_free_user_page();
  mmu_map_page(rcr3(), (vaddr_t)virt2, datos2, MMU_W | MMU_U | MMU_P);

  copy_page(datos2, datos1);

  print((char*)virt, 0,0, C_FG_WHITE | C_BG_BLACK);
  print((char*)virt2, 0,0, C_FG_WHITE | C_BG_BLACK);

}
 /**
 * mmu_init_task_dir inicializa las estructuras de paginación vinculadas a una tarea cuyo código se encuentra en la dirección phy_start
 * @param phy_start es la dirección donde comienzan las dos páginas de código de la tarea asociada a esta llamada
 * @return el contenido que se ha de cargar en un registro CR3 para la tarea asociada a esta llamada
 */
paddr_t mmu_init_task_dir(paddr_t phy_start) {
  paddr_t cr3_task = mmu_next_free_kernel_page(); // Consigo una página para el PD de mi tarea, es al mismo tiempo el cr3 que tengo que devolver
  paddr_t pt_dir = mmu_next_free_kernel_page(); // página nueva donde guardo la tabla de páginas del identity mapping
  zero_page(cr3_task); // limpio el pd por las dudas
  zero_page(pt_dir); // limpio el pt por las dudas
  pt_entry_t *page_dir = (pt_entry_t*)cr3_task; // puntero a la tabla de páginas del directorio de páginas de la tarea
  page_dir[0].page = (pt_dir >> 12); // nos quedamos con los 20 bits de la direccion fisica del directorio de paginas
  page_dir[0].attrs = MMU_P | MMU_W; // le asignamos los atributos
  page_dir = (pt_entry_t*)pt_dir; // puntero a la tabla de paginas
  paddr_t obj_dir = 0; // variable para recorrer la tabla de paginas desde el incio
  for (uint32_t i = 0; i < 1024; i++) // 1024 pq son 4 bytes cada iteracion
  {
    page_dir[i].page = (obj_dir >> 12); // nos quedamos con los 20 bits de la direccion fisica de la pagina
    page_dir[i].attrs = MMU_P | MMU_W; // le asignamos los atributos
    obj_dir += PAGE_SIZE; //pasas a la siguiente pagina
  }
  
  // Mapeo las direcciones del código que me dan

 // Aca cr3, donde quiero mapear el codigo de la tarea, la direccion fisica donde esta el codigo de la tarea, los atributos
  mmu_map_page(cr3_task, TASK_CODE_VIRTUAL, phy_start, MMU_U | MMU_P);
  
  //Aca seteamos la siguiente pagina, nos piden en el enunciado que sean 2
  mmu_map_page(cr3_task, TASK_CODE_VIRTUAL + PAGE_SIZE, phy_start + PAGE_SIZE, MMU_U | MMU_P); 

  // Mapeo la pila
  //Aca cr3, donde empieza la pila (direccion mas alta) - page size para que entre toda la pagina en la pila cuando crezca, pagina de usuario porque las tareas son de usuario, atributos de escritura usuario y presente
  mmu_map_page(cr3_task, TASK_STACK_BASE - 0x1000, mmu_next_free_user_page(), MMU_W | MMU_U | MMU_P);

  // Mapeo la memoria compartida
  // Aca cr3, donde quiero mapear la pagina compartida, direccion fisica de la pagina compartida, atributos de escritura usuario y presente
  mmu_map_page(cr3_task, TASK_SHARED_PAGE, 0x1D000, MMU_U | MMU_P);

  return cr3_task;
}

// COMPLETAR: devuelve true si se atendió el page fault y puede continuar la ejecución 
// y false si no se pudo atender
bool page_fault_handler(vaddr_t virt) {
  print("Atendiendo page fault...", 0, 0, C_FG_WHITE | C_BG_BLACK);
  // Chequeemos si el acceso fue dentro del area on-demand
  if (virt >= ON_DEMAND_MEM_START_VIRTUAL && virt <= ON_DEMAND_MEM_END_VIRTUAL)
  {// En caso de que si, mapear la pagina
    mmu_map_page(rcr3(), virt, ON_DEMAND_MEM_START_PHYSICAL, MMU_P | MMU_W | MMU_U);
    return true;
  }
  
  return false;
}
