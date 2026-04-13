#include <asm.h>

#Guarda los registros que se pueden haver utilizado
#define SAVE_REGS \
    pushl %ebx;  \
    pushl %esi;  \
    pushl %edi;

#Restaura los registros que se pueden haver utilizado
#define REST_REGS \
    popl %edi;   \
    popl %esi;   \
    popl %ebx;

ENTRY(write)
    # Implementacion con sysenter y Mbrs

    # Guardamos pila usuario
    pushl %ebp
	mov %esp,%ebp

    # Guardar registros que se podrian usar, en este caso solo guardamos ebx
    # SAVE_REGS
    pushl %ebx

    #Pasamos los parametros
    mov 0x10(%ebp), %ebx # size -> ebx
	mov 0x0c(%ebp), %ecx # buffer -> ecx
	mov 0x08(%ebp), %edx # fd -> edx

    # Codigo 4 system call para write en %eax
    movl $4, %eax

    # Guardar %ecx y %edx en la pila de usuario
    pushl %ecx
    pushl %edx

    # Guardamos la dirrecion de lo que se hara despues deñ sysenter (loque se hará despues del sysenter)
    pushl $write_return 
    
    # fake dinamic link
    pushl %ebp
    mov %esp, %ebp

    #Entrar al sistema
    sysenter

write_return:
    # Eliminamos datos de la pila
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    #Comparamos el return de la syscall
    cmpl $0, %eax
	jge fast_wr_no_error

	# se ejecuta cuando nos devuelven un error
	negl %eax           # Para obtener codigo error en positivo
	movl %eax, errno    # Pone el error en errno
	movl $-1, %eax

fast_wr_no_error:
    # entramos aqui cuando no hay errores o despues de procesarlos

    # Restaurar registros antes de salir
    # REST_REGS pero en este caso solo es ebx
    popl %ebx

	popl %ebp
	ret


ENTRY(gettime)

    pushl %ebp
	mov %esp,%ebp

    #Codigo 10 systemcall en %eax
    movl $10, %eax

    # Guardar %ecx y %edx en pila de usuario
    pushl %ecx
    pushl %edx

    #Guardar la direccion de retorno en la pila
    pushl $gettime_return 
    
    # fake dynamic link
    pushl %ebp
    mov %esp, %ebp

    #Entrar al sistema
	sysenter

gettime_return:
    # sacamos de la pila
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    # vemos si ha habido algun error
    cmpl $0, %eax
	jge fast_gt_no_error

	# entramos si hay error y guardamos el codigo en errno(hay que negar el codigo porque viene en negativo)
	negl %eax           # Para obtener codigo error en positivo
	movl %eax, errno    # Pone el error en errno
	movl $-1, %eax

fast_gt_no_error:
    # Saltamos aqui cuando no hay error y despues de procesarlo
	popl %ebp
	ret

ENTRY(getpid)
    pushl %ebp
    movl %esp, %ebp

    pushl %ecx
    pushl %edx

    movl $20, %eax      #Código de la syscall

    pushl $getpid_return        #Direccion de retorno

    pushl %ebp          #Fake dynamic link
    movl %esp, %ebp

    sysenter

getpid_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx
    
    cmpl $0, %eax
    jge getpid_sin_error        #Si no hay error saltamos

    negl %eax               #Ponemos el error en errno
    movl %eax, errno
    movl $-1, %eax

getpid_sin_error:
    popl %ebp
    ret