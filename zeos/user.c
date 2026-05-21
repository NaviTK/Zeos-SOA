#include <libc.h>

char buf[128];
char num[16];

// TEST 1: Verificación de direcciones en 8MB
void test_m3_addresses() {
    int local_var;
    write(1, "\n--- M3 Address Verification ---\n", 33);
    write(1, "L_USER_START expected at 0x800000 (8MB)\n", 40);

    write(1, "Local var (Stack) addr: 0x", 26);
    itoa_hex((unsigned int)&local_var, num);
    write(1, num, strlen(num));
   
    write(1, "\nGlobal var (Data) addr: 0x", 27);
    itoa_hex((unsigned int)&buf, num);
    write(1, num, strlen(num));
   
    write(1, "\nCode addr (test_m3):    0x", 27);
    itoa_hex((unsigned int)test_m3_addresses, num);
    write(1, num, strlen(num));
    write(1, "\n", 1);
}

// TEST 2: Estrés de memoria
void test_m3_stress() {
    int pids[128]; // Array para guardar los PIDs y desbloquearlos luego
    int count = 0;
    char buf[16];
    write(1, "\n--- M3 Stress Test (Block/Unblock) ---\n", 40);
    write(1, "Allocating all 8MB of RAM...\n", 29);
    while (count < 128) {
        int pid = fork();
        if (pid == 0) {
            // El hijo se bloquea inmediatamente para NO liberar memoria
            block(); 
            exit();
        } else if (pid < 0) {
            // Memoria llena
            write(1, "\nMemory FULL! Alive children: ", 30);
            itoa(count, buf);
            write(1, buf, strlen(buf));
            write(1, "\n", 1);
            break;
        } else {
            pids[count] = pid;
            count++;
            if (count % 10 == 0) { itoa(count, buf); write(1, buf, strlen(buf)); write(1, " ", 1); }
        }
    }
    // Una vez llena la memoria, liberamos a todos
    write(1, "Cleaning up memory (unblocking all)...\n", 39);
    for (int i = 0; i < count; i++) {
        unblock(pids[i]);
    }
    write(1, "Done. Memory is free again.\n", 28);
}

// TEST 3: Lectura simple
void test_read_simple() {
    write(1, "\n--- Simple Read Test ---\n", 26);
    int pid = fork();
    if (pid == 0) {
        write(1, "Child waiting for 5 chars...\n", 29);
        int n = read(buf, 5);
        write(1, "Child got: [", 12);
        write(1, buf, n);
        write(1, "]\n", 2);
        exit();
    }
    read(buf, 1);
}

// TEST 4: Multiproceso
void test_read_multiprocess() {
    write(1, "\n--- Multiprocess Read Test ---\n", 32);
    int pid1 = fork();
    if (pid1 == 0) {
    	write(1, "C1 waiting for 3 chars: ", 24);
    	write(1, "\n", 1);
        int n = read(buf, 3);
        write(1, "C1 got: [", 9); write(1, buf, n); write(1, "]\n", 2);
        exit();
    }
    int pid2 = fork();
    if (pid2 == 0) {
    	write(1, "C2 waiting for 5 chars: ", 24);
    	write(1, "\n", 1);
        int n = read(buf, 5);
        write(1, "C2 got: [", 9); write(1, buf, n); write(1, "]\n", 2);
        exit();
    }
    int t = gettime();
    while ((gettime() - t) < 200); //yield para que el padre no se bloquee antes que los hijos
    write(1, "Parent blocking first input(1 char)\n", 36);
    read(buf, 1);
    write(1, "Parent unblocked. Ending test\n", 30);
}

// Helper: read an integer from keyboard (digit by digit, echo, confirm with '.')
int read_int() {
    int val = 0;
    char c;
    while (1) {
        read(&c, 1);
        if (c == '.') break;  // Period to confirm (Enter/Space not in char_map)
        if (c >= '0' && c <= '9') {
            write(1, &c, 1);  // echo the digit
            val = val * 10 + (c - '0');
        }
    }
    write(1, " ", 1);
    return val;
}

// TEST 5: gotoxy + set_color (interactive)
void test_screen_syscalls() {
    set_color(2, 0);  // default green on black
    write(1, "\n--- Screen Test (q to quit) ---\n", 33);
    write(1, "(type digits then '.' to confirm)\n", 34);

    write(1, "X col (0-79): ", 14);
    int px = read_int();

    write(1, "\nY row (0-24): ", 15);
    int py = read_int();

    write(1, "\nFG color (0-15): ", 18);
    int fg = read_int();

    write(1, "\nBG color (0-15): ", 18);
    int bg = read_int();

    write(1, "\nSize of text to write: ", 25);
    int size = read_int();
    
    write(1, "\nText to write: ", 16);
    int n = read(buf, size);

    if (gotoxy(px, py) < 0) {
        write(1, "Error: invalid position\n", 24);
        set_color(2, 0);
        return;
    }
    if (set_color(fg, bg) < 0) {
        write(1, "Error: invalid color\n", 21);
        set_color(2, 0);
        return;
    }
    
    write(1, buf, n);

    set_color(2, 0);  // restore default
}



int __attribute__ ((__section__(".text.main"))) main(void)
{
    while (1) {
        write(1, "\n======= ZeOS MENU =======\n", 27);
        write(1, "1. M3 Addresses\n", 16);
        write(1, "2. M3 Stress\n", 13);
        write(1, "3. Read Simple\n", 15);
        write(1, "4. Read Multi\n", 14);
        write(1, "5. Screen (gotoxy/color)\n", 25);
        write(1, "Selection: ", 11);

        read(buf, 1);
        if (buf[0] == '1') test_m3_addresses();
        else if (buf[0] == '2') test_m3_stress();
        else if (buf[0] == '3') test_read_simple();
        else if (buf[0] == '4') test_read_multiprocess();
        else if (buf[0] == '5') test_screen_syscalls();
    }
    return 0;
}
