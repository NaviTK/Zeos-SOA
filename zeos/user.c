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


// TEST 6: Shared Memory (shmat)
void test_shmat() {
    write(1, "\n--- Shared Memory Test ---\n", 28);

    write(1, "--- PART 1: Auto-find address (NULL) ---\n", 41);
    // 1. Attach shared page 0 with auto-find
    int *p = (int*)shmat(0, (void*)0);
    if (p == (int*)-1) {
        write(1, "ERROR: shmat(0, NULL) failed\n", 29);
        return;
    }
    write(1, "Parent shmat(0,NULL) -> 0x", 26);
    itoa_hex((unsigned int)p, num);
    write(1, num, strlen(num));
    write(1, "\n", 1);

    // 2. Write a magic value
    *p = 12345;
    write(1, "Parent wrote 12345\n", 19);

    // 3. Fork child to read from same shared page
    int pid = fork();
    if (pid == 0) {
        // Child: attach same shared page 0
        int *cp = (int*)shmat(0, (void*)0);
        if (cp == (int*)-1) {
            write(1, "Child: shmat failed\n", 20);
            exit();
        }
        write(1, "Child shmat(0,NULL) -> 0x", 25);
        itoa_hex((unsigned int)cp, num);
        write(1, num, strlen(num));
        write(1, "\n", 1);

        // Read the value written by parent
        char vbuf[16];
        itoa(*cp, vbuf);
        write(1, "Child read: ", 12);
        write(1, vbuf, strlen(vbuf));
        if (*cp == 12345) write(1, " OK!\n", 5);
        else write(1, " MISMATCH!\n", 11);
        exit();
    }

    // 4. Wait for child
    int t = gettime();
    while ((gettime() - t) < 150);

    write(1, "\n--- PART 2: Forced address (0x830000) ---\n", 43);
    // We will use shared page ID 1 and force address 0x830000 for both
    int *p_forced = (int*)shmat(1, (void*)0x830000);
    if (p_forced == (int*)-1) {
        write(1, "ERROR: shmat(1, 0x830000) failed\n", 33);
        return;
    }
    write(1, "Parent shmat(1, 0x830000) -> 0x", 31);
    itoa_hex((unsigned int)p_forced, num);
    write(1, num, strlen(num));
    write(1, "\n", 1);

    *p_forced = 67890;
    write(1, "Parent wrote 67890 at forced address\n", 37);

    pid = fork();
    if (pid == 0) {
        // Child maps the same shared page ID 1 to the exact same forced address 0x830000
        int *cp_forced = (int*)shmat(1, (void*)0x830000);
        if (cp_forced == (int*)-1) {
            write(1, "Child: shmat forced failed\n", 27);
            exit();
        }
        write(1, "Child shmat(1, 0x830000) -> 0x", 30);
        itoa_hex((unsigned int)cp_forced, num);
        write(1, num, strlen(num));
        write(1, "\n", 1);

        char vbuf[16];
        itoa(*cp_forced, vbuf);
        write(1, "Child read: ", 12);
        write(1, vbuf, strlen(vbuf));
        if (*cp_forced == 67890) write(1, " OK!\n", 5);
        else write(1, " MISMATCH!\n", 11);
        exit();
    }

    t = gettime();
    while ((gettime() - t) < 150);

    // 5. Test error cases
    write(1, "\n--- PART 3: Error validation ---\n", 34);
    int *bad = (int*)shmat(99, (void*)0);
    if (bad == (int*)-1) write(1, "shmat(99,NULL) -> error OK\n", 27);
    else write(1, "shmat(99,NULL) -> should have failed!\n", 38);

    int *bad2 = (int*)shmat(1, (void*)0x801001);
    if (bad2 == (int*)-1) write(1, "shmat(1,0x801001) -> error OK\n", 30);
    else write(1, "shmat(1,0x801001) -> should have failed!\n", 41);

    write(1, "--- Shared Memory Test Done ---\n", 31);
}

