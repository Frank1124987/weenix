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
#include "types.h"
#include "util/debug.h"
#include "util/list.h"

#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"

#include "mm/pframe.h"
#include "mm/mmobj.h"

static void blockdev_ref(mmobj_t *o);
static void blockdev_put(mmobj_t *o);
static int blockdev_lookuppage(mmobj_t *o, uint32_t pagenum,
                               int forwrite, pframe_t **pf);
static int blockdev_fillpage(mmobj_t *o, pframe_t *pf);
static int blockdev_dirtypage(mmobj_t *o, pframe_t *pf);
static int blockdev_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t blockdev_mmobj_ops = {
        .ref = blockdev_ref,
        .put = blockdev_put,
        .lookuppage = blockdev_lookuppage,
        .fillpage = blockdev_fillpage,
        .dirtypage = blockdev_dirtypage,
        .cleanpage = blockdev_cleanpage
};

static list_t blockdevs;

void
blockdev_init()
{
        list_init(&blockdevs);
        /* Initialize all subsystems */
        ata_init();
}

int
blockdev_register(blockdev_t *dev)
{
        blockdev_t *bd;

        /* Make sure dev, dev ops, and dev id not null */
        if (!dev
            || (NULL_DEVID == dev->bd_id)
            || !(dev->bd_ops))
                return -1;

        /* dev id must be unique */
        list_iterate_begin(&blockdevs, bd, blockdev_t, bd_link) {
                if (dev->bd_id == bd->bd_id)
                        return -1;
        } list_iterate_end();

        /* Initialize its object here */
        mmobj_init(&dev->bd_mmobj, &blockdev_mmobj_ops);

        list_insert_tail(&blockdevs, &dev->bd_link);
        return 0;
}

blockdev_t *
blockdev_lookup(devid_t id)
{
        blockdev_t *bd;
        list_iterate_begin(&blockdevs, bd, blockdev_t, bd_link) {
                if (id == bd->bd_id)
                        return bd;
        } list_iterate_end();
        return NULL;
}

/*
 * Clean and then free all resident pages belonging to this
 * particular block device.
 * As with pframe_clean_all, this is not guaranteed to terminate.
 */
void
blockdev_flush_all(blockdev_t *dev)
{
        pframe_t *pf;

        /* Clean all pages - see pframe_clean_all for
         * explanation of this loop */
clean:
        list_iterate_begin(&dev->bd_mmobj.mmo_respages, pf,
                           pframe_t, pf_olink) {
                if (pframe_is_dirty(pf)) {
                        pframe_clean(pf);
                        goto clean;
                }
        } list_iterate_end();

        /* Free all pages */
        list_iterate_begin(&dev->bd_mmobj.mmo_respages, pf,
                           pframe_t, pf_olink) {
                KASSERT(!pframe_is_dirty(pf));
                pframe_free(pf);
        } list_iterate_end();
}

/* Implementation of mmobj entry points: */

/* Block device mmobjs don't need to ref or put, as they will
 * never be destroyed until the driver is shut down. */
static void
blockdev_ref(mmobj_t *o) {}
static void
blockdev_put(mmobj_t *o) {}

static int
blockdev_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        return pframe_get(o, pagenum, pf);
}

static int
blockdev_fillpage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(pf && pf->pf_obj);
        /* Find the corresponding blockdev */
        blockdev_t *bd = CONTAINER_OF(pf->pf_obj, blockdev_t, bd_mmobj);
        /* And fill in the page by reading from it */
        return bd->bd_ops->read_block(bd, pf->pf_addr, pf->pf_pagenum, 1);
}

/* block devices don't need to make use of this entry point: */
static int
blockdev_dirtypage(mmobj_t *o, pframe_t *pf)
{
        return 0;
}

static int
blockdev_cleanpage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(pf && pf->pf_obj);
        /* Find the corresponding blockdev */
        blockdev_t *bd = CONTAINER_OF(pf->pf_obj, blockdev_t, bd_mmobj);
        /* Clean the corresponding page by writing it back */
        return bd->bd_ops->write_block(bd, pf->pf_addr, pf->pf_pagenum, 1);
}
