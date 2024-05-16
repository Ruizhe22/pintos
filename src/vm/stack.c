//
// Created by ruizhe on 24-5-15.
//

#include "stack.h"

/** the arg upage indicate the stack page unalloc,
 * this function is to create a new supplement page table entry,
 * set its infomation and alloc a frame to it. */
void stack_allot_load(void *upage)
{
    if(find_page(&thread_current()->page_table, upage)) return;
    struct page *page = create_insert_page(thread_current(), NULL, 0, 0, PGSIZE, PAGE_EXE_WRITABLE, upage, PAGE_FRAME);
    struct frame *frame = allot_frame();
    frame_table_lock_acquire();
    link_page_frame(page, frame);
    frame_table_lock_release();
    pagedir_set_page(page->thread->pagedir, page->upage, frame->kpage, page->property);
    frame->pin = false;
    return;
}

/** this is a design! every time load a stack page aligned ti upage,
 * immediate load all of the stack page above it, from the PHYS_BASE,
 * which ensures that the stack space is contiguous. */
void stack_load(void *upage)
{
    for(void *addr = upage; addr<PHYS_BASE; addr+=PGSIZE){
        stack_allot_load(addr);
    }
}

/* check if user address in stack area */
inline bool stack_addr(const void *esp, const void *addr)
{
    return (addr>STACK_BOTTOM)&&(addr<PHYS_BASE)&&(esp - addr <= STACK_BELOW);
}