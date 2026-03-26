/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <hardware.h>
#include <interrupt.h>

union task_union task[NR_TASKS]
	__attribute__((__section__(".data.task")));

char initial_stack[KERNEL_STACK_SIZE]; // Space for the initial system stack
struct task_struct *init_task;
struct task_struct *idle_task;

struct list_head freequeue;
struct list_head readyqueue;

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return list_entry(l, struct task_struct, list);
}
#endif

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
		;
	}
}

void init_idle(void)
{
	// Paso 4: Asignar un nuevo struct task_struct.
	// Usamos la freequeue estándar (como tenías comentado en init_task1)
	struct list_head *l = list_first(&freequeue);
	list_del(l);
	struct task_struct *PCB = list_head_to_task_struct(l);

	// Pasos 1, 2 y 5: Crear espacio de direcciones (solo directorio)
	// y asignar a dir_pages_baseAddr. La macro/función allocate_DIR
	// de ZeOS se encarga de esto y reutiliza la tabla del sistema.
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
	// 1. Sacamos un PCB válido de la freequeue
	struct list_head *lh = list_first(&freequeue);
	list_del(lh);
	struct task_struct *pcb = list_head_to_task_struct(lh);

	pcb->PID = 1;
	set_quantum(pcb, 10);

	// Esto inicializa el baseAddr por defecto (aunque luego lo sobreescribas)
	allocate_DIR(pcb);

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

	init_TP[0].entry = so;
	init_TP[1].entry = user;
	// ========================================================
	// BLOQUE INMODIFICABLE FIN
	// ========================================================

	// 2. Asignamos la tabla de páginas (creada a mano arriba) al PCB
	pcb->dir_pages_baseAddr = init_TP;

	// 3. Configuramos el hardware (CR3) para que use esta tabla de páginas
	set_cr3(pcb->dir_pages_baseAddr);

	// 4. Configuramos el stack de sistema para cuando la CPU pase a Ring 0
	// OJO: tss.esp0 debe apuntar a la CIMA de la pila del kernel del proceso,
	// no al inicio de un array global. Accedemos a él mediante task_union.
	union task_union *task1_union = (union task_union *)pcb;
	tss.esp0 = (DWord) & (task1_union->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, (int)tss.esp0);

	// 5. Guardamos el proceso INIT en la variable global
	init_task = pcb;
}

void inner_task_switch(union task_union *new)
{
	tss.esp0 = KERNEL_ESP((union task_union *)new);

	writeMSR(0x175, (int)tss.esp0);

	set_cr3(get_DIR(&new->task));

	cambio_pila(&current()->kernel_esp, new->task.kernel_esp);
}

void init_sched()
{
	// Inicialitzem freequeue
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	// cuando tengamos blocked INIT_LIST_HEAD(&blocked);
	//  Recorre vector de tasks per afegir tots en freeequeue
	for (int i = 0; i < NR_TASKS; ++i)
		list_add(&(task[i].task.list), &freequeue);
}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry *get_DIR(struct task_struct *t)
{
	return t->dir_pages_baseAddr;
}

int allocate_DIR(struct task_struct *t)
{
	int pos;

	pos = ((int)t - (int)task) / sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry *)&dir_pages[pos]; // dir_pages esta declarado en mm.h, de momento no funciona
																 // hay que hacer que se guarden las tablas de paginas aqui

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
