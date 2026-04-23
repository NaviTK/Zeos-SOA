/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>

#define NR_TASKS 10
#define KERNEL_STACK_SIZE    1024

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  int PID;                /* Process ID. This MUST be the first field of the struct. */
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;
  //Block i Unblock
  int pending_unblocks;                 //unblocks pendents
  struct list_head children_blocked;    //Children blocked
  struct list_head children_unblocked;  //Children no blocked
  //Node de germans -> Node que "es posa" a la llista de fills del para (per tant la "llista" esta fromada per els fills d'un proces en concret)
  struct list_head sibiling;
  unsigned long quantum; // quant de temps pasa fins al canvi de context
  unsigned long kernel_esp; // pila del sistema

  struct task_struct* parent; // punter al pare
  struct list_head task_list; // llista de tots els processos
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

#define KERNEL_ESP(t)       (DWord) &(t)->stack[KERNEL_STACK_SIZE]

/* ------------------------------------------------------------- */
/* ¡CORRECCIÓN VITAL! initial_stack DEBE ser unsigned long       */
/* para que ocupe 4096 bytes (4KB) en lugar de solo 1024 bytes.  */
/* ------------------------------------------------------------- */
extern unsigned long initial_stack[KERNEL_STACK_SIZE] __attribute__((__aligned__(4096)));
#define INITIAL_ESP         (DWord) &initial_stack[KERNEL_STACK_SIZE]

/* Inicialitza les dades del proces inicial */
void init_task1(void);
void init_idle(void);
void init_sched(void);
void inner_task_switch(union task_union *new);
struct task_struct *list_head_to_task_struct(struct list_head *l);
struct task_struct * current();
void schedule();
page_table_entry * get_PT (struct task_struct *t) ;
page_table_entry * get_DIR (struct task_struct *t) ;
int allocate_DIR(struct task_struct *t);
void set_quantum(struct task_struct *t, int new_quantum);

extern struct task_struct *idle_task;
extern struct task_struct *init_task;
extern struct list_head list_tasks;

struct task_struct *alloc_pcb();
void free_pcb(struct task_struct *p);

void task_switch(union task_union *new);

/* Añadimos la firma de nuestra función en ensamblador */
void cambio_pila(unsigned long *current_addr, unsigned long new_kesp);

void schedule();
void sched_next_rr();
void update_sched_data_rr();
int needs_sched_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue);

void set_quantum(struct task_struct *t, int new_quantum);
int get_quantum(struct task_struct *t);

#endif  /* __SCHED_H__ */
