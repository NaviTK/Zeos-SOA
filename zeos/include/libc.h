/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

void itoa(int a, char *b);

int strlen(char *a);

int write(int fd, char *buffer, int size);
int read(char *b, int maxchars);
int gettime();
int getpid();
void exit();
int fork();
void perror();
int block();
int unblock(int pid);
int get_stats(int type);

#endif  /* __LIBC_H__ */
