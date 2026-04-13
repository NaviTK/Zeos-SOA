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

int sys_gettime() {
  	return zeos_ticks;
}

int sys_getpid(){
	return current()->PID;
}
