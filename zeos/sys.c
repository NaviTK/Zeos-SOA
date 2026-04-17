/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;
extern struct list_head freequeue;
extern struct list_head readyqueue;
extern int last_kernel;
extern int first_kernel;


int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

#define BUFF_SIZE 256
char buffer_sys[BUFF_SIZE];

int sys_write(int fd, char *buffer, int size) {
	// check de file descriptor y permisos
	int ret  = check_fd(fd,ESCRIPTURA);
	if (ret < 0) return ret; // devuelve -X si salimos con errores
	// check del buffer
	if(buffer == NULL) return -EFAULT;  /* Bad address */

	//check de size > 0 y que el puntero este en memoria de usuario, sino podria leer del kernel por ejemplo
  	if (size < 0 || !access_ok(VERIFY_READ, buffer, size)) return -EINVAL;   /* Invalid argument */

	int porEscribir = size; // bits que faltan por escribir
	char * bufferAux = buffer; // puntero auxiliar para recorrer el buffer sin mover el puntero original
	// como no sabemos si el tamaño del buffer es muy grande
	// lo escribimos como dijo jack el destripador, por partes
	while (porEscribir > BUFF_SIZE) {
	    copy_from_user(bufferAux, buffer_sys, BUFF_SIZE);
	    int bytesEscritos = sys_write_console(buffer_sys, BUFF_SIZE);
	
	    bufferAux += bytesEscritos;
	    porEscribir -= bytesEscritos;
  	}
	// escribimos lo que faltaba que sera menor que BUFF_SIZE
	copy_from_user(bufferAux, buffer_sys, porEscribir);
	int bytesEscritos = sys_write_console(buffer_sys, porEscribir);
	
	porEscribir -= bytesEscritos;

	return size - porEscribir;
}

int ret_from_fork(){
	return 0;
}

int pidGlobal = 100;

int sys_fork(){
  if (list_empty(&freequeue)) return -ENOMEM;

  struct list_head *lh = list_first(&freequeue);
  list_del(lh);
  union task_union* child = (union task_union*)list_head_to_task_struct(lh);
  
  // Copiamos el stack y PCB del padre al hijo
  copy_data(current(), child, sizeof(union task_union));

  // Inicializamos el directorio del hijo (hereda el kernel)
  allocate_DIR(&child->task);

  // Reservamos una tabla de páginas para el espacio de usuario del hijo
  int child_ut_frame = alloc_frame();
  if (child_ut_frame < 0) {
      list_add_tail(&child->task.list, &freequeue);
      return -EAGAIN;
  }
  page_table_entry *child_ut = (page_table_entry*)(child_ut_frame << 12);
  clear_page_table(child_ut);

  // Enlazamos la tabla de usuario al directorio del hijo (entrada 1)
  set_ss_pag(get_DIR(&child->task), 1, child_ut_frame, 1);

  // Obtenemos las tablas de usuario de padre e hijo
  page_table_entry *parent_ut = (page_table_entry*)(get_DIR(current())[1].bits.pbase_addr << 12);
  
  // 1. Compartir páginas de código (solo lectura)
  for (int i = 0; i < NUM_PAG_CODE; i++) {
    int frame = get_frame(parent_ut, NUM_PAG_DATA + i);
    set_ss_pag(child_ut, NUM_PAG_DATA + i, frame, 1);
  }

  // 2. Copiar páginas de datos (nuevos frames)
  int frames_data[NUM_PAG_DATA];
  for (int i = 0; i < NUM_PAG_DATA; i++) {
    frames_data[i] = alloc_frame();
    if (frames_data[i] < 0) {
      // Error: liberar lo reservado
      for (int j = 0; j < i; j++) free_frame(frames_data[j]);
      free_frame(child_ut_frame);
      list_add_tail(&child->task.list, &freequeue);
      return -EAGAIN;
    }
    // Mapeamos en el hijo
    set_ss_pag(child_ut, i, frames_data[i], 1);
  }

  // 3. Copia física de los datos usando un mapeo temporal en el padre
  for (int i = 0; i < NUM_PAG_DATA; i++) {
      // Usamos una página libre del kernel (ej: 512) para mapear temporalmente el frame del hijo
      set_ss_pag(get_PT(current()), 512, frames_data[i], 0);
      set_cr3(get_DIR(current())); // Flush TLB para activar mapeo temporal
      
      // Copiamos de la dirección lógica del padre (4MB + i*4KB) a la temporal
      copy_data((void*)((1024 + i) << 12), (void*)(512 << 12), PAGE_SIZE);
      
      // Desmapeamos temporal
      del_ss_pag(get_PT(current()), 512);
  }
  set_cr3(get_DIR(current())); // Flush final

  // Configuración de PID y Quantum
  child->task.PID = ++pidGlobal;
  child->task.quantum = 100;

  // Preparar la pila para task_switch (ebp, ret_from_fork)
  // El layout esperado por cambio_pila y task_switch:
  // [Contexto syscall/interrupción] <- 16 regs
  // [Return address a entry.S]      <- 17
  // [ret_from_fork]                 <- 18
  // [ebp=0]                         <- 19
  
  ((unsigned long *)KERNEL_ESP(child))[-19] = (unsigned long) 0;			// %ebp
  ((unsigned long *)KERNEL_ESP(child))[-18] = (unsigned long) ret_from_fork;	// @RET
  child->task.kernel_esp = (unsigned long) &(child->stack[KERNEL_STACK_SIZE - 19]);

  list_add_tail(&(child->task.list), &readyqueue);
  return child->task.PID;
}

void sys_exit() {
    struct task_struct *curr = current();
    
    // 1. Liberar páginas de datos
    page_table_entry *ut = (page_table_entry*)(get_DIR(curr)[1].bits.pbase_addr << 12);
    free_user_pages(ut);
    
    // 2. Liberar tabla de páginas de usuario
    free_frame(get_DIR(curr)[1].bits.pbase_addr);
    get_DIR(curr)[1].entry = 0;
    
    // 3. Liberar el PCB
    list_add_tail(&curr->list, &freequeue);
    
    // 4. Cambiar a otro proceso
    schedule();
}

int sys_gettime() {
  	return zeos_ticks;
}

int sys_getpid(){
	return current()->PID;
}
