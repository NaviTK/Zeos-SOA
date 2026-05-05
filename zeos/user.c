#include <libc.h>

int __attribute__ ((__section__(".text.main")))
main(void)
{
    char buf[16];
    char num[16];

    while (1) {
        write(1, "\ncomprobar uso de read (m2) pulsa 1: \n", 38);
        write(1, "comprobar cambio a 2048 pages (m3) pulsa 2: \n", 45);
        write(1, "test read multiproceso (m2) pulsa 3: \n", 38);
        
        char choice;
        read(&choice, 1);
        
        if (choice == '1') {
            write(1, "\n--- read() syscall test ---\n", 29);
            write(1, "Type 5 keys: ", 13);
            int n = read(buf, 5);
            write(1, "\nRead ", 6);
            itoa(n, num);
            write(1, num, strlen(num));
            write(1, " chars: [", 9);
            write(1, buf, n);
            write(1, "]\n", 2);
        } else if (choice == '2') {
            write(1, "\n--- Memory Stats test ---\n", 27);
            int total = get_stats(0);
            int free_pages = get_stats(1);
            
            write(1, "Total Pages: ", 13);
            itoa(total, num);
            write(1, num, strlen(num));
            
            write(1, "\nFree Pages: ", 13);
            itoa(free_pages, num);
            write(1, num, strlen(num));
            write(1, "\n", 1);
        } else if (choice == '3') {
            write(1, "\n--- Multi-process read() test ---\n", 35);

            /* Fork child 1: wants 3 chars */
            int pid1 = fork();
            if (pid1 == 0) {
                /* CHILD 1 */
                int mypid = getpid();
                write(1, "Child1 (PID=", 12);
                itoa(mypid, num);
                write(1, num, strlen(num));
                write(1, ") waiting for 3 chars...\n", 24);

                int n = read(buf, 3);

                write(1, "Child1 got ", 11);
                itoa(n, num);
                write(1, num, strlen(num));
                write(1, " chars: [", 9);
                write(1, buf, n);
                write(1, "]\n", 2);
                exit();
            }

            /* Fork child 2: wants 5 chars */
            int pid2 = fork();
            if (pid2 == 0) {
                /* CHILD 2 */
                int mypid = getpid();
                write(1, "Child2 (PID=", 12);
                itoa(mypid, num);
                write(1, num, strlen(num));
                write(1, ") waiting for 5 chars...\n", 24);

                int n = read(buf, 5);

                write(1, "Child2 got ", 11);
                itoa(n, num);
                write(1, num, strlen(num));
                write(1, " chars: [", 9);
                write(1, buf, n);
                write(1, "]\n", 2);
                exit();
            }

            /* PARENT: report what was forked */
            write(1, "Parent forked PID=", 18);
            itoa(pid1, num);
            write(1, num, strlen(num));
            write(1, " and PID=", 9);
            itoa(pid2, num);
            write(1, num, strlen(num));
            write(1, "\nType chars to wake them up!\n", 28);
        }
    }
}
