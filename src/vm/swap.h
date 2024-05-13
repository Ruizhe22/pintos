//
// Created by ruizhe on 24-5-14.
//

#ifndef PINTOS_SWAP_H
#define PINTOS_SWAP_H
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hash.h>
#include <bitmap.h>
#include "devices/block.h"
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
#include "frame.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

/* evoked in pintos init */
void swap_init();

/* select a swap slot
 * write the frame into the sector
 * set page->swap_slot
 * */
void write_swap(struct page *page);

/* swap slot is in page->swap_slot
 * read into the frame
 * clear the swap slot
 * */
void read_swap(struct page *page);



#endif //PINTOS_SWAP_H
