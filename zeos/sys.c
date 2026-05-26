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
extern struct list_head blocked;
extern struct list_head readyqueue;
extern struct list_head kbd_blocked;
extern int last_kernel;
extern int first_kernel;
extern Byte phys_mem[TOTAL_PAGES];
extern int shared_frames[NUM_SHARED_PAGES];
extern int shared_refs[NUM_SHARED_PAGES];
extern int shared_marked_rm[NUM_SHARED_PAGES];

int sys_get_stats(int type) {
    if (type == 0) return TOTAL_PAGES;
    if (type == 1) {
        int count = 0;
        for (int i = 0; i < TOTAL_PAGES; i++) {
            if (phys_mem[i] == 1 /* USED_FRAME */) continue;
            count++;
        }
        return count;
    }
    return -EINVAL;
}


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
  struct task_struct *pcb_child = alloc_pcb();
  if (pcb_child == NULL) return -ENOMEM;
  union task_union* child = (union task_union*)pcb_child;
  
  // Copiamos el stack y PCB del padre al hijo
  copy_data(current(), child, sizeof(union task_union));

  // Inicializamos el directorio del hijo (hereda el kernel)
  allocate_DIR(&child->task);

  // Reservamos una tabla de páginas para el espacio de usuario del hijo
  int child_ut_frame = alloc_frame();
  if (child_ut_frame < 0) {
      free_pcb(pcb_child);
      return -EAGAIN;
  }
  
  // Mapear el frame en la TP de sistema correspondiente si paging está activo
  if (kernel_page_table) {
      if (child_ut_frame < 1024) set_ss_pag(kernel_page_table, child_ut_frame, child_ut_frame, 0);
      else if (kernel_page_table2) set_ss_pag(kernel_page_table2, child_ut_frame - 1024, child_ut_frame, 0);
  }

  page_table_entry *child_ut = (page_table_entry*)(child_ut_frame << 12);
  clear_page_table(child_ut);

  // Enlazamos la tabla de usuario al directorio del hijo (entrada 2 para M3)
  set_ss_pag(get_DIR(&child->task), 2, child_ut_frame, 1);

  // Obtenemos las tablas de usuario de padre e hijo
  page_table_entry *parent_ut = (page_table_entry*)(get_DIR(current())[2].bits.pbase_addr << 12);
  
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
      free_pcb(pcb_child);
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
      
      // Copiamos de la dirección lógica del padre (8MB + i*4KB) a la temporal
      // 8MB >> 12 = 2048
      copy_data((void*)((2048 + i) << 12), (void*)(512 << 12), PAGE_SIZE);
      
      // Desmapeamos temporal
      del_ss_pag(get_PT(current()), 512);
  }
  set_cr3(get_DIR(current())); // Flush final

  // Heredar páginas de memoria compartida del padre al hijo
  for (int shm_idx = NUM_PAG_DATA + NUM_PAG_CODE; shm_idx < PT_ENTRIES; shm_idx++) {
    if (parent_ut[shm_idx].bits.present) {
      unsigned int shm_frame = get_frame(parent_ut, shm_idx);
      for (int i = 0; i < NUM_SHARED_PAGES; i++) {
        if (shared_frames[i] == (int)shm_frame) {
          set_ss_pag(child_ut, shm_idx, shm_frame, 1);
          shared_refs[i]++;
          break;
        }
      }
    }
  }

  // Configuración de PID y Quantum
  child->task.PID = ++pidGlobal;
  child->task.quantum = 10;

  // --- INICIO DE LAS CORRECCIONES PARA BLOCK/UNBLOCK ---
  
  // 1. El padre del nuevo proceso es el proceso actual (quien llama a fork)
  child->task.parent = current();
  
  // 2. Inicializar las listas de hijos bloqueados/no bloqueados para que nazcan vacías
  INIT_LIST_HEAD(&(child->task.children_blocked));
  INIT_LIST_HEAD(&(child->task.children_unblocked));
  
  // 3. Añadir este hijo a la lista de "no bloqueados" del padre actual
  list_add_tail(&(child->task.sibiling), &(current()->children_unblocked));
  
  // 4. Asegurar que el hijo no hereda llamadas a unblock pendientes del padre
  child->task.pending_unblocks = 0;
  
  // --- FIN DE LAS CORRECCIONES ---

  // Preparar la pila para task_switch (ebp, ret_from_fork)
  // El layout esperado por cambio_pila y task_switch:
  // [Contexto syscall/interrupción] <- 16 regs
  // [Return address a entry.S]      <- 17
  // [ret_from_fork]                 <- 18
  // [ebp=0]                         <- 19
  
  ((unsigned long *)KERNEL_ESP(child))[-19] = (unsigned long) 0;            // %ebp
  ((unsigned long *)KERNEL_ESP(child))[-18] = (unsigned long) ret_from_fork;    // @RET
  child->task.kernel_esp = (unsigned long) &(child->stack[KERNEL_STACK_SIZE - 19]);

  list_add_tail(&(child->task.list), &readyqueue);
  return child->task.PID;
}

