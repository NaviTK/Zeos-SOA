#include <libc.h>

int __attribute__ ((__section__(".text.main")))
main(void)
{
    char buf[8];
    char num[4];

    write(1, "\n--- read() syscall test ---\n", 29);

    while (1) {
        write(1, "Type 5 keys: ", 13);

        int n = read(buf, 5);   /* blocks until 5 keystrokes are captured */

        write(1, "\nRead ", 6);
        itoa(n, num);
        write(1, num, strlen(num));
        write(1, " chars: [", 9);
        write(1, buf, n);
        write(1, "]\n", 2);
    }
}
