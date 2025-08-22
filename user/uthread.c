#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"

/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct context threadContext;
};

struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(struct context*, struct context*);

void
thread_init(void)
{
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->state = RUNNABLE;
  t->threadContext.ra = (uint64)func;
  t->threadContext.sp = (uint64)t->stack + STACK_SIZE;
}

void
thread_schedule(void)
{
  struct thread *t, *next_thread = 0;

  int current_idx = current_thread - all_thread;

  for (int i = 1; i < MAX_THREAD; i++) {
    int idx = (current_idx + i) % MAX_THREAD;
    if (all_thread[idx].state == RUNNABLE) {
      next_thread = &all_thread[idx];
      break;
    }
  }

  if (next_thread == 0) {
    if (current_thread->state != FREE) {
      return;
    }
    next_thread = &all_thread[0];
  }
  
  if (current_thread != next_thread) {
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;

    if(t != &all_thread[0] && t->state != FREE) {
      t->state = RUNNABLE;
    }

    thread_switch(&t->threadContext, &current_thread->threadContext);
  }
}

void
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();

  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();

  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();

  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int
main(int argc, char *argv[])
{
  for(int i=0; i<MAX_THREAD; ++i) {
      all_thread[i].state = FREE;
  }

  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);

  while (1) {
    int has_active_threads = 0;
    for (struct thread *t = all_thread + 1; t < all_thread + MAX_THREAD; t++) {
        if (t->state != FREE) {
            has_active_threads = 1;
            break;
        }
    }

    if (!has_active_threads) {
        break;
    }

    thread_yield();
  }

  printf("uthread: all threads finished\n");
  exit(0);
}
