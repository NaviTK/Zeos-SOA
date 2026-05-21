/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

/** Screen functions **/
/**********************/

void printc(char c);
void printc_xy(Byte x, Byte y, char c);
void printk(char *string);
int sys_gotoxy(int col, int row);
int sys_set_color(int fg, int bg);

#endif  /* __IO_H__ */
