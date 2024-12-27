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

#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should "return" the current break. We use this to
 * implement sbrk(0) without writing a separate syscall. Look in
 * user/libc/syscall.c if you're curious.
 *
 * You should support combined use of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
        if (addr == NULL){
                *ret = curproc->p_brk;
                return 0;
        }

        if ((uintptr_t)addr >= USER_MEM_HIGH){
                return -ENOMEM;
        }

        if (curproc->p_start_brk > addr){
                return -ENOMEM;
        }

        if (curproc->p_brk == addr){
                *ret = curproc->p_brk;
                return 0;
        }

        uint32_t start_brk_vfn = ADDR_TO_PN(curproc->p_start_brk);
        uint32_t new_brk_vfn = ADDR_TO_PN(addr);
        if (!PAGE_ALIGNED(curproc->p_start_brk)){
                start_brk_vfn++;
        }

        if (!PAGE_ALIGNED(addr)){
                new_brk_vfn++;
        }

        /*
        if addr > curproc->p_brk
                case 1: addr lies in a existing vma
                        case 1: inside data+bss region
                                check the boundary and simply set new end page
                        case 2: own heap region
                                check the boundary and simply set new end page
                case 2: addr is beyond initial region
                        vmmap_map a new vmarea

        else if addr == curproc->p_brk
                nothing happen
        else if addr < curproc->p_brk
                case 1: addr fall back to initial region
                        remove the heap vma
                case 2: still inside heap 
                        set new end page
        */

       /* !ISSUE: change the logic if fork-bomb failed */
       if (addr > curproc->p_brk){
                vmarea_t* heap_vma = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(addr));
                if (heap_vma){
                        heap_vma->vma_end = new_brk_vfn;
                        curproc->p_brk = addr;
                        *ret = addr;
                        return 0;
                }else{
                        if (ADDR_TO_PN(curproc->p_start_brk) != ADDR_TO_PN(curproc->p_brk)){
                                heap_vma = vmmap_lookup(curproc->p_vmmap, start_brk_vfn);

                                uint32_t end_page = heap_vma->vma_end;
                                if (!vmmap_is_range_empty(curproc->p_vmmap, end_page, new_brk_vfn - end_page)){
                                        return -ENOMEM;
                                }

                                heap_vma->vma_end = new_brk_vfn;
                                curproc->p_brk = addr;
                                *ret = addr;
                                return 0;
                        }else{
                                int err = vmmap_map(curproc->p_vmmap, 
                                                        NULL, 
                                                        start_brk_vfn, 
                                                        new_brk_vfn - start_brk_vfn, 
                                                        PROT_READ | PROT_WRITE, 
                                                        MAP_ANON | MAP_PRIVATE, 
                                                        0,
                                                        0,
                                                        &heap_vma );
                                
                                if (err < 0){
                                        // panic("something went wrong with vmmap_map in brk.c\n");
                                        return -1;
                                }

                                curproc->p_brk = addr;
                                *ret = addr;
                                return 0; 
                        }
                }
       }else{
                vmarea_t* heap_vma = vmmap_lookup(curproc->p_vmmap, start_brk_vfn);
                if (ADDR_TO_PN(addr) == ADDR_TO_PN(curproc->p_start_brk) && ADDR_TO_PN(curproc->p_start_brk) != ADDR_TO_PN(curproc->p_brk)){
                        // previously outside initial region and now inside, remove cur heap vma
                        vmmap_remove(curproc->p_vmmap, heap_vma->vma_start, heap_vma->vma_start - heap_vma->vma_end);
                        vmarea_t* data_vma = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(curproc->p_start_brk));
                        data_vma->vma_end = new_brk_vfn;
                        curproc->p_brk = addr;
                        *ret = addr;
                        return 0;
                }else{
                        heap_vma->vma_end = new_brk_vfn;
                        curproc->p_brk = addr;
                        *ret = addr;
                        return 0;
                }
       }


        // vmarea_t* heap_vma = vmmap_lookup(curproc->p_vmmap, start_brk_vfn);

        // if (heap_vma == NULL){
        //         int err = vmmap_map(curproc->p_vmmap, 
        //                                 NULL, 
        //                                 start_brk_vfn, 
        //                                 new_brk_vfn - start_brk_vfn, 
        //                                 PROT_READ | PROT_WRITE, 
        //                                 MAP_ANON | MAP_PRIVATE, 
        //                                 curproc->p_start_brk - PN_TO_ADDR(start_brk_vfn),
        //                                 0,
        //                                 &heap_vma );
                
        //         if (err < 0){
        //                 // panic("something went wrong with vmmap_map in brk.c\n");
        //                 return -1;
        //         }

        //         curproc->p_brk = addr;
        //         *ret = addr;
        //         return 0;
        // }else{
        //         if (addr <= curproc->p_brk){
        //                 curproc->p_brk = addr;
        //                 heap_vma->vma_end = new_brk_vfn;
        //                 *ret = addr;
        //                 return 0;
        //         }else{
        //                 uint32_t end_page = heap_vma->vma_end;
        //                 if (!vmmap_is_range_empty(curproc->p_vmmap, end_page, new_brk_vfn - end_page)){
        //                         return -ENOMEM;
        //                 }
        //                 heap_vma->vma_end = new_brk_vfn;
        //                 curproc->p_brk = addr;
        //                 *ret = addr;
        //                 return 0;
        //         }
        // }

        // NOT_YET_IMPLEMENTED("VM: do_brk");
}