// TEST 7: shmdt y shmrm
void test_shmdt_shmrm() {
    write(1, "\n--- shmdt/shmrm Test ---\n", 26);

    // PART A: shmat + shmdt basico
    write(1, "A) shmat(2,NULL) + shmdt\n", 25);
    int *p = (int*)shmat(2, (void*)0);
    if (p == (int*)-1) { write(1, "ERROR: shmat(2,NULL) fallo\n", 27); return; }
    *p = 999;
    write(1, "Escrito 999 en shm[2]\n", 22);
    int r = shmdt((void*)p);
    if (r == 0) write(1, "shmdt OK\n", 9);
    else        write(1, "ERROR: shmdt fallo\n", 19);

    // Verificar que el frame sigue activo (refs=0 pero sin shmrm)
    int *p2 = (int*)shmat(2, (void*)0);
    if (p2 == (int*)-1) { write(1, "ERROR: re-shmat fallo\n", 22); return; }
    char vbuf[16];
    itoa(*p2, vbuf);
    write(1, "Re-leido: ", 10);
    write(1, vbuf, strlen(vbuf));
    if (*p2 == 999) write(1, " OK (frame persistio)\n", 22);
    else            write(1, " FALLO (valor distinto)\n", 24);
    shmdt((void*)p2);

    // PART B: shmrm con ref=0 => limpieza inmediata
    write(1, "B) shmrm con refs=0: limpieza inmediata\n", 40);
    int *p3 = (int*)shmat(3, (void*)0);
    if (p3 == (int*)-1) { write(1, "ERROR: shmat(3) fallo\n", 22); return; }
    *p3 = 42;
    shmdt((void*)p3);
    int r2 = shmrm(3);
    if (r2 == 0) write(1, "shmrm(3) OK (limpieza inmediata)\n", 33);
    else         write(1, "ERROR: shmrm(3) fallo\n", 22);
    // shmat(3) ahora debe fallar (pagina libre pero sin frame)
    // Nota: shmrm libera el frame, pero shmat puede volver a asignar uno nuevo
    // Solo verificamos que shmrm no crashea el sistema

    // PART C: shmrm diferido (proceso hijo mantiene la pagina activa)
    write(1, "C) shmrm diferido (hijo activo)\n", 32);
    int *p4 = (int*)shmat(4, (void*)0);
    if (p4 == (int*)-1) { write(1, "ERROR: shmat(4) fallo\n", 22); return; }
    *p4 = 77777;
    write(1, "Padre escribio 77777\n", 21);

    int pid = fork();
    if (pid == 0) {
        // El padre llamara shmrm mientras nosotros tenemos el mapeo heredado
        // Esperamos un poco para que el padre ejecute shmrm
        int t = gettime();
        while ((gettime() - t) < 100);
        // Leemos: debe seguir valido
        char hbuf[16];
        itoa(*p4, hbuf);
        write(1, "Hijo lee: ", 10);
        write(1, hbuf, strlen(hbuf));
        if (*p4 == 77777) write(1, " OK\n", 4);
        else              write(1, " FALLO\n", 7);
        shmdt((void*)p4);
        exit();
    }

    // Padre: marca para borrado mientras hijo sigue activo
    int r3 = shmrm(4);
    if (r3 == 0) write(1, "Padre: shmrm(4) OK (diferido)\n", 30);
    else         write(1, "ERROR: shmrm(4) fallo\n", 22);
    // shmat(4) debe fallar ahora (marcado para borrado)
    int *bad = (int*)shmat(4, (void*)0);
    if (bad == (int*)-1) write(1, "shmat(4) tras shmrm -> error OK\n", 32);
    else                 write(1, "shmat(4) tras shmrm -> deberia fallar!\n", 38);
    // Desconectamos la del padre
    shmdt((void*)p4);

    int t2 = gettime();
    while ((gettime() - t2) < 200);

    // PART D: errores esperados
    write(1, "D) Casos de error\n", 18);
    int e1 = shmdt((void*)0x801000);
    if (e1 == -1) write(1, "shmdt(no-mapeada) -> error OK\n", 30);
    else          write(1, "shmdt sin mapeo -> deberia fallar!\n", 35);
    int e2 = shmrm(99);
    if (e2 == -1) write(1, "shmrm(99) -> error OK\n", 22);
    else          write(1, "shmrm(99) -> deberia fallar!\n", 29);
    int e3 = shmdt((void*)0x800123); // no alineada
    if (e3 == -1) write(1, "shmdt(no-alineada) -> error OK\n", 31);
    else          write(1, "shmdt(no-alineada) -> deberia fallar!\n", 37);

    write(1, "--- shmdt/shmrm Test Done ---\n", 30);

    // PART E: sys_exit limpia pagina shmrm (sin shmdt explicito)
    write(1, "E) sys_exit + shmrm: sin shmdt explicito\n", 41);
    // 1. Padre e hijo adjuntan SHM id=5
    int *p5 = (int*)shmat(5, (void*)0);
    if (p5 == (int*)-1) { write(1, "ERROR: shmat(5) fallo\n", 22); return; }
    *p5 = 55555;
    write(1, "Padre escribio 55555 en shm[5]\n", 31);

    int pid2 = fork();
    if (pid2 == 0) {
        // El hijo hereda el mapeo, espera y hace EXIT sin shmdt
        int t3 = gettime();
        while ((gettime() - t3) < 80);
        // verificamos que podemos leer
        char hbuf2[16];
        itoa(*p5, hbuf2);
        write(1, "Hijo lee: ", 10);
        write(1, hbuf2, strlen(hbuf2));
        if (*p5 == 55555) write(1, " OK\n", 4);
        else              write(1, " FALLO\n", 7);
        // EXIT sin shmdt: sys_exit debe llamar cleanup_shared_page
        exit();
    }

    // Padre: marca shmrm mientras hijo vive (refs=2: padre+hijo)
    int r4 = shmrm(5);
    if (r4 == 0) write(1, "Padre: shmrm(5) OK (diferido, refs>0)\n", 38);
    else         write(1, "ERROR: shmrm(5) fallo\n", 22);

    // Padre hace shmdt (refs baja a 1, hijo aun vive)
    shmdt((void*)p5);
    write(1, "Padre: shmdt(5) hecho (refs=1)\n", 31);

    // Esperamos a que el hijo termine con exit() sin shmdt
    // Tras el exit del hijo: refs=0 y marked_rm=1 => cleanup_shared_page en sys_exit
    int t4 = gettime();
    while ((gettime() - t4) < 200);

    // Verificacion: shmat(5) debe dar una pagina NUEVA (frame limpio, valor = 0)
    int *p6 = (int*)shmat(5, (void*)0);
    if (p6 == (int*)-1) {
        write(1, "RESULT: shmat(5) fallo => frame fue liberado OK\n", 48);
    } else {
        char vbuf2[16];
        itoa(*p6, vbuf2);
        write(1, "RESULT: shmat(5) -> valor=", 26);
        write(1, vbuf2, strlen(vbuf2));
        if (*p6 == 0) write(1, " => frame zeroed OK (sys_exit limpio)\n", 38);
        else          write(1, " => FALLO: valor antiguo sobrevivio!\n", 36);
        shmdt((void*)p6);
    }

    // PART F: escenarios faltantes
    // F1: bidireccional — hijo escribe, padre lee
    write(1, "F1) Hijo escribe, padre lee\n", 28);
    int *pF = (int*)shmat(6, (void*)0);
    if (pF == (int*)-1) { write(1, "ERROR: shmat(6)\n", 16); return; }
    *pF = 0;
    int pidF = fork();
    if (pidF == 0) {
        *pF = 99999;
        write(1, "Hijo escribio 99999\n", 20);
        shmdt((void*)pF);
        exit();
    }
    int tF = gettime(); while ((gettime() - tF) < 150);
    char fvbuf[16];
    itoa(*pF, fvbuf);
    write(1, "Padre lee: ", 11); write(1, fvbuf, strlen(fvbuf));
    if (*pF == 99999) write(1, " OK\n", 4);
    else              write(1, " FALLO\n", 7);
    shmdt((void*)pF);

    // F2: hijo hace shmdt sin shmrm -> frame sigue vivo para el padre
    write(1, "F2) Hijo shmdt sin shmrm, padre sigue leyendo\n", 47);
    int *pF2 = (int*)shmat(7, (void*)0);
    if (pF2 == (int*)-1) { write(1, "ERROR: shmat(7)\n", 16); return; }
    *pF2 = 11111;
    int pidF2 = fork();
    if (pidF2 == 0) {
        shmdt((void*)pF2);   // hijo desconecta (refs: 2->1), sin shmrm
        write(1, "Hijo: shmdt(7) hecho\n", 21);
        exit();
    }
    int tF2 = gettime(); while ((gettime() - tF2) < 150);
    // El padre aun debe poder leer (frame no liberado: refs=1, no shmrm)
    char f2buf[16];
    itoa(*pF2, f2buf);
    write(1, "Padre lee tras shmdt hijo: ", 27); write(1, f2buf, strlen(f2buf));
    if (*pF2 == 11111) write(1, " OK (frame vivo)\n", 17);
    else               write(1, " FALLO\n", 7);
    shmdt((void*)pF2);

    // F3: varios hijos heredan -> refs correctos
    write(1, "F3) Dos hijos heredan shm[8], refs correctos\n", 46);
    int *pF3 = (int*)shmat(8, (void*)0);
    if (pF3 == (int*)-1) { write(1, "ERROR: shmat(8)\n", 16); return; }
    *pF3 = 33333;
    // Fork hijo1
    int pidH1 = fork();
    if (pidH1 == 0) {
        int tH1 = gettime(); while ((gettime() - tH1) < 50);
        char h1buf[16]; itoa(*pF3, h1buf);
        write(1, "H1 lee: ", 8); write(1, h1buf, strlen(h1buf));
        if (*pF3 == 33333) write(1, " OK\n", 4); else write(1, " FALLO\n", 7);
        shmdt((void*)pF3);
        exit();
    }
    // Fork hijo2
    int pidH2 = fork();
    if (pidH2 == 0) {
        int tH2 = gettime(); while ((gettime() - tH2) < 80);
        char h2buf[16]; itoa(*pF3, h2buf);
        write(1, "H2 lee: ", 8); write(1, h2buf, strlen(h2buf));
        if (*pF3 == 33333) write(1, " OK\n", 4); else write(1, " FALLO\n", 7);
        shmdt((void*)pF3);
        exit();
    }
    // Padre espera a que ambos hijos terminen, luego verifica que el frame sigue vivo
    int tF3 = gettime(); while ((gettime() - tF3) < 200);
    char f3buf[16]; itoa(*pF3, f3buf);
    write(1, "Padre tras hijos: ", 18); write(1, f3buf, strlen(f3buf));
    if (*pF3 == 33333) write(1, " OK (refs correctos)\n", 21);
    else               write(1, " FALLO\n", 7);
    shmdt((void*)pF3);

    write(1, "--- shmdt/shmrm Test Done ---\n", 30);
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
        write(1, "6. Shared Memory (shmat)\n", 25);
        write(1, "7. shmdt + shmrm\n", 17);
        write(1, "Selection: ", 11);

        read(buf, 1);
        if (buf[0] == '1') test_m3_addresses();
        else if (buf[0] == '2') test_m3_stress();
        else if (buf[0] == '3') test_read_simple();
        else if (buf[0] == '4') test_read_multiprocess();
        else if (buf[0] == '5') test_screen_syscalls();
        else if (buf[0] == '6') test_shmat();
        else if (buf[0] == '7') test_shmdt_shmrm();
    }
    return 0;
}