void sys_exit() {
    struct task_struct *curr = current();
    page_table_entry *ut = (page_table_entry*)(get_DIR(curr)[2].bits.pbase_addr << 12);

    // 0. Desconectar páginas de memoria compartida antes de liberar
    for (unsigned int pt_idx = NUM_PAG_DATA + NUM_PAG_CODE; pt_idx < PT_ENTRIES; pt_idx++) {
        if (ut[pt_idx].bits.present) {
            unsigned int frame = get_frame(ut, pt_idx);
            for (int i = 0; i < NUM_SHARED_PAGES; i++) {
                if (shared_frames[i] == (int)frame) {
                    del_ss_pag(ut, pt_idx);
                    shared_refs[i]--;
                    if (shared_refs[i] == 0 && shared_marked_rm[i] == 1) {
                        free_frame(shared_frames[i]);
                        shared_frames[i] = -1;
                        shared_marked_rm[i] = 0;
                    }
                    break;
                }
            }
        }
    }
    set_cr3(get_DIR(curr)); // Flush TLB tras desconectar shm

    // 1. Liberar páginas de datos (frames físicos)
    free_user_pages(ut);
    
    // 2. Liberar tabla de páginas de usuario (el frame de la TP en sí)
    free_frame(get_DIR(curr)[2].bits.pbase_addr);
    get_DIR(curr)[2].entry = 0;

    // Liberar el PCB
    // 3. Liberar el directorio
    free_frame(((unsigned long)curr->dir_pages_baseAddr) >> 12);


    // 4. Liberar el PCB
    free_pcb(curr);
    
    // 4. Cambiar a otro proceso
    sched_next_rr();
}

int sys_gettime() {
  	return zeos_ticks;
}

int sys_read(char *b, int maxchars)
{
  /* --- Parameter validation --- */
  if (b == NULL) return -EFAULT;
  if (maxchars < 0) return -EINVAL;
  if (maxchars == 0) { kbd_buf_flush(); return 0; } /* special: flush buffer */
  if (!access_ok(VERIFY_WRITE, b, maxchars)) return -EFAULT;

  int to_read = (maxchars > BUFF_SIZE) ? BUFF_SIZE : maxchars;

  /* --- Try to serve immediately from the buffer --- */
  int got = kbd_buf_read(buffer_sys, to_read);
  if (got > 0) {
    copy_to_user(buffer_sys, b, got);
    return got;
  }

  /* --- Nothing available yet: block until enough chars arrive --- */
  struct task_struct *curr = current();
  curr->kbd_chars_needed = to_read;
  curr->kbd_user_buf     = b;
  curr->kbd_read_result  = 0;

  /* Move to kbd_blocked queue */
  update_process_state_rr(curr, &kbd_blocked);
  sched_next_rr();            /* context-switch away; returns when woken */

  /* We are back with our own CR3 active — read into kernel buffer, then copy to user */
  got = kbd_buf_read(buffer_sys, to_read);
  if (got > 0) {
    copy_to_user(buffer_sys, b, got);
  }
  return got;
}

int sys_getpid(){
	return current()->PID;
}

int sys_block() {
  struct task_struct* current_pcb = current();
  struct task_struct* parent_pcb = current_pcb->parent;

  //Si hi ha pending unblocks només es decrementa variable i return
  if (current()->pending_unblocks > 0) {
    current()->pending_unblocks--;
    return 0;
  }

  //Afegim el proces a la blocked queue si el pare es diferent de idle (null)
  if (parent_pcb != NULL) {
    list_del(&(current_pcb->sibiling));   //Borrem de la llista actual
    list_add_tail(&(current_pcb->sibiling), &(parent_pcb->children_blocked));   //Afegim a llista de blocked del pare
  }

  //Afegim el proces a la blocked queue i canviem de proces
  update_process_state_rr(current_pcb, &blocked);
  sched_next_rr();
  return 0;
 }

 int sys_unblock(int pid) {
  struct task_struct* parent_pcb = current();

  //Recorrem tots els fills del pare per trobar el que té PID = pid
  struct list_head* pos;
  list_for_each(pos, &(parent_pcb->children_blocked)) {
    struct task_struct* child = list_entry(pos, struct task_struct, sibiling);
    if (child->PID == pid) {
      list_del(&(child->sibiling));
      list_add_tail(&(child->sibiling), &(parent_pcb->children_unblocked));
      update_process_state_rr(child, &readyqueue);
      return 0;
    }
  }

  //Si no estaba en blocked alsehores augmentem el pendingUnblocks
  list_for_each(pos, &(parent_pcb->children_unblocked)) {
    struct task_struct* child = list_entry(pos, struct task_struct, sibiling);
    if (child->PID == pid) {
      child->pending_unblocks++;
      return 0;
    }
  }

  //Retornem -1 si no hem trobat el pid
  return -1;
 }

