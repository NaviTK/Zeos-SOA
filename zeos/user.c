#include <libc.h>

char buff[24];

int pid;

//int add(int par1, int par2);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    
  while(1) {
	// codigo usuario de prueba
 	//int n = add(1, 2);
	  
 	//Test Write
  	//if (write(1, "El siguiente write dara error de canal:\n", 40) < 0) perror();
  	//if (write(2, "Hello World!\n", 13) < 0) perror();
  	//if (write(1, "FUNCIONA!\n", 10) < 0) perror();
	//Test Write
	//if (fast_write(1, "Tambien pasara lo mismo con fast_write:\n", 40) < 0) perror();
  	//if (fast_write(2, "Hello World!\n", 13) < 0) perror();
  	if (fast_write(1, "FUNCIONA!\n", 10) < 0) perror();
	
 	// para testear el pagefault ->
 	//char* p = 0;
	//*p = 'x';
  }
}
