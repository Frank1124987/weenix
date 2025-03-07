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
#include "util/debug.h"

/* Approximate numbers taken from various points in Linux kernel */
#define LOOPS_PER_JIFFY (1 << 12)
#define HZ 100 /* Found this in a random place in the kernel */


/* From arch/x86/lib/delay.c in Linux kernel */
/*
 *      Precise Delay Loops for i386
 *
 *      Copyright (C) 1993 Linus Torvalds
 *      Copyright (C) 1997 Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 *      Copyright (C) 2008 Jiri Hladky <hladky _dot_ jiri _at_ gmail _dot_ com>
 *
 *      The __delay function must _NOT_ be inlined as its execution time
 *      depends wildly on alignment on many x86 processors. The additional
 *      jump magic is needed to get the timing stable on all the CPU's
 *      we have to worry about.
 */

static void __delay(unsigned long loops)
{
        __asm__ volatile(
                "	test %0,%0	\n"
                "	jz 3f		\n"
                "	jmp 1f		\n"

                ".align 16		\n"
                "1:	jmp 2f		\n"

                ".align 16		\n"
                "2:	dec %0		\n"
                "	jnz 2b		\n"
                "3:	dec %0		\n"

                : /* we don't need output */
                :"a"(loops)
        );

}

static inline void __const_udelay(unsigned long xloops)
{
        int d0;

        xloops *= 4;
        __asm__ volatile(
                "mull %%edx"
                :"=d"(xloops), "=&a"(d0)
                :"1"(xloops), "0"
                (LOOPS_PER_JIFFY * (HZ/4))
        );

        __delay(++xloops);

}

static inline void __udelay(unsigned long usecs)
{
        __const_udelay(usecs * 4295);   /* 2**32 / 1000000 */
}

static inline void __ndelay(unsigned long nsecs)
{
        __const_udelay(nsecs * 5);      /* 2**32 / 1000000000 */
}

#define udelay(n) (__builtin_constant_p(n) ? \
                   ((n) > 20000 ? panic("Delay too large!") : __const_udelay((n) * 4295)) : \
                           __udelay(n))

#define ndelay(n) (__builtin_constant_p(n) ? \
                   ((n) > 20000 ? panic("Delay too large!") : __const_udelay((n) * 5)) : \
                           __ndelay(n))