int sys_shmat(int id, void* addr) {
    /* 1. Validate shared page ID */
    if (id < 0 || id >= NUM_SHARED_PAGES) return -EINVAL;
    /* Do not allow attaching pages marked for removal */
    if (shared_marked_rm[id] == 1) return -EINVAL;
    if (shared_frames[id] < 0) {
        shared_frames[id] = alloc_frame();
        if (shared_frames[id] < 0) return -ENOMEM;
    }

    /* 2. Get the process's user page table */
    page_table_entry *pt = (page_table_entry*)(get_DIR(current())[2].bits.pbase_addr << 12);

    unsigned int log_page;

    if (addr != (void*)0) {
        /* Must be page-aligned */
        if ((unsigned int)addr & 0xFFF) return -EINVAL;
        log_page = (unsigned int)addr >> 12;

        unsigned int pt_idx = log_page - (L_USER_START >> 12);

        if (pt_idx < (NUM_PAG_DATA + NUM_PAG_CODE) || pt_idx >= PT_ENTRIES)
            return -EINVAL;

        if (pt[pt_idx].bits.present) {
            if (get_frame(pt, pt_idx) == shared_frames[id]) {
                return (int)(log_page << 12);
            }
            return -EINVAL;
        }

        set_ss_pag(pt, pt_idx, shared_frames[id], 1);
    } else {
        unsigned int start = NUM_PAG_DATA + NUM_PAG_CODE;
        int found = 0;
        unsigned int pt_idx;
        for (pt_idx = start; pt_idx < PT_ENTRIES; pt_idx++) {
            if (!pt[pt_idx].bits.present) {
                found = 1;
                break;
            }
        }
        if (!found) return -ENOMEM;

        set_ss_pag(pt, pt_idx, shared_frames[id], 1);
        log_page = (L_USER_START >> 12) + pt_idx;
    }

    /* Increment reference count */
    shared_refs[id]++;

    set_cr3(get_DIR(current()));
    return (int)(log_page << 12);
}

/* Helper: zero out and free a shared page (call when refs==0 and marked_rm==1) */
static void cleanup_shared_page(int id)
{
    /* Map temporarily at kernel page 512 to zero the content */
    set_ss_pag(get_PT(current()), 512, shared_frames[id], 0);
    set_cr3(get_DIR(current()));
    int *tmp = (int *)(512 << 12);
    for (int i = 0; i < (int)(PAGE_SIZE / sizeof(int)); i++)
        tmp[i] = 0;
    del_ss_pag(get_PT(current()), 512);
    set_cr3(get_DIR(current()));

    free_frame(shared_frames[id]);
    shared_frames[id]    = -1;
    shared_marked_rm[id] = 0;
    shared_refs[id]      = 0;
}

int sys_shmdt(void *addr)
{
    if ((unsigned int)addr & 0xFFF) return -EINVAL;

    unsigned int log_page = (unsigned int)addr >> 12;
    unsigned int pt_idx   = log_page - (L_USER_START >> 12);

    if (pt_idx < (NUM_PAG_DATA + NUM_PAG_CODE) || pt_idx >= PT_ENTRIES)
        return -EINVAL;

    page_table_entry *pt = (page_table_entry*)(get_DIR(current())[2].bits.pbase_addr << 12);
    if (!pt[pt_idx].bits.present) return -EINVAL;

    unsigned int frame = get_frame(pt, pt_idx);
    int id = -1;
    for (int i = 0; i < NUM_SHARED_PAGES; i++) {
        if (shared_frames[i] == (int)frame) { id = i; break; }
    }
    if (id == -1) return -EINVAL;

    del_ss_pag(pt, pt_idx);
    set_cr3(get_DIR(current()));

    shared_refs[id]--;
    if (shared_refs[id] == 0 && shared_marked_rm[id] == 1)
        cleanup_shared_page(id);

    return 0;
}

int sys_shmrm(int id)
{
    if (id < 0 || id >= NUM_SHARED_PAGES) return -EINVAL;
    if (shared_frames[id] < 0) return -EINVAL;

    shared_marked_rm[id] = 1;
    if (shared_refs[id] == 0)
        cleanup_shared_page(id);

    return 0;
}
