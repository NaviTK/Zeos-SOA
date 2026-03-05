# 0 "Wrappers.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "Wrappers.S"
# 1 "include/asm.h" 1
# 2 "Wrappers.S" 2

#Guarda los registros que se pueden haver utilizado





#Restaura los registros que se pueden haver utilizado





.globl write; .type write, @function; .align 0; write:
    # Implementacion con sysenter.

    # Guardar pila usuario
    pushl %ebp
 mov %esp,%ebp

    # Guardar registros que se podrian usar, ebx se usa para guardar la i si hay bucle así que es necesario
    # pushl %ebx; pushl %esi; pushl %edi;
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
 negl %eax # Para obtener codigo error en positivo
 movl %eax, errno # Pone el error en errno
 movl $-1, %eax

fast_wr_no_error:
    # Se ejecuta si no hay error o cuando el error se ha guardado en errno

    # Restaurar registros antes de salir
    # popl %edi; popl %esi; popl %ebx;
    popl %ebx

 popl %ebp
 ret
