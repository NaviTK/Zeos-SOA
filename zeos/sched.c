/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <hardware.h>
#include <interrupt.h>

char initial_stack[KERNEL_STACK_SIZE]; // Space for the initial system stack
struct task_struct* init_task;
struct task_struct *idle_task;

struct list_head freequeue;
struct list_head readyqueue;


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
	while(1)
	{
	;
	}
}

void init_idle (void)
{

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
	int Dir = alloc_frame();
	page_table_entry* init_TP = (page_table_entry*) (Dir<<12);
	clear_page_table(init_TP);
	
	int so = alloc_frame();
	page_table_entry* so_TP = (page_table_entry*) (so<<12);
	set_kernel_pages(so_TP);

	int user = alloc_frame();
	page_table_entry* user_TP = (page_table_entry*) (user<<12);
	set_user_pages(user_TP);

	set_ss_pag(init_TP, Dir, Dir, 0);
	set_ss_pag(so_TP, so, so, 0);
	set_ss_pag(so_TP, user, user, 1);

	init_TP[0].entry = so;
	init_TP[1].entry = user;

	/*struct list_head *l = list_first(&freequeue);
	list_del(l);
	struct task_struct *PCB = list_head_to_task_struct(l);*/
	
	struct task_struct *PCB = (struct task_struct *) alloc_frame();
		
	set_ss_pag(so_TP, (int) PCB, (int) PCB, 0);

	PCB->PID = 1;
	
	tss.esp0 = (DWord) &initial_stack;
	writeMSR(0x175, (int) tss.esp0);
	
	PCB->dir_pages_baseAddr = init_TP;
	set_cr3(init_TP);
		
	init_task = PCB;
}	

void inner_task_switch(union task_union *new){
	tss.esp0 = KERNEL_ESP(union task_union *) new;

	writeMSR(0x175, (int) tss.esp0);

	set_cr3(get_DIR(&new->task));

	cambio_pila();
}

void init_sched()
{

}

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
       return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
       return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}
// retorna el quantum del proces t
int get_quantum (struct task_struct *t) {
	return t->quantum;
}

//modifica el quantum del proces t
void set_quantum (struct task_struct *t, int new_quantum) {
	t->quantum = new_quantum;
}

