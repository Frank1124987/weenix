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

/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{

        if (oflags != O_RDWR &&
                oflags != O_WRONLY &&
                oflags != O_RDONLY &&
                oflags != (O_RDWR | O_CREAT) &&
                oflags != (O_WRONLY | O_CREAT) &&
                oflags != (O_RDONLY | O_CREAT) &&
                oflags != (O_RDWR | O_APPEND) &&
                oflags != (O_WRONLY | O_APPEND) &&
                oflags != (O_RDONLY | O_APPEND)) {

                dbg(DBG_PRINT, "(GRADING2B)\n");

                return -EINVAL; 
        }

        int lower_mask = 0xFF;
        int oflag_lower = lower_mask & oflags;

        int higher_mask = ~lower_mask;
        int oflag_higher = higher_mask & oflags;


        int fd = get_empty_fd(curproc);
        file_t *f_obj = fget(-1);
        curproc->p_files[fd] = f_obj;

        if (oflag_lower == O_RDONLY){
                f_obj->f_mode = FMODE_READ;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }else if (oflag_lower == O_WRONLY){
                f_obj->f_mode = FMODE_WRITE;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }else if (oflag_lower == O_RDWR){
                f_obj->f_mode = FMODE_READ | FMODE_WRITE;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        if (oflag_higher == O_APPEND){
                f_obj->f_mode |= FMODE_APPEND;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        vnode_t* file_vnode;
        int err = open_namev(filename, oflag_higher, &file_vnode, NULL);
        if ( err < 0){
                curproc->p_files[fd] = NULL;
                fput(f_obj);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return err;
        }

        if ((S_ISDIR(file_vnode->vn_mode)) && ((oflag_lower == O_WRONLY) || (oflag_lower == O_RDWR))){
                curproc->p_files[fd] = NULL;
                fput(f_obj);
                vput(file_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EISDIR;
        }

        f_obj->f_vnode = file_vnode;
        f_obj->f_pos = 0;
       
        dbg(DBG_PRINT, "(GRADING2B)\n");

        dbg(DBG_VNREF, "opening file: %s, vnode: %d, refcount: %d\n", filename, file_vnode->vn_vno, file_vnode->vn_mmobj.mmo_refcount);

        // NOT_YET_IMPLEMENTED("VFS: do_open");
        return fd;
}
