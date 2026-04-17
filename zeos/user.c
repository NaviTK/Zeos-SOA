#include <libc.h>

char buff[24];

int pid;

//int add(int par1, int par2);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    char buffer[16];
    
    write(1, "Iniciando prueba de fork...\n", 28);
    
    int pidHijo = fork();
    
    if (pidHijo < 0) {
        perror();
    } 
    else if (pidHijo == 0) {
        // Estamos en el hijo
        int my_pid = getpid();
        itoa(my_pid, buffer);
        write(1, "Soy el HIJO. Mi PID es: ", 24);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
        // El hijo termina
        exit();
    } 
    else {
        // Estamos en el padre
        int my_pid = getpid();
        itoa(my_pid, buffer);
        write(1, "Soy el PADRE. Mi PID es: ", 25);
        write(1, buffer, strlen(buffer));
        write(1, ". Mi hijo tiene PID: ", 21);
        itoa(pidHijo, buffer);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
        
        // El padre espera un poco y termina
        for(int i=0; i<1000000; i++); 
        exit();
    }

    while(1); // No debería llegar aquí
}
