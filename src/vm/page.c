//
// Created by ruizhe on 24-5-12.
//

#include "page.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hash.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "frame.h"
#include "swap.h"


unsigned page_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
    const struct page *p = hash_entry (e, struct page, hash_elem);
    return hash_int((int)p->upage);
}

bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    const struct page *ap = hash_entry (a, struct page, hash_elem);
    const struct page *bp = hash_entry (b, struct page, hash_elem);
    return ap->upage < bp->upage;
}


/* evoked in thread_create */
bool page_table_init(struct hash *hash)
{
    return hash_init (hash, page_hash_func, page_less_func, NULL);
}

static void page_destroy_action_func(struct hash_elem *e, void *aux UNUSED)
{
    struct page *page = hash_entry(e, struct page, hash_elem);
    if (page->status == PAGE_FRAME && page->frame)
    {
        destroy_frame(page->frame);
    }
    free(page);
}

/* evoked in thread_exit */
void page_table_destroy(struct hash *hash)
{
    hash_destroy(hash, page_destroy_action_func);
}


/** in process.c */
/* evoked many times when lazy load, create struct page_file in heap */
struct page *create_page(struct thread *thread, struct file *file, uint32_t ofs, uint32_t read_bytes, uint32_t zero_bytes, enum page_property property, void *upage)
{
    struct page *page = (struct page *)malloc(sizeof(struct page));
    page->file.file = file;
    page->file.file_offset = ofs;
    page->file.read_bytes = read_bytes;
    page->file.zero_bytes = zero_bytes;
    page->property = property;
    page->upage = upage;
    page->status = PAGE_FILE;
    page->frame = NULL;
    page->thread = thread;
    return page;
}

/* insert into hash table */
bool insert_page(struct hash *page_table, struct page *page)
{
    return (hash_insert(page_table, &page->hash_elem) == NULL);
}

struct page *create_insert_page(struct thread *thread, struct file *file, uint32_t ofs, uint32_t read_bytes, uint32_t zero_bytes, enum page_property property, void *upage, enum page_status status)
{
    struct page *page = (struct page *)malloc(sizeof(struct page));
    page->file.file = file;
    page->file.file_offset = ofs;
    page->file.read_bytes = read_bytes;
    page->file.zero_bytes = zero_bytes;
    page->property = property;
    page->upage = upage;
    page->status = status;
    page->frame = NULL;
    page->thread = thread;
    hash_insert(&thread->page_table, &page->hash_elem);
    return page;
}

struct page *find_page(struct hash *page_table, void *user_addr)
{
    struct page page_tmp;
    page_tmp.upage = (void *) ((uintptr_t) user_addr & ~PGMASK);
    struct hash_elem *e = hash_find(page_table, &page_tmp.hash_elem);
    if (!e) return NULL;
    else return hash_entry(e, struct page, hash_elem);
}

/* evoked when page fault or kernel memory access, need to find a frame,
 * then load it to the frame, in swap or file.
 * add the page to the process's address space.
 **/
bool load_page(struct page *page)
{
    frame_table_lock_acquire();
    if(page->status == PAGE_FRAME || pagedir_get_page(page->thread->pagedir, page->upage) != NULL) {
        frame_table_lock_release();
        return true;
    }
    else {
        struct frame *frame = allot_frame();
        link_page_frame(page, frame);
        if (page->status == PAGE_SWAP) {
            if(!load_page_from_swap(page, frame)) {
                frame_table_lock_release();
                return false;
            }
        } else if (page->status == PAGE_FILE) {
            if(!load_page_from_file(page, frame)) {
                frame_table_lock_release();
                return false;
            }
        }

        /* enable user page dir */
        if (!pagedir_set_page(page->thread->pagedir, page->upage, frame->kpage, page->property)){
            ASSERT(false);
            evict_frame(frame);
            frame_table_lock_release();
            return false;
        }
        page->status = PAGE_FRAME;
        page->frame->pin = false;
        frame_table_lock_release();
        return true;
    }
}

/** auxiliary functions evoked by load_page */

/* insert frame into the frame_table.
 * need to acquire the lock, and set pin to true
 * not set page->status until load finish.
 * */
extern struct list frame_table;
void link_page_frame(struct page *page,struct frame *frame)
{
    page->frame = frame;
    frame->page = page;
    frame->thread = page->thread;
    frame->pin = true;
    /* already acquire the lock */
    list_push_back(&frame_table, &frame->elem);
}

/* fill in the frame */
bool load_page_from_file(struct page *page, struct frame *frame)
{
    //printf("frame %p\n", frame->kpage);
    struct page_file *pfile = &page->file;
    file_seek(pfile->file, pfile->file_offset);
    void *kpage = frame->kpage;
    void *upage = page->upage;
    if (file_read(pfile->file, kpage, pfile->read_bytes) != (int) pfile->read_bytes) {
        evict_frame(frame);
        return false;
    }
    memset(kpage + pfile->read_bytes, 0, pfile->zero_bytes);
    return true;
}



bool load_page_from_swap(struct page *page, struct frame *frame)
{
    read_swap(page,frame);
    return true;
}


