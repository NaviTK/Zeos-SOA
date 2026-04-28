#ifndef DEVICES_H__
#define  DEVICES_H__

#include <list.h>

int sys_write_console(char *buffer, int size);

/* Keyboard circular buffer API */
void kbd_buf_write(char c);  /* called by ISR — enqueue a char and wake readers */
int  kbd_buf_read(char *dst, int n); /* dequeue up to n chars; returns count */

extern struct list_head kbd_blocked; /* processes blocked in read() */

#endif /* DEVICES_H__*/
