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

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page. Make sure that if the
 * user writes to the page it will be handled correctly. This
 * includes your shadow objects' copy-on-write magic working
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{

        uint32_t vfn = ADDR_TO_PN(vaddr);
        off_t off = PAGE_OFFSET(vaddr);
        vmarea_t* vma = vmmap_lookup(curproc->p_vmmap, vfn);
        if (vma == NULL){
                // dbg(DBG_ERROR, "vmmap_lookup return null in handle pagefault, addres: %x\n", vaddr);
                // do_exit(EFAULT);
                panic("vmmap_lookup return null in handle pagefault, addres: %x\n", vaddr);
        }
        int forwrite = 0;
        uint32_t pdflags = PD_PRESENT | PD_USER;
        uint32_t ptflags = PT_PRESENT | PT_USER;

        
        if (FAULT_EXEC & cause){
                if (!(vma->vma_prot & PROT_EXEC)){
                        do_exit(EFAULT);
                }
        }else if ((FAULT_WRITE & cause) == 0){
                if (!(vma->vma_prot & PROT_READ)){
                        do_exit(EFAULT);
                }
        }else if (FAULT_WRITE & cause){
                if (!(vma->vma_prot & PROT_WRITE)){
                        do_exit(EFAULT);
                }

                forwrite = 1;
                pdflags |= PD_WRITE;
                ptflags |= PT_WRITE;
        }

        /* !ISSUE: remember to handle COW later */

        pframe_t* target_pframe; 
        uint32_t pagenum = vfn - vma->vma_start + vma->vma_off;
        mmobj_t* cur_mmobj = vma->vma_obj;
        pframe_lookup(cur_mmobj, pagenum, forwrite, &target_pframe);

        KASSERT(target_pframe); /* this page frame must be non-NULL */
        KASSERT(target_pframe->pf_addr); /* this page frame's pf_addr must be non-NULL */
        dbg(DBG_PRINT, "(GRADING3A 5.a)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");

        if (forwrite){
                pframe_dirty(target_pframe);
        }

        int err = pt_map(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vfn), pt_virt_to_phys((uintptr_t)target_pframe->pf_addr), pdflags, ptflags);
        if (err < 0){
                do_exit(EFAULT);
        }
        

        // NOT_YET_IMPLEMENTED("VM: handle_pagefault");
}
