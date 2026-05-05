#include <libc.h>

int __attribute__ ((__section__(".text.main")))
main(void)
{
    char buf[16];
    char num[16];

    while (1) {
        write(1, "\ncomprobar uso de read (m2) pulsa 1: \n", 38);
        write(1, "comprobar cambio a 2048 pages (m3) pulsa 2: \n", 45);
        
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
            int free = get_stats(1);
            
            write(1, "Total Pages: ", 13);
            itoa(total, num);
            write(1, num, strlen(num));
            
            write(1, "\nFree Pages: ", 13);
            itoa(free, num);
            write(1, num, strlen(num));
            write(1, "\n", 1);
        }
    }
}
