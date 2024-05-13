//
// Created by ruizhe on 24-5-13.
//

#ifndef PINTOS_FRAME_H
#define PINTOS_FRAME_H
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hash.h>
#include "threads/synch.h"
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "page.h"
#include "swap.h"


/* global data structure */
extern struct lock frame_table_lock;
extern struct list frame_table;

struct frame {
    struct page *page;
    bool pin;       /* can't be evicted, indicate page is being loaded or accessed by kernel */
    void *kpage;    /* indicate physical address */
    struct thread *thread;
    struct list_elem elem;
};

/* initialize frame table and its lock */
void frame_table_init();

/* used in function load_page()
 * need to acquire new space from user pool or evict another frame.
 * construct the frame entry, only set the kpage field, !!not insert it into the frame_table.
 * */
struct frame *alloc_frame();
struct frame *select_frame_to_evict();
/* save frame content before evict it */
bool save_frame(struct frame *frame);
/* put frame back to the user pool
 * remove frame entry from frame table
 * unlink frame and page
 * */
void evict_frame(struct frame *frame);

/* global frame table lock  */
void frame_table_lock_acquire();
void frame_table_lock_release();

#endif //PINTOS_FRAME_H