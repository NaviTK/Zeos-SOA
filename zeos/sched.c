/*

 * sched.c - initializes struct for task 0 anda task 1

 */


#include <sched.h>

#include <mm.h>

#include <io.h>

#include <hardware.h>

#include <interrupt.h>
#include <utils.h>
#include <devices.h>

// union task_union task[NR_TASKS] no longer needed

// 2. Alineamos la pila de sistema a 4KB
unsigned long initial_stack[KERNEL_STACK_SIZE] 
    __attribute__((__aligned__(4096))); // Space for the initial system stack

struct task_struct *init_task;

struct task_struct *idle_task;

extern struct list_head blocked;
struct list_head list_tasks;

struct list_head readyqueue;
int remaining_quantum;

#if 1

struct task_struct *list_head_to_task_struct(struct list_head *l)

{

    return list_entry(l, struct task_struct, list);

}

#endif

struct task_struct *alloc_pcb()
{
  int frame = alloc_frame();
  if (frame < 0) return NULL;
  
  struct task_struct *p = (struct task_struct *)(frame << 12);
  INIT_LIST_HEAD(&p->task_list);
  list_add_tail(&p->task_list, &list_tasks);
  
  return p;
}

void free_pcb(struct task_struct *p)
{
  list_del(&p->task_list);
  free_frame(((unsigned long)p) >> 12);
}


void writeMSR(unsigned long msr, unsigned long valor);

void task_switch(union task_union *new);

void cambio_pila(unsigned long *current_addr, unsigned long new_kesp);


/*

proceso de sistema que se usa cuando no hay procesos de modo usuario para ejecutar en la CPU

se ejecuta en modo sistema

directorio 0 de idle apunta a la tabla de paginas del kernel (guardarse esta tabla de paginas porque la necesitaremos varias veces)

se necesita el task_switch para que entre idle a la CPU

para entrar en CPU hay que hacer un push de la direccion de retorno de cpu_idle y luego push de 0 para que haga match con EBP

*/

void cpu_idle(void)

{

    __sti();

    while (1)

    {

        __asm__ __volatile__("hlt");

    }

}


void init_idle(void)

{

    // Paso 4: Asignar un nuevo struct task_struct dinámicamente.
    struct task_struct *PCB = alloc_pcb();

	// Inicialitzem les variables per block i unblock
	PCB->parent = NULL;
	INIT_LIST_HEAD(&(PCB->children_blocked));
	INIT_LIST_HEAD(&(PCB->children_unblocked));
	PCB->pending_unblocks = 0;

    // Crear espacio de direcciones (solo directorio) dinámicamente
    allocate_DIR(PCB);


    // Paso 3: Inicializar el contexto de ejecución.

    // Accedemos a la pila mediante la union task_union.

    union task_union *idle_union = (union task_union *)PCB;


    // 3.a) Guardar la dirección del código a ejecutar (cpu_idle) en la cima de la pila

    idle_union->stack[KERNEL_STACK_SIZE - 1] = (unsigned long)cpu_idle;


    // 3.b) Guardar el valor inicial de EBP (0) justo debajo para deshacer el enlace dinámico

    idle_union->stack[KERNEL_STACK_SIZE - 2] = 0;


    // 3.c) Guardar la posición actual de la pila en un nuevo campo del PCB.

    // IMPORTANTE: Debes asegurarte de añadir 'int kernel_esp;' dentro

    // de 'struct task_struct' en tu archivo sched.h.

    PCB->kernel_esp = (int)&(idle_union->stack[KERNEL_STACK_SIZE - 2]);


    // Paso 6: Asignar PID 0 al proceso

    PCB->PID = 0;

    set_quantum(PCB, 10);

    // Paso 7: Inicializar la variable global idle_task

    idle_task = PCB;

}


