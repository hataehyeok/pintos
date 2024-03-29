#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "synch.h"
#include "lib/kernel/hash.h"
#include "vm/page.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

//floating point type
typedef int fp_t;

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* Kernel access block for parent-child relationship */
struct PCB
{
   struct semaphore sema_wait_for_load;
   struct semaphore sema_wait_for_exit;
   struct semaphore sema_wait_for_destroy;
   bool child_loaded;
   int exit_code;
   
   /* file descriptor */
   struct file **fdt;
   int next_fd;
};

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* Priority donation */
    struct list donators_list;
    struct list_elem d_elem;
    struct lock *wait_on_lock;
    int original_priority;

    /* mlfqs variable */
    int nice;
    fp_t recent_cpu;

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */

    /* for parent-child relationship */
    struct thread *parent_process;
    struct list child_process_list;
    struct list_elem child_process_elem;
    struct PCB *pcb;
    bool child_load_success;
    struct file *executable;
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
    
    /* Virtual Memory */
    struct hash vm_table;
    struct list mmap_list;

    /* wake up time used in priority scheduling*/
    int64_t wakeup_time;
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

// My preemption function
void thread_preemption (void);

// My sleep list functions
void thread_sleep (int64_t ticks);
void thread_wakeup(const int64_t current_time);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

// My mlfqs functions
int mlfqs_calculate_priority (fp_t recent_cpu, int nice);
fp_t mlfqs_calculate_recent_cpu (fp_t recent_cpu, int nice);
void update_load_avg (void);
void increment_recent_cpu_on_every_tick (void);
void mlfqs_set_priority_of_all_thread (void);
void mlfqs_set_recent_cpu_of_all_thread (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

// My helper function for sorting list with priority
bool less_wakeup_time (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
bool set_list_to_priority_descending (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
void update_current_thread_priority_with_donators(void);

/* for parent-child relationship */
struct thread *get_child_thread (tid_t tid);

#endif /* threads/thread.h */
