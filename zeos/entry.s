# 0 "entry.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "entry.S"




# 1 "include/asm.h" 1
# 6 "entry.S" 2
# 1 "include/segment.h" 1
# 7 "entry.S" 2
# 1 "include/errno.h" 1
# 8 "entry.S" 2
# 71 "entry.S"
.globl clock_handler; .type clock_handler, @function; .align 0; clock_handler:
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
 movb $0x20, %al; outb %al, $0x20;
 call clock_routine
 popl %edx; popl %ecx; popl %ebx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
 iret

.globl keyboard_handler; .type keyboard_handler, @function; .align 0; keyboard_handler:
       pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
       call keyboard_routine
       movb $0x20, %al; outb %al, $0x20;
       popl %edx; popl %ecx; popl %ebx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
       iret

.globl pagefault_handler; .type pagefault_handler, @function; .align 0; pagefault_handler:
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
 pushl 0x30(%esp)
 call pagefault_routine
 movb $0x20, %al; outb %al, $0x20;
 popl %edx; popl %ecx; popl %ebx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
 iret

.globl syscall_handler_sysenter; .type syscall_handler_sysenter, @function; .align 0; syscall_handler_sysenter:
 push $0x2B
 push %EBP
 pushfl
 push $0x23
 push 4(%EBP)
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
 cmpl $0, %EAX
 jl sysenter_err
 cmpl $MAX_SYSCALL, %EAX
 jg sysenter_err
 call *sys_call_table(, %EAX, 0x04)
 jmp sysenter_fin

sysenter_err:
 movl $-38, %EAX

sysenter_fin:
 movl %EAX, 0x18(%ESP)
 popl %edx; popl %ecx; popl %ebx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
 movl (%ESP), %EDX
 movl 12(%ESP), %ECX
 sti
 sysexit

.globl system_call_handler; .type system_call_handler, @function; .align 0; system_call_handler:
      pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es

      # Comprobamos si el código de syscall es válido (0 > Syscall < MAX_SYSCALL)
      cmpl $0, %eax
      jl is_error
      cmpl $MAX_SYSCALL, %eax
      jg is_error

   # Código válido
      call *sys_call_table(,%eax, 0x04);
      jmp end

is_error:
      # Código inválido, -código de error a eax
      movl $-38, %eax
end:
      # Machacamos el anterior valor de %eax para que ahora contenga el resultado del system call
      movl %eax, 0x18(%esp)
      popl %edx; popl %ecx; popl %ebx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs;
      iret

.globl writeMSR; .type writeMSR, @function; .align 0; writeMSR:
 #Guardamos la pila de usuario
 pushl %ebp
 mov %esp, %ebp

 #MSR(%ECX) = EDX:EAX
 mov 0x8(%ebp), %ecx #ECX = identificador del MSR
 mov $0, %edx #Parte alta del valor del MSR (siempre 0)
 mov 0xC(%ebp), %eax #Parte baja del valor del MSR
 wrmsr

 pop %ebp
 ret
