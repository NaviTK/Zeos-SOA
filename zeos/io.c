/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

#include <hardware.h>

#include <errno.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS    25

Byte x, y=19;
Byte current_color = 0x02; /* default: green on black */

void printc(char c)
{
  bochs_out(c);
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | ((Word)current_color << 8);
	Word *screen = (Word *)0xb8000;
	screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
  }
}

void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx=x;
  cy=y;
  x=mx;
  y=my;
  printc(c);
  x=cx;
  y=cy;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}

int sys_gotoxy(int col, int row)
{
  if (col < 0 || col >= NUM_COLUMNS) return -EINVAL;
  if (row < 0 || row >= NUM_ROWS)    return -EINVAL;
  x = (Byte)col;
  y = (Byte)row;
  return 0;
}

int sys_set_color(int fg, int bg)
{
  if (fg < 0 || fg > 15) return -EINVAL;
  if (bg < 0 || bg > 15) return -EINVAL;
  current_color = (Byte)((bg << 4) | (fg & 0x0F));
  return 0;
}
