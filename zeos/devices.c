#include <io.h>
#include <utils.h>
#include <list.h>
#include <sched.h>
#include <devices.h>

extern struct list_head readyqueue;

/* -------------------------------------------------------------------
 * Keyboard circular buffer
 * ------------------------------------------------------------------- */
#define KEYBOARD_BUF_SIZE 256   /* must be a power of 2 */
#define KBD_MASK (KEYBOARD_BUF_SIZE - 1)

static char kbd_buf[KEYBOARD_BUF_SIZE];
static int  kbd_head = 0;   /* next position to write */
static int  kbd_tail = 0;   /* next position to read  */
static int  kbd_count = 0;  /* number of chars currently stored */

/* General-purpose blocked queue (used by sys_block / sys_unblock) */
struct list_head blocked;

/* Queue of processes blocked in sys_read() */
struct list_head kbd_blocked;

/* -------------------------------------------------------------------
 * sys_write_console — write a buffer to the screen
 * ------------------------------------------------------------------- */
int sys_write_console(char *buffer, int size)
{
  int i;
  for (i = 0; i < size; i++)
    printc(buffer[i]);
  return size;
}

/* -------------------------------------------------------------------
 * kbd_buf_read — dequeue up to n characters into dst (kernel buffer).
 * Returns the number of characters actually copied.
 * ------------------------------------------------------------------- */
int kbd_buf_read(char *dst, int n)
{
  int copied = 0;
  while (copied < n && kbd_count > 0) {
    dst[copied++] = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) & KBD_MASK;
    kbd_count--;
  }
  return copied;
}

/* -------------------------------------------------------------------
 * kbd_buf_flush — discard all characters in the circular buffer.
 * Useful to drain leftover chars from keyboard auto-repeat.
 * ------------------------------------------------------------------- */
void kbd_buf_flush(void)
{
  kbd_head = 0;
  kbd_tail = 0;
  kbd_count = 0;
}

/* -------------------------------------------------------------------
 * kbd_buf_write — enqueue one character produced by the keyboard ISR.
 * After inserting, check whether the first blocked reader now has
 * enough characters; if so, copy them to user space and wake it up.
 * ------------------------------------------------------------------- */
void kbd_buf_write(char c)
{
  /* Drop the character if the buffer is full */
  if (kbd_count >= KEYBOARD_BUF_SIZE)
    return;

  kbd_buf[kbd_head] = c;
  kbd_head = (kbd_head + 1) & KBD_MASK;
  kbd_count++;

  /* Wake the first blocked reader if it now has enough chars */
  if (!list_empty(&kbd_blocked)) {
    struct list_head *entry = list_first(&kbd_blocked);
    struct task_struct *t = list_entry(entry, struct task_struct, list);

    if (kbd_count >= t->kbd_chars_needed) {
      /* Only touch kernel data structures here — never user-space memory.
       * The copy from the ring buffer to the user buffer is done by
       * sys_read after the process resumes with its own CR3. */
      list_del(entry);
      update_process_state_rr(t, &readyqueue);
    }
  }
}
