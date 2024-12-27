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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        vmmap_t * new_vmmap = (vmmap_t *) slab_obj_alloc(vmmap_allocator);

        list_init(&new_vmmap->vmm_list);
        new_vmmap->vmm_proc = NULL;
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_create");
        return new_vmmap;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        KASSERT(NULL != map); /* function argument must not be NULL */
        dbg(DBG_PRINT, "(GRADING3A 3.a)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");

        vmarea_t* vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink){
                list_remove(&vma->vma_plink);

                if (list_link_is_linked(&vma->vma_olink))
                        list_remove(&vma->vma_olink);

                vma->vma_obj->mmo_ops->put(vma->vma_obj);
                vmarea_free(vma);
        } list_iterate_end();
        
        slab_obj_free(vmmap_allocator, map);
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        KASSERT(NULL != map && NULL != newvma); /* both function arguments must not be NULL */
        KASSERT(NULL == newvma->vma_vmmap); /* newvma must be newly create and must not be part of any existing vmmap */
        KASSERT(newvma->vma_start < newvma->vma_end); /* newvma must not be empty */
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
                                  /* addresses in this memory segment must lie completely within the user space */
        dbg(DBG_PRINT, "(GRADING3A 3.b)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");

        vmarea_t* cur_vma;
        list_iterate_begin(&map->vmm_list, cur_vma, vmarea_t, vma_plink ){
                if (cur_vma->vma_start > newvma->vma_start){
                        list_insert_before(&cur_vma->vma_plink, &newvma->vma_plink);
                        newvma->vma_vmmap = map;
                        return;
                }
        } list_iterate_end();

        list_insert_tail(&map->vmm_list, &newvma->vma_plink);
        newvma->vma_vmmap = map;
        // NOT_YET_IMPLEMENTED("VM: vmmap_insert");
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        if (dir == VMMAP_DIR_HILO){
                if (list_empty(&map->vmm_list)){
                        return ADDR_TO_PN(USER_MEM_HIGH) - npages;
                }

                vmarea_t* vma;
                uint32_t prev_start = ADDR_TO_PN(USER_MEM_HIGH);
                list_iterate_reverse(&map->vmm_list, vma, vmarea_t, vma_plink){
                        if (prev_start - vma->vma_end >= npages ){
                                return prev_start - npages;
                        }
                        prev_start = vma->vma_start;
                }list_iterate_end();

                if (prev_start - ADDR_TO_PN(USER_MEM_LOW) >= npages){
                        return prev_start - npages;
                }else{
                        return -1;
                }
        }else{
                if (list_empty(&map->vmm_list)){
                        return USER_MEM_LOW;
                }

                vmarea_t* vma;
                uint32_t prev_end = ADDR_TO_PN(USER_MEM_LOW);
                list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink){
                        if (vma->vma_start - prev_end >= npages){
                                return prev_end;
                        }
                        prev_end = vma->vma_end;
                }list_iterate_end();

                if (ADDR_TO_PN(USER_MEM_HIGH) - prev_end >= npages){
                        return prev_end;
                }else{
                        return -1;
                }
        }
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        KASSERT(NULL != map); /* the first function argument must not be NULL */
        dbg(DBG_PRINT, "(GRADING3A 3.c)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");

        vmarea_t* vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink){
                if (vma->vma_start <= vfn && vma->vma_end > vfn)
                        return vma;
        } list_iterate_end();
        
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        vmmap_t* map_clone = vmmap_create();

        vmarea_t* vma_iter;
        list_iterate_begin(&map->vmm_list, vma_iter, vmarea_t, vma_plink){
                vmarea_t* vma_clone = vmarea_alloc();

                vma_clone->vma_start = vma_iter->vma_start;
                vma_clone->vma_end = vma_iter->vma_end;
                vma_clone->vma_off = vma_iter->vma_off;

                vma_clone->vma_prot = vma_iter->vma_prot;
                vma_clone->vma_flags = vma_iter->vma_flags;

                list_link_init(&(vma_clone->vma_plink));
                list_link_init(&(vma_clone->vma_olink));

                vmmap_insert(map_clone, vma_clone);
                
        }list_iterate_end();
        
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        return map_clone;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        KASSERT(NULL != map); /* must not add a memory segment into a non-existing vmmap */
        KASSERT(0 < npages); /* number of pages of this memory segment cannot be 0 */
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags)); /* must specify whether the memory segment is shared or private */
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage)); /* if lopage is not zero, it must be a user space vpn */
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
                                    /* if lopage is not zero, the specified page range must lie completely within the user space */
        KASSERT(PAGE_ALIGNED(off)); /* the off argument must be page aligned */
        dbg(DBG_PRINT, "(GRADING3A 3.d)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");
            
        int startvfn;
        int err;
        int remove = 0;

        if (lopage == 0){
                startvfn = vmmap_find_range(map, npages, dir);
                if (startvfn < 0){
                        return startvfn;
                }
        }else{
                if (!vmmap_is_range_empty(map, lopage, npages)){
                        // vmmap_remove(map, lopage, npages);
                        /* !QUESTION: why can't we remove right here? */
                        remove = 1;
                }
                startvfn = lopage;
        }
        

        vmarea_t* new_vma = vmarea_alloc();
        mmobj_t* new_mmobj;

        new_vma->vma_start = startvfn;
        new_vma->vma_end = startvfn + npages;
        /* !ISSUE: Not sure*/
        new_vma->vma_off = ADDR_TO_PN(off);

        new_vma->vma_prot = prot;
        new_vma->vma_flags = flags;

        list_link_init(&(new_vma->vma_plink));
        list_link_init(&(new_vma->vma_olink));
        // there is a certain order so we couldn't simply insert it
        // list_insert_tail(&map->vmm_list, &(new_vma->vma_plink));

        if (file == NULL || flags & MAP_ANON){
                // anon object
                new_mmobj = anon_create();
                new_vma->vma_obj = new_mmobj;
                new_mmobj->mmo_refcount = 1;
                // new_mmobj->mmo_ops->ref(new_mmobj);
        }else{
                err = file->vn_ops->mmap(file, new_vma, &new_mmobj);
                new_vma->vma_obj = new_mmobj;

                if (err < 0){
                        // dbg(DBG_ERROR, "something went wrong with mmap");
                        return err;
                }
                /* !QUESTION: can a vnode be represent by more than one mmobj? because in vnode.h it seems like only one but what about real world? */
        }

        if ((prot & (PROT_WRITE & PROT_READ)) && (flags & MAP_PRIVATE)){
                mmobj_t* shadow_mmobj = shadow_create();
                new_vma->vma_obj = shadow_mmobj;
                shadow_mmobj->mmo_refcount++;

                shadow_mmobj->mmo_shadowed = new_mmobj;

                shadow_mmobj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(new_mmobj);
                shadow_mmobj->mmo_un.mmo_bottom_obj->mmo_ops->ref(shadow_mmobj->mmo_un.mmo_bottom_obj);

                list_insert_tail(&shadow_mmobj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &(new_vma->vma_olink));
        }

        if (remove){
                err = vmmap_remove(map, lopage, npages);
                if (err < 0){
                        new_vma->vma_obj->mmo_ops->put(new_vma->vma_obj);
                        vmarea_free(new_vma);
                        return err;
                }
        }

        vmmap_insert(map, new_vma);
        if (new != NULL)
                *new = new_vma;

        // NOT_YET_IMPLEMENTED("VM: vmmap_map");
        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        vmarea_t* vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink){
                uint32_t endpage = lopage + npages;
                if (vma->vma_start < lopage && vma->vma_end > endpage){
                        // case 1
                        // first half
                        vma->vma_end = lopage;

                        // second half
                        vmarea_t* new_vma = vmarea_alloc();

                        new_vma->vma_start = endpage;
                        new_vma->vma_end = vma->vma_end;
                        new_vma->vma_off = endpage - vma->vma_start + vma->vma_off;

                        new_vma->vma_prot = vma->vma_prot;
                        new_vma->vma_flags = vma->vma_flags;

                        list_link_init(&(new_vma->vma_plink));
                        vmmap_insert(map, new_vma);

                        new_vma->vma_obj = vma->vma_obj;
                        vma->vma_obj->mmo_ops->ref(vma->vma_obj);

                        list_link_init(&(new_vma->vma_olink));
                        if (vma->vma_obj->mmo_shadowed){
                                list_insert_tail(&vma->vma_obj->mmo_shadowed->mmo_un.mmo_vmas, &(new_vma->vma_olink));
                        }

                        return 0;
                }else if (vma->vma_start <= lopage && vma->vma_end > lopage && vma->vma_end < endpage){
                        // case 2
                        vma->vma_end = lopage;

                }else if (vma->vma_start > lopage && vma->vma_start <= endpage && vma->vma_end >= endpage){
                        // case 3
                        vma->vma_start = endpage;
                        vma->vma_off = endpage - vma->vma_start + vma->vma_off;
                        return 0;
                }else if (vma->vma_start >= lopage && vma->vma_end <= endpage){
                        // case 4
                        list_remove(&vma->vma_plink);
                        vma->vma_obj->mmo_ops->put(vma->vma_obj);
                        if (list_link_is_linked(&vma->vma_olink))
                                list_remove(&vma->vma_olink);
                        vmarea_free(vma);
                }
        } list_iterate_end(); 
        
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        uint32_t endvfn = startvfn + npages;
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
                                  /* the specified page range must not be empty and lie completely within the user space */
        dbg(DBG_PRINT, "(GRADING3A 3.e)\n");
        dbg(DBG_PRINT, "(GRADING3B 7)\n");

        vmarea_t* vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink){
                if (!(vma->vma_start > (startvfn + npages) || vma->vma_end <= startvfn))
                        return 0;
        } list_iterate_end();

        // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        int err;
        uint32_t vfn = ADDR_TO_PN(vaddr);
        off_t off = PAGE_OFFSET(vaddr);
        char* temp_buf = (char*) buf;

        vmarea_t* vma = vmmap_lookup(map, vfn);
        KASSERT(vma);

        while(count > 0){

                pframe_t* target_pframe;
                uint32_t pagenum = vfn - vma->vma_start + vma->vma_off;
                if ((err = pframe_lookup(vma->vma_obj, pagenum, 1, &target_pframe)) < 0){
                        return err;
                }

                char* target_addr = (char*)target_pframe->pf_addr + off;
                size_t read_len = MIN((PAGE_SIZE - off), count);
                
                memcpy(temp_buf, target_addr, read_len);

                temp_buf += read_len;
                off = 0;
                count -= read_len;
                vfn += 1;
        }  
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_read");
        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        int err;
        uint32_t vfn = ADDR_TO_PN(vaddr);
        off_t off = PAGE_OFFSET(vaddr);
        char* temp_buf = (char*) buf;

        vmarea_t* vma = vmmap_lookup(map, vfn);
        KASSERT(vma);

        while(count > 0){

                pframe_t* target_pframe;
                uint32_t pagenum = vfn - vma->vma_start + vma->vma_off;
                if ((err = pframe_lookup(vma->vma_obj, pagenum, 1, &target_pframe)) < 0){
                        return err;
                }

                char* target_addr = (char*)target_pframe->pf_addr + off;
                size_t write_len = MIN((PAGE_SIZE - off), count);
                
                pframe_dirty(target_pframe);
                memcpy(target_addr, temp_buf, write_len);

                temp_buf += write_len;
                off = 0;
                count -= write_len;
                vfn += 1;
        }
        
        // NOT_YET_IMPLEMENTED("VM: vmmap_write");
        return 0;
}
