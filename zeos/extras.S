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
    # 1. Crear el marco de pila de la función
    pushl %ebp
    movl %esp, %ebp

    # 2. Guardar el EBP actual (que apunta a la cima temporal de la pila) en el PCB viejo
    # El parámetro &current()->kernel_esp está en la posición 8(%ebp)
    movl 8(%ebp), %eax      
    movl %ebp, (%eax)       

    # 3. Cambiar la pila
    # El parámetro new_kesp está en la posición 12(%ebp)
    movl 12(%ebp), %esp     

    # 4. Deshacer el marco de pila del proceso nuevo y retornar limpiamente
    popl %ebp
    ret
