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
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        if (!(flags & MAP_PRIVATE) && !(flags & MAP_SHARED)){
                return -EINVAL;
        }

        if (!(flags & MAP_ANON) && 
                !(flags & MAP_FIXED) &&
                !(flags & MAP_PRIVATE) &&
                !(flags & MAP_SHARED))
        {
                return -EINVAL;
        }

        /* length was 0 */
        if (len == 0){
                return -EINVAL;
        }

        /* We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary). */
        if (!PAGE_ALIGNED(addr)){
                return -EINVAL;
        }
        

        file_t* file_obj = fget(fd);

        /* fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).*/
        if (file_obj == NULL && !(flags & MAP_ANON)){
                return -EBADF;
        }

        if (file_obj == NULL && flags * MAP_ANON){
                goto StartMap;
        }

        /* file mapping was requested, but fd is not open for reading */
        if (!(file_obj->f_mode & FMODE_READ)){
                fput(file_obj);
                return -EACCES;
        }

        /* PROT_WRITE is set, but the file is append-only. */
        if ((prot & PROT_WRITE) && (((file_obj->f_mode - FMODE_APPEND) & 1) == 0)){
                fput(file_obj);
                return -EACCES;
        }

        /* MAP_SHARED was requested and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode */
        if (flags & MAP_SHARED && prot & PROT_WRITE && !((file_obj->f_mode & FMODE_READ) && (file_obj->f_mode & FMODE_WRITE))){
                fput(file_obj);
                return -EACCES;
        }


StartMap: ;

        uint32_t lopage = ADDR_TO_PN(addr);
        uint32_t npages = (uint32_t)len / PAGE_SIZE + (len % PAGE_SIZE > 0 ? 1 : 0);

        vmarea_t* new_vma;
        int err = vmmap_map(curproc->p_vmmap, file_obj->f_vnode, lopage, npages, prot, flags, off, VMMAP_DIR_HILO, &new_vma);

        if (file_obj){
                fput(file_obj);
        }

        if (err < 0){
                return (uintptr_t) MAP_FAILED;
        }

        uintptr_t vaddr = (uintptr_t) PN_TO_ADDR(new_vma->vma_start);
        tlb_flush_range(vaddr, npages);
        pt_unmap_range(curproc->p_pagedir,  vaddr, (vaddr + (uintptr_t)PN_TO_ADDR(npages)));
        if (*ret){
                *ret = (void*)vaddr;
        }

        KASSERT(NULL != curproc->p_pagedir); /* page table must be valid after a memory segment is mapped into the address space */
        dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");

        // NOT_YET_IMPLEMENTED("VM: do_mmap");
        return 0;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{

        /* length was 0 */
        if (len == 0){
                return -EINVAL;
        }
        /* We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary). */
        if (!PAGE_ALIGNED(addr)){
                return -EINVAL;
        }

        uint32_t lopage = ADDR_TO_PN(addr);
        uint32_t npages = (uint32_t)len / PAGE_SIZE + (len % PAGE_SIZE > 0 ? 1 : 0);

        if (vmmap_is_range_empty(curproc->p_vmmap, lopage, npages)){
                return 0;
        }

        int err = vmmap_remove(curproc->p_vmmap, lopage, npages);

        tlb_flush_range((uintptr_t)addr, npages);
        pt_unmap_range(curproc->p_pagedir, (uintptr_t) PN_TO_ADDR(lopage), (uintptr_t)(PN_TO_ADDR(lopage + npages)));
        // NOT_YET_IMPLEMENTED("VM: do_munmap");
        return err;
}

