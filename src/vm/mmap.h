//
// Created by ruizhe on 24-5-15.
//

#ifndef PINTOS_MMAP_H
#define PINTOS_MMAP_H
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

/** record information of one mmap syscall, used to lazy load and unmap pages
 * note that it is not used in load file or wirte file, because information these funcs need is saved in struct page. */
struct mmap{
    void *start_upage;   /* the addr passed by mmap_syscall, indicate the start page of the mapped contiguous space */
    int page_volume;         /* the pages number the file need to allocate */
    uint32_t read_bytes;      /* actual file size */
    uint32_t zero_bytes;      /* padding */
    mapid_t map_id;        /* corresponding to thread mapid number when syscall occurs. */
    struct file *file;        /* the reopened struct file */
    struct hash_elem hash_elem;
};

bool mmap_table_init(struct hash *hash);

void mmap_table_destroy(struct hash *hash);

struct mmap *create_insert_mmap(struct thread *thread, struct file *file, void *upage);

bool mmap_set_page(struct mmap *mmap);

bool check_mmap_valid(int length, int page_volume, void *addr);

void write_mmap(struct page *page, struct frame *frame);

struct mmap *find_mmap(struct hash *mmap_table, mapid_t map_id);

#endif //PINTOS_MMAP_H
