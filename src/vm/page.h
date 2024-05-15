//
// Created by ruizhe on 24-5-12.
//

#ifndef PINTOS_PAGE_H
#define PINTOS_PAGE_H

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


enum page_status {
    PAGE_FILE,
    PAGE_FRAME,
    PAGE_SWAP
};

struct page_file {
    struct file *file;
    uint32_t file_offset;
    uint32_t read_bytes;
    uint32_t zero_bytes;
};

struct page {
    enum page_status status;
    bool writable;  /* if true, when evicted, write back to swap, is false, read from file later */
    void *upage;    /* user virtual address */
    struct page_file file;
    struct frame *frame;    /* avaliable when in frame */
    uint32_t swap_slot;
    struct thread *thread;
    struct hash_elem hash_elem;
};

/* evoked when load, create a struct page_file in heap */
struct page *create_page(struct thread *thread, struct file *file, uint32_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, void *upage);
/* insert into hash table */
bool insert_page(struct hash *page_table, struct page *page);
struct page *create_insert_page(struct thread *thread, struct file *file, uint32_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, void *upage, enum page_status status);


struct page *find_page(struct hash *page_table, void *user_addr);

/* evoked when page fault or kernel memory access, need to find a frame,
 * then load to the frame. its in swap or file. */
bool load_page(struct page *page);
/* auxiliary functions evoked by load_page */
void link_page_frame(struct page *page,struct frame *frame);
bool load_page_from_file(struct page *page, struct frame *frame);
bool load_page_from_swap(struct page *page, struct frame *frame);


unsigned page_hash_func (const struct hash_elem *e, void *aux UNUSED);
bool page_less_func (const struct hash_elem *a,const struct hash_elem *b, void *aux UNUSED);

/* evoked in thread_create */
bool page_table_init(struct hash *hash);

/* evoked in thread_exit */
void page_table_destroy(struct hash *hash);



#endif //PINTOS_PAGE_H
