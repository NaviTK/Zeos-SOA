#include <libc.h>

char buff[24];
int pid;

int __attribute__ ((__section__(".text.main")))
main(void)
{

    char buffer[16];
/*
    // --- TEST FORK 
    int pidHijo = fork();
    
    if (pidHijo < 0) {
        perror();
    } 
    else if (pidHijo == 0) {
        int my_pid = getpid();
        itoa(my_pid, buffer);
        write(1, "Soy el HIJO. Mi PID es: ", 24);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
        exit();
    } 
    else {
        int my_pid = getpid();
        itoa(my_pid, buffer);
        write(1, "Soy el PADRE. Mi PID es: ", 25);
        write(1, buffer, strlen(buffer));
        write(1, ". Mi hijo tiene PID: ", 21);
        itoa(pidHijo, buffer);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
    }*/
    
    //--------------------------------------- 

    // --- NUEVO TEST: BLOCK / UNBLOCK ---
    
    write(1, "\n--- Iniciando Test Block/Unblock ---\n", 38);

    int child_pid = fork();

    if (child_pid < 0) {
        perror();
    } 
    else if (child_pid == 0) {
        // Lógica del HIJO
        write(1, "[HIJO] Ejecutando y a punto de bloquearme...\n", 45);
        
        block(); // El hijo se detiene aquí hasta que el padre llame a unblock
        for(int i = 0; i < 200000000; i++); // tarda unos 5-6 segundos en escribirlo
        write(1, "[HIJO] ¡Despierto! Gracias, padre. Terminando...\n", 50);
        exit();
    } 
    else {
        // Lógica del PADRE
        write(1, "[PADRE] El hijo deberia estar bloqueado ahora.\n", 47);
        
        // Esperamos un tiempo para asegurar que el hijo llegue a ejecutar block()
        for(int i = 0; i < 5000000; i++); 

        write(1, "[PADRE] Voy a desbloquear al hijo con PID: ", 43);
        itoa(child_pid, buffer);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);

        if (unblock(child_pid) < 0) {
            write(1, "Error: No se pudo desbloquear al hijo.\n", 39);
            perror();
        }

        write(1, "[PADRE] Unblock enviado. Esperando finalizacion...\n", 51);
    }

    // Bucle infinito para que el sistema no muera inmediatamente
    while(1) {
    }
}
