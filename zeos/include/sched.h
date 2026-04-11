/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>

#define NR_TASKS 10
#define KERNEL_STACK_SIZE	1024

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  int PID;			/* Process ID. This MUST be the first field of the struct. */
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;
  
  unsigned long quantum; // quant de temps pasa fins al canvi de context
  unsigned long kernel_esp; // pila del sistema

  struct task_struct* parent; // punter al pare
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};


#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

extern char initial_stack[KERNEL_STACK_SIZE];
#define INITIAL_ESP             (DWord) &initial_stack[KERNEL_STACK_SIZE]

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

void inner_task_switch(union task_union *new);

struct task_struct * current();

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;
int allocate_DIR(struct task_struct *t);
void set_quantum(struct task_struct *t, int new_quantum);

extern struct task_struct *idle_task;
extern struct task_struct *init_task;

void task_switch(union task_union *new);

#endif  /* __SCHED_H__ */
