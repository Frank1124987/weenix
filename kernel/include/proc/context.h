/******************************************************************************/
/* Important Fall 2024 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#pragma once

#include "types.h"

#include "mm/pagetable.h"

/*
 * The function pointer to be implemented by functions which are entry
 * points for new threads.
 */
typedef void *(*context_func_t)(int, void *);

typedef struct context {
        uint32_t   c_eip; /* instruction pointer (EIP) */
        uint32_t   c_esp; /* stack pointer (ESP) */
        uint32_t   c_ebp; /* frame pointer (EBP) */

        pagedir_t *c_pdptr; /* pointer to the page directory for this proc */

        uintptr_t  c_kstack;
        size_t     c_kstacksz;
} context_t;

/**
 * Initialize the given context such that when it begins execution it
 * will execute func(arg1,arg2). When the thread returns from func it
 * will be cancelled. A kernel stack and page directory exclusive to
 * this context must also be provided.
 *
 * @param c the context to initialize
 * @param func the function which will begin executing when this
 * context is first made active
 * @param arg1 the first argument to func
 * @param arg2 the second argument to func
 * @param kstack a pointer to the kernel stack this context will use
 * @param kstacksz the size of the kernel stack
 * @param pdptr the pagetable this context will use
 */
void context_setup(context_t *c, context_func_t func, int arg1, void *arg2,
                   void *kstack, size_t kstacksz, pagedir_t *pdptr);

/**
 * Makes the given context the one currently running on the CPU. Use
 * this mainly for the initial context.
 *
 * @param c the context to make active
 */
void context_make_active(context_t *c);

/**
 * Save the current state of the machine into the old context, and begin
 * executing the new context. Used primarily by the scheduler.
 *
 * @param oldc the context to switch from
 * @param newc the context to switch to
 */
void context_switch(context_t *oldc, context_t *newc);
