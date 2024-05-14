//
// Created by ruizhe on 24-5-14.
//

#include "swap.h"
#include <debug.h>
#include <bitmap.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "userprog/tss.h"
#include "threads/vaddr.h"
#include "page.h"
#include "frame.h"

struct lock swap_lock;
struct block *swap_block_device;
struct bitmap *swap_table;

void swap_init()
{
    swap_block_device = block_get_role(BLOCK_SWAP);
    swap_table = bitmap_create( block_size(swap_block_device) / SECTORS_PER_PAGE );
    bitmap_set_all(swap_table, 0);
    lock_init(&swap_lock);
}

void read_swap(struct page *page)
{
    lock_acquire(&swap_lock);
    bitmap_flip(swap_table, page->swap_slot);
    for (int i = 0; i < SECTORS_PER_PAGE; ++i){
        block_read(swap_block_device, page->swap_slot * SECTORS_PER_PAGE + i, page->upage + i * BLOCK_SECTOR_SIZE);
    }
    lock_release(&swap_lock);
}

/* select a swap slot
 * write the frame into the sector
 * set page->swap_slot
 * */
void write_swap(struct page *page)
{
    lock_acquire(&swap_lock);
    size_t free_slot = bitmap_scan_and_flip(swap_table, 0, 1, 0);
    if (free_slot == BITMAP_ERROR){
        PANIC("Swap Partition Full");
    }

    for (int i = 0; i < SECTORS_PER_PAGE; ++i){
        block_write(swap_block_device, free_slot * SECTORS_PER_PAGE + i, page->upage + i * BLOCK_SECTOR_SIZE);
    }

    page->swap_slot = free_slot;
    lock_release(&swap_lock);

}
