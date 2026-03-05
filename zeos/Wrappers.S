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

ENTRY(fast_write)
    # Implementacion con sysenter.

    # Guardar pila usuario
    pushl %ebp
	mov %esp,%ebp

    # Guardar registros que se podrian usar, ebx se usa para guardar la i si hay bucle así que es necesario
    # SAVE_REGS
    pushl %ebx

    #Pasar parametros
    mov 0x10(%ebp), %ebx # size -> ebx
	mov 0x0c(%ebp), %ecx # buffer -> ecx
	mov 0x08(%ebp), %edx # fd -> edx

    #Codigo system call en %eax
    movl $4, %eax

    # Guardar %ecx y %edx en user stack
    pushl %ecx
    pushl %edx

    #Guardar la return address en stack (loque se hará despues del sysenter)
    pushl $write_return 
    
    # Se hace fake dinamic link
    pushl %ebp
    mov %esp, %ebp

    #Entrar al sistema
    sysenter

write_return:
    # Eliminamos data de stack 
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    #Comparamos el return de la syscall
    cmpl $0, %eax
	jge fast_wr_no_error

	# Se ejcuta si hay error
	#negl %eax           # Para obtener codigo error en positivo
	#movl %eax, errno    # Pone el error en errno
	movl $-1, %eax

fast_wr_no_error:
    # Se ejecuta si no hay error o cuando el error se ha guardado en errno

    # Restaurar registros antes de salir
    # REST_REGS
    popl %ebx

	popl %ebp
	ret
