//
// Created by ruizhe on 24-5-13.
//
#include "frame.h"
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
#include "mmap.h"


/* initial in thread.c */
struct lock frame_table_lock;
struct list frame_table;

/* initialize frame table and its lock */
void frame_table_init(void)
{
    list_init(&frame_table);
    lock_init(&frame_table_lock);
}

/* used in function load_page() */
struct frame *allot_frame(void)
{
    struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
    void *kpage_tmp = palloc_get_page(PAL_USER);
    struct frame *frame_tmp;
    while(kpage_tmp == NULL){
        frame_tmp = select_frame_to_evict();
        save_frame(frame_tmp);
        evict_frame(frame_tmp);
        kpage_tmp = palloc_get_page(PAL_USER);
    }
    frame->kpage = kpage_tmp;
    return frame;
}

struct frame *select_frame_to_evict(void)
{
    //frame_table_lock_acquire();
    struct list_elem *e = list_begin(&frame_table);
    struct frame *frame = list_entry(e, struct frame, elem);
    /* clock algorithm */
    while (true){
        if (!frame->pin){
            if (pagedir_is_accessed(frame->thread->pagedir, frame->page->upage)){
                pagedir_set_accessed(frame->thread->pagedir, frame->page->upage, false);
            }
            else{
                break;
            }
        }
        e = list_next(e);
        if (e == list_end(&frame_table)){
            e = list_begin(&frame_table);
        }
        frame = list_entry(e, struct frame, elem);
    }
    //frame_table_lock_release();
    return frame;
}

/* save frame content before evict it
 * mark page and frame as unavailable
 * have a semantic that once content be saved, can't be access!
 * so it flip the status, and clear pd.
 * */
bool save_frame(struct frame *frame)
{
    /* the order of statements bellow can't be change! */
    if(frame->page->property == PAGE_EXE_WRITABLE){
        frame->page->status = PAGE_SWAP;
        write_swap(frame->page, frame);
        pagedir_clear_page(frame->thread->pagedir, frame->page->upage);
        return true;
    }
    else if(frame->page->property == PAGE_MMAP){
        frame->page->status = PAGE_FILE;
        if(pagedir_is_dirty (frame->thread->pagedir, frame->page->upage)){
            /* to define it in mmap.c */
            write_mmap(frame->page, frame);
        }
        pagedir_clear_page(frame->thread->pagedir, frame->page->upage);
        return true;
    }
    else {
        frame->page->status = PAGE_FILE;
        pagedir_clear_page(frame->thread->pagedir, frame->page->upage);
        return true;
    }
}

/* put frame back to the user pool
 * remove frame entry from frame table
 * unlink frame and page
 * it assume that before being called, page status and pd is clear.
 **/
void evict_frame(struct frame *frame)
{
    list_remove(&frame->elem);
    frame->page->frame = NULL;
    palloc_free_page(frame->kpage);
    free(frame);
}

/* evoked in page_table_destroy when thread exit
 * equals to evict_frame() plus pagedir_clear_page()
 * */
void destroy_frame (struct frame *frame)
{
    list_remove(&frame->elem);
    /* these two must occur or miss at the same time */
    //palloc_free_page(frame->kpage);
    //pagedir_clear_page(frame->thread->pagedir, frame->page->upage);
    frame->page->frame = NULL;
    free(frame);
}


void frame_table_lock_acquire(void)
{
    lock_acquire(&frame_table_lock);
}

void frame_table_lock_release(void)
{
    lock_release(&frame_table_lock);
}