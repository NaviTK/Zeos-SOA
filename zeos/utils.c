#include <utils.h>
#include <types.h>

#include <mm_address.h>

void copy_data(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
}
/* Copia de espacio de usuario a espacio de kernel, devuelve 0 si ok y -1 si error*/
int copy_from_user(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
  return 0;
}
/* Copia de espacio de kernel a espacio de usuario, devuelve 0 si ok y -1 si error*/
int copy_to_user(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
  return 0;
}

/* access_ok: Checks if a user space pointer is valid
 * @type:  Type of access: %VERIFY_READ or %VERIFY_WRITE. Note that
 *         %VERIFY_WRITE is a superset of %VERIFY_READ: if it is safe
 *         to write to a block, it is always safe to read from it
 * @addr:  User space pointer to start of block to check
 * @size:  Size of block to check
 * Returns true (nonzero) if the memory block may be valid,
 *         false (zero) if it is definitely invalid
 */
int access_ok(int type, const void * addr, unsigned long size)
{
  unsigned long addr_ini, addr_fin;

  addr_ini=(((unsigned long)addr)>>12);
  addr_fin=((((unsigned long)addr)+size)>>12);
  if (addr_fin < addr_ini) return 0; //This looks like an overflow ... deny access

  switch(type)
  {
    case VERIFY_WRITE:
      /* Should suppose no support for automodifyable code */
      if ((addr_ini>=PAG_LOG_INIT_DATA)&&
          (addr_fin<=PAG_LOG_INIT_DATA+NUM_PAG_DATA))
	  return 1;
    default:
      if ((addr_ini>=PAG_LOG_INIT_DATA)&&
          (addr_fin<=(PAG_LOG_INIT_DATA+NUM_PAG_CODE+NUM_PAG_DATA)))
          return 1;
  }
  return 0;
}

void itoa_hexadecimal(int a, char *b) {
  int i = 0, i1;
  char c;
  int resto;

  if (a == 0) { 
      b[0] = '0'; 
      b[1] = 0;
      return; 
  }

  while (a > 0) {
    resto = a % 16;    
    if (resto < 10) {
      // Si es del 0 al 9, lo convertimos a texto normal
      b[i] = resto + '0';
    } else {
      // Si es del 10 al 15, le restamos 10 y le sumamos 'A' para que 10 sea 'A', 11 sea 'B', etc.
      b[i] = (resto - 10) + 'A'; 
    }
    
    a = a / 16;
    i++;
  }

  for (i1 = 0; i1 < i / 2; i1++) {
    c = b[i1];
    b[i1] = b[i - i1 - 1];
    b[i - i1 - 1] = c;
  }
  
  b[i] = 0;
}
