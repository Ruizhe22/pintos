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


struct mmap{
    void *start_upage;
    int page_volume;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    mapid_t map_id;
    struct file *file;
    struct hash_elem hash_elem;
};

bool mmap_table_init(struct hash *hash);

void mmap_table_destroy(struct hash *hash);

struct mmap *create_insert_mmap(struct thread *thread, struct file *file, void *upage);

bool mmap_set_page(struct mmap *mmap);

bool check_mmap_valid(int length, int page_volume, void *addr);

void write_mmap(struct page *page, struct frame *frame);

#endif //PINTOS_MMAP_H
