//
// Created by ruizhe on 24-5-15.
//

#ifndef PINTOS_STACK_H
#define PINTOS_STACK_H
#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/syscall.h"
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"

#define STACK_BELOW 32
/* 8 MB */
#define STACK_LIMIT (1<<23)
#define STACK_BOTTOM ((PHYS_BASE) - (STACK_LIMIT))

void stack_allot_load(void *upage);
bool stack_addr(void *esp, void *addr);

#endif //PINTOS_STACK_H
