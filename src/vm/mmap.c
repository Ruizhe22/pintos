//
// Created by ruizhe on 24-5-15.
//

#include "mmap.h"
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


unsigned mmap_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
    const struct mmap *m = hash_entry (e, struct mmap, hash_elem);
    return hash_int((int)m->map_id);
}

bool mmap_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    const struct mmap *am = hash_entry (a, struct mmap, hash_elem);
    const struct mmap *bm = hash_entry (b, struct mmap, hash_elem);
    return am->map_id < bm->map_id;
}

bool mmap_table_init(struct hash *hash)
{
    return hash_init (hash, mmap_hash_func, mmap_less_func, NULL);
}

void mmap_destroy_action_func(struct hash_elem *e, void *aux UNUSED)
{
    struct mmap *m = hash_entry (e, struct mmap, hash_elem);
    free(m);
}

void mmap_table_destroy(struct hash *hash)
{
    hash_destroy(hash, mmap_destroy_action_func);
}

struct mmap *create_insert_mmap(struct thread *thread, struct file *file, void *upage)
{

    int length = file_length (file);
    int page_volume = (length+PGSIZE-1)/PGSIZE;
    if(!check_mmap_valid(length, page_volume, upage)) return NULL;
    struct mmap *mmap = (struct mmap *)malloc(sizeof(struct mmap));
    mmap->file = file_reopen(file);
    mmap->read_bytes = length;
    mmap->page_volume = page_volume;
    mmap->zero_bytes = (mmap->page_volume * PGSIZE) - mmap->read_bytes;
    mmap->map_id = thread->map_id++;
    mmap->start_upage = upage;
    hash_insert(&thread->mmap_table, &mmap->hash_elem);
    return mmap;
}

bool mmap_set_page(struct mmap *mmap)
{
    struct thread *thread = thread_current();
    off_t ofs = 0;
    struct file *file = mmap->file;
    file_seek (file,ofs);
    uint32_t read_bytes = mmap->read_bytes;
    uint32_t zero_bytes = mmap->zero_bytes;
    void *upage = mmap->start_upage;
    while (read_bytes > 0 || zero_bytes > 0)
    {
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;
        struct page *page = create_insert_page(thread, file, ofs, page_read_bytes, page_zero_bytes, PAGE_MMAP, upage, PAGE_FILE);
        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        ofs += page_read_bytes;
        upage += PGSIZE;
    }
    return true;
}
/* evoked by create_insert_mmap()
 * check not in stack */
bool check_mmap_valid(int length, int page_volume, void *addr)
{
    if(length == 0) return false;
    if(addr==0) return false;
    if(pg_ofs (addr) != 0) return false;
    for(int i = 0; i < page_volume; ++i){
        struct page *page = find_page(&thread_current()->page_table, addr);
        if (page) {
            return false;
        }
        addr += PGSIZE;
    }
    return true;

}

void write_mmap(struct page *page, struct frame *frame)
{
    filesys_lock_acquire();
    file_write_at(page->file.file, frame->kpage,
                  page->file.read_bytes,
                  page->file.file_offset);
    filesys_lock_release();
}