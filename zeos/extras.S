#include <asm.h>


ENTRY(writeMSR)
	#Guardamos la pila de usuario
	pushl %ebp
	mov %esp, %ebp
	
	#MSR(%ECX) = EDX:EAX
	mov 0x8(%ebp), %ecx	#ECX = identificador del MSR
	mov $0, %edx		#Parte alta del valor del MSR (siempre 0)
	mov 0xC(%ebp), %eax	#Parte baja del valor del MSR
	wrmsr       #Función que escribe en el MSR
	
	pop %ebp
	ret



ENTRY(task_switch)
    push %ebp
    mov %esp, %ebp

    push %esi
    push %edi
    push %ebx

    pushl 8(%ebp)   #aqui esta *new

    call inner_task_switch

    add $4, %esp

    pop %ebx
    pop %edi
    pop %esi
    
    mov %ebp, %esp
    pop %ebp

    ret


ENTRY(cambio_pila)
    # Guardamos el ebp del current
    movl 4(%esp), %eax
    movl %ebp, (%eax)

    # Guardamos %ebp en el valor del esp del nuevo proceso 
    movl 8(%esp), %esp

    # Preparamos la estructura de la pila
    popl %ebp

    ret