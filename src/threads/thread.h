#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"
/** States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /**< Running thread. */
    THREAD_READY,       /**< Not running but ready to run. */
    THREAD_BLOCKED,     /**< Waiting for an event to trigger. */
    THREAD_DYING        /**< About to be destroyed. */
  };

/** Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /**< Error value for tid_t. */

/** Thread priorities. */
#define PRI_MIN 0                       /**< Lowest priority. */
#define PRI_DEFAULT 31                  /**< Default priority. */
#define PRI_MAX 63                      /**< Highest priority. */

/* Nice value boundary */
#define NICE_MIN -20
#define NICE_INITIAL 0
#define NICE_MAX 20
/* recent_cpu in the begining */
#define RECENT_CPU_INITIAL 0
#define FRACTION 16384

/* Fixed-point real arithmetic */
/* Here x and y are fixed-point number, n is an integer */
#define CONVERT_TO_FP(n) ((n) * (FRACTION))
#define CONVERT_TO_INT_ZERO(x) ((x) / (FRACTION))
#define CONVERT_TO_INT_NEAREST(x) ((x) >= 0 ? ((x) + (FRACTION) / 2)\
                                   / (FRACTION) : ((x) - (FRACTION) / 2)\
                                   / (FRACTION))
#define ADD_X_Y(x, y) ((x) + (y))
#define SUB_X_Y(x, y) ((x) - (y))
#define ADD_X_N(x, n) ((x) + (n) * (FRACTION))
#define SUB_X_N(x, n) ((x) - (n) * (FRACTION))
#define MULTIPLE_X_Y(x, y) (((int64_t)(x)) * (y) / (FRACTION))
#define MULTIPLE_X_N(x, n) ((x) * (n))
#define DIVIDE_X_Y(x, y) (((int64_t)(x)) * (FRACTION) / (y))
#define DIVIDE_X_N(x, n) ((x) / (n))

/** A kernel thread or user process.

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
/** The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /**< Thread identifier. */
    enum thread_status status;          /**< Thread state. */
    char name[16];                      /**< Name (for debugging purposes). */
    uint8_t *stack;                     /**< Saved stack pointer. */
    int priority_origin;                       /**< Original Priority.only change when thread_set_priority only use in donations */
    int priority;        /**< default equals to priority, when donation occurs, update this value, the max donation */
    struct lock *lock_wait;             /**< Lock the thread requests using for nested donate */
    struct list locks_hold;             /**< List of locks, lock element in synch.h */

    struct list_elem allelem;           /**< List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /**< List element. */

    int nice;                             /* Thread nice value */
    int recent_cpu;                       /* Thread recent CPU */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /**< Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /**< Detects stack overflow. */
  };

/** If false (default), use round-robin scheduler.
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
void thread_check_priority_yield(struct thread *t);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/** Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

bool thread_list_priority_less (const struct list_elem *a,
                                const struct list_elem *b,
                                void *aux);
bool thread_alllist_priority_less (const struct list_elem *a,
                                   const struct list_elem *b,
                                   void *aux);
int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void calculate_load_avg (void);
void calculate_recent_cpu (struct list_elem *e, void *aux UNUSED);
void calculate_priority (struct list_elem *e, void *aux UNUSED);
#endif /**< threads/thread.h */
