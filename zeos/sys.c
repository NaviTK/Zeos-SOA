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
	int num_pag_kernel = ((int)&last_kernel - (int)&first_kernel) >> 12; // Dividir por 4096 (PAGE_SIZE)

	if(list_empty(&freequeue)) return -ENOMEM;

	struct list_head *lh = list_first(&freequeue);
	list_del(lh);
	union task_union* child = list_head_to_task_struct(lh);
	copy_data(current(), child, sizeof(union task_union));

	allocate_DIR((struct task_struct*) child);
	
	int numPaginas[NUM_PAG_DATA];
	for(int frame = 0; frame < NUM_PAG_DATA; frame++) {
		numPaginas[frame] = alloc_frame();

		if(numPaginas[frame] < 0) {			//Si no tenemos suficiente espacio
			for(int i = 0; i < frame; i++) free_frame(numPaginas[i]);

			list_add_tail(&child->task.list, &freequeue);

			return -EAGAIN;
		}
	}
	printk("Llego");
	//Paginas del hijo creadas
	page_table_entry *child_pt = get_PT(&(child->task));
	printk("Llego1");
	page_table_entry *parent_pt = get_PT(current());
printk("Llego2");
	//Para que padre e hijo compartan paginas físicas de código
	for(int page = 0; page < NUM_PAG_CODE; page++) {
		set_ss_pag(child_pt, PAG_LOG_INIT_CODE + page, get_frame(parent_pt, PAG_LOG_INIT_CODE + page), 1);
	}
printk("Llego");
	//Para que padre e hijo compartan memoria de sistema
	for(int page = 0; page < num_pag_kernel; page++) {
		set_ss_pag(child_pt, page, get_frame(parent_pt, page), 1);
	}
printk("Llego");
	//Mapeo de data + stack del child
	for(int page = 0; page < NUM_PAG_DATA; page++) {
		set_ss_pag(child_pt, PAG_LOG_INIT_DATA + page, numPaginas[page], 1);
	}
printk("Llego");
	int TOTAL_SPACE = num_pag_kernel + NUM_PAG_DATA + NUM_PAG_CODE;
	for(int page = 0; page < NUM_PAG_DATA; page++) {
		set_ss_pag(parent_pt, page + TOTAL_SPACE, get_frame(child_pt, PAG_LOG_INIT_DATA + page), 0);
		copy_data((void *)((PAG_LOG_INIT_DATA + page) << 12), (void *)((TOTAL_SPACE + page) << 12), PAGE_SIZE);
		del_ss_pag(parent_pt, TOTAL_SPACE + page);
	}
	set_cr3(get_DIR(current()));		//Flush del TLB

	child->task.PID = ++pidGlobal;
	child->task.quantum = 100;

	/*Para que el nuevo proceso funcione con task_switch preparamos la pila (ponemos algo en ebp, @RET, kernel_esp-->@handler)*/
	((unsigned long *)KERNEL_ESP(child))[-0x13] = (unsigned long) 0;						//%ebp = $0
	((unsigned long *)KERNEL_ESP(child))[-0x12] = (unsigned long) ret_from_fork;			//@RET = ret_from_fork
	child->task.kernel_esp = (unsigned long) &(child->stack[KERNEL_STACK_SIZE - 19]); 		//Kernel_esp = top de la pila

	/*POR AÑADIR LO DEL BLOCKED Y UNBLOCKED*/

	list_add_tail(&(child->task.list), &readyqueue);		//Añadimos al child a la ready queue
	return child->task.PID;
}

int sys_gettime() {
  	return zeos_ticks;
}

int sys_getpid(){
	return current()->PID;
}