/*
en tiempo de boot (direcciones fisicas)
preparar todas las tablas de paginas para que el kernel pueda trabajar con direcciones logicas
se crea la primera tabla de paginas (a mano)

se necesita: cr3 -> un directorio para init (4KB), una tabla de paginas donde , un segmento de la TP con las traducciones
memory manager --> alloc.frame y free.frame
alloc.frame devuelve el identificador de una pagina fisica que este libre
free.frame libera una pagina fisica
IDPF ==> 20 bits de id y 12 bits a 0
@logica 0 --> sistema || @logica 0x400 000 --> usuario
*/
void init_task1(void)
{
    // 1. Sacamos un PCB válido
    struct task_struct *pcb = alloc_pcb();

    pcb->PID = 1;
    set_quantum(pcb, 10);
	//Inicialitzem les variables per block i unblock
	pcb->parent = NULL;
	INIT_LIST_HEAD(&(pcb->children_blocked));
	INIT_LIST_HEAD(&(pcb->children_unblocked));
	INIT_LIST_HEAD(&(pcb->sibiling));
	pcb->pending_unblocks = 0;
    
    // ========================================================
    // BLOQUE INMODIFICABLE INICIO
    // ========================================================
    int Dir = alloc_frame();
    page_table_entry *init_TP = (page_table_entry *)(Dir << 12);
    clear_page_table(init_TP);

    int so = alloc_frame();
    page_table_entry *so_TP = (page_table_entry *)(so << 12);
    set_kernel_pages(so_TP);

    int user = alloc_frame();
    page_table_entry *user_TP = (page_table_entry *)(user << 12);
    set_user_pages(user_TP);

    set_ss_pag(init_TP, Dir, Dir, 0);
    set_ss_pag(so_TP, so, so, 0);
    set_ss_pag(so_TP, user, user, 1);

    set_ss_pag(init_TP, 0, so, 0);            // directorio entrada 0 --> tabla kernel
    set_ss_pag(init_TP, 1, user, 1);          // directorio entrada 1 --> tabla usuario
    // ========================================================
    // BLOQUE INMODIFICABLE FIN
    // ========================================================

    // 2. Asignamos la tabla de páginas
    pcb->dir_pages_baseAddr = init_TP;

    // 3. Parche para Idle Task (copiamos el código de sistema)
    if (idle_task != NULL) {
        get_DIR(idle_task)[0] = init_TP[0];
    }

    // 4. Configuramos el hardware (CR3)
    set_cr3(pcb->dir_pages_baseAddr);

    // 5. ¡ESTA ES LA LÍNEA MÁGICA QUE YA TENÍAS! 
    // Obliga a que al interrumpir init, se use su pila.
    tss.esp0 = KERNEL_ESP((union task_union *)pcb);
    writeMSR(0x175, (int)tss.esp0);

    // 6. Guardamos el proceso INIT
    init_task = pcb;
}

void inner_task_switch(union task_union *t) {
    tss.esp0 = KERNEL_ESP(t);
    writeMSR(0x175, (int)tss.esp0);
    set_cr3(t->task.dir_pages_baseAddr);
    
    struct task_struct *curr = current(); // Guardamos current() aquí
    
    // DEBUG
    /*char buf[16];
    printk("PID actual: ");
    itoa_hexadecimal(curr->PID, buf);
    printk(buf);
    printk("\n");

    printk("Nuevo ESP: ");
    itoa_hexadecimal(t->task.kernel_esp, buf); 
    printk(buf);
    printk("\n");*/

    cambio_pila(&(curr->kernel_esp), t->task.kernel_esp);
}


void init_sched()

{

    // Inicialitzem list_tasks
    INIT_LIST_HEAD(&list_tasks);
    INIT_LIST_HEAD(&readyqueue);
    INIT_LIST_HEAD(&blocked);
    INIT_LIST_HEAD(&kbd_blocked);
}

void schedule(void) {
	update_sched_data_rr();
	if(needs_sched_rr()) {
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}

void update_sched_data_rr(void) {
	--remaining_quantum;
}

int needs_sched_rr(void) {
	if (remaining_quantum > 0 && !list_empty(&readyqueue)) return 0;
	return 1;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue) {
	struct list_head * list_tmp = &t->list;
	if(!(list_tmp->prev == NULL && list_tmp->next == NULL)){
		list_del(list_tmp);
	}

	if (dst_queue) list_add_tail(list_tmp, dst_queue);
}

void sched_next_rr(void)
{
	struct task_struct *next;

	if(!list_empty(&readyqueue)){
		struct list_head *lf = list_first(&readyqueue);
		list_del(lf);
		next = list_head_to_task_struct(lf);
	}
	else{
		next = idle_task;
	}
	remaining_quantum = get_quantum(next);
	if(current()->PID != next->PID) task_switch((union task_union *)next);
}


/* get_DIR - Returns the Page Directory address for task 't' */

page_table_entry *get_DIR(struct task_struct *t)

{

    return t->dir_pages_baseAddr;

}


int allocate_DIR(struct task_struct *t)
{
    int frame = alloc_frame();
    if (frame < 0) return -1;

    t->dir_pages_baseAddr = (page_table_entry *)(frame << 12);
    clear_page_table(t->dir_pages_baseAddr);

    // Copy kernel mapping
    t->dir_pages_baseAddr[0] = current()->dir_pages_baseAddr[0];

    return 1;
}


/* get_PT - Returns the Page Table address for task 't' */

page_table_entry *get_PT(struct task_struct *t)

{

    return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr)) << 12);

}

// retorna el quantum del proces t

int get_quantum(struct task_struct *t)

{

    return t->quantum;

}


// modifica el quantum del proces t

void set_quantum(struct task_struct *t, int new_quantum)

{

    t->quantum = new_quantum;

} 
