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
    # Implementacion con sysenter y Mbrs

    # Guardamos pila usuario
    pushl %ebp
 mov %esp,%ebp

    # Guardar registros que se podrian usar, en este caso solo guardamos ebx
    # pushl %ebx; pushl %esi; pushl %edi;
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
 negl %eax # Para obtener codigo error en positivo
 movl %eax, errno # Pone el error en errno
 movl $-1, %eax

fast_wr_no_error:
    # entramos aqui cuando no hay errores o despues de procesarlos

    # Restaurar registros antes de salir
    # popl %edi; popl %esi; popl %ebx; pero en este caso solo es ebx
    popl %ebx

 popl %ebp
 ret


.globl gettime; .type gettime, @function; .align 0; gettime:

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
 negl %eax # Para obtener codigo error en positivo
 movl %eax, errno # Pone el error en errno
 movl $-1, %eax

fast_gt_no_error:
    # Saltamos aqui cuando no hay error y despues de procesarlo
 popl %ebp
 ret

.globl getpid; .type getpid, @function; .align 0; getpid:
    pushl %ebp
    movl %esp, %ebp

    pushl %ecx
    pushl %edx

    movl $20, %eax #Código de la syscall

    pushl $getpid_return #Direccion de retorno

    pushl %ebp #Fake dynamic link
    mov %esp, %ebp

    sysenter

getpid_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge getpid_sin_error #Si no hay error saltamos

    negl %eax #Ponemos el error en errno
    movl %eax, errno
    movl $-1, %eax

getpid_sin_error:
    popl %ebp
    ret

.globl fork; .type fork, @function; .align 0; fork:
    pushl %ebp
    movl %esp, %ebp

    pushl %ecx
    pushl %edx

    movl $2, %eax

    pushl $fork_return

    pushl %ebp
    mov %esp, %ebp

    sysenter

fork_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge fork_sin_error

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fork_sin_error:
    popl %ebp
    ret

.globl exit; .type exit, @function; .align 0; exit:
    pushl %ebp
    movl %esp, %ebp

    pushl %ecx
    pushl %edx

    movl $1, %eax

    pushl $exit_return

    pushl %ebp
    mov %esp, %ebp

    sysenter

exit_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    # Exit no debería volver nunca
    popl %ebp
    ret

    .globl block; .type block, @function; .align 0; block:
    pushl %ebp
    movl %esp, %ebp

    # Guardar %ecx y %edx en user stack
    pushl %ecx
    pushl %edx

    #Ponemos codigo syscall
    movl $21, %eax

    pushl $block_return

    # Se hace fake dinamic link
    pushl %ebp
    mov %esp, %ebp

    #Entrar al sistema
    sysenter

block_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    #Comparamos el return de la syscall
    cmpl $0, %eax
 jge block_no_error

    # Se ejcuta si hay error
 negl %eax # Para obtener codigo error en positivo
 movl %eax, errno # Pone el error en errno
 movl $-1, %eax

block_no_error:
    # Se ejecuta si no hay error o cuando el error se ha guardado en errno
 popl %ebp
 ret


.globl unblock; .type unblock, @function; .align 0; unblock:
    pushl %ebp
    movl %esp, %ebp


    mov 0x08(%ebp),%edx

    # Guardar %ecx y %edx en user stack
    pushl %ecx
    pushl %edx

    #Ponemos codigo syscall
    movl $22, %eax

    pushl $unblock_return

    # Se hace fake dinamic link
    pushl %ebp
    mov %esp, %ebp

    #Entrar al sistema
    sysenter

unblock_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    #Comparamos el return de la syscall
    cmpl $0, %eax
 jge unblock_no_error

    # Se ejcuta si hay error
 negl %eax # Para obtener codigo error en positivo
 movl %eax, errno # Pone el error en errno
 movl $-1, %eax

unblock_no_error:
    # Se ejecuta si no hay error o cuando el error se ha guardado en errno
 popl %ebp
 ret

.globl read; .type read, @function; .align 0; read:
    pushl %ebp
    movl %esp, %ebp

    # Load arguments: edx = b (arg1), ecx = maxchars (arg2)
    mov 0x08(%ebp), %edx # b -> edx
    mov 0x0c(%ebp), %ecx # maxchars -> ecx

    # Save ecx and edx onto user stack (as expected by sysenter return)
    pushl %ecx
    pushl %edx

    # Syscall number 3 (read)
    movl $3, %eax

    # Push return address and fake dynamic link
    pushl $read_return
    pushl %ebp
    mov %esp, %ebp

    sysenter

read_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge read_no_error

    negl %eax
    movl %eax, errno
    movl $-1, %eax

read_no_error:
    popl %ebp
    ret

.globl get_stats; .type get_stats, @function; .align 0; get_stats:
    pushl %ebp
    movl %esp, %ebp

    mov 0x08(%ebp), %edx

    pushl %ecx
    pushl %edx

    movl $35, %eax

    pushl $get_stats_return
    pushl %ebp
    mov %esp, %ebp

    sysenter

get_stats_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge get_stats_no_error

    negl %eax
    movl %eax, errno
    movl $-1, %eax

get_stats_no_error:
    popl %ebp
    ret
