#include <libc.h>

char buff[24];

int pid;

//int add(int par1, int par2);

int __attribute__ ((__section__(".text.main")))
  main(void)
  {
    // Test fork
    
    char buffer[16];
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
    }

    while(1){
        // codigo usuario de prueba
        //int n = add(1, 2);

        //Test pagefault
        //char* p = 0;
        //*p = 'x';

        //Test Write
        //if (write(1, "El siguiente write dara error de canal:\n", 40) < 0) perror();
        //if (write(2, "Hello World!\n", 13) < 0) perror();
        //if (write(1, "FUNCIONA!\n", 10) < 0) perror();

        //Test gettime
        //char buffer[10];
        //itoa(gettime(), buffer);
        //if(write(1, "Tiempo desde el inicio del sistema: ", 36) < 0) perror();
        //if(write(1, buffer, strlen(buffer)) < 0) perror();
        //if(write(1, "\n", 1) < 0) perror();
        //for(int i = 0; i < 100000000; i++); // Bucle para perder tiempo

        //Test gettime
        //int pid = getpid();
        //itoa(pid, buffer);
        //if(write(1, "El pid del proceso es: ", 23) < 0) perror();
        //if(write(1, buffer, strlen(buffer)) < 0) perror();
        //if(write(1, "\n", 1) < 0) perror();
    }
}
