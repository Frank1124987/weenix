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
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        return size;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1) {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                        if (p->p_pid == pid) {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid) {
                                        return -1;
                                } else {
                                        goto failed;
                                }
                        }
                } list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
        proc_t* proc_temp = (proc_t*) slab_obj_alloc(proc_allocator);

        if (strncmp(name, "init", strlen("init")) == 0){
                proc_initproc = proc_temp;
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }

        proc_temp->p_pid = _proc_getid();

        KASSERT(PID_IDLE != proc_temp->p_pid || list_empty(&_proc_list)); /* pid can only be PID_IDLE if this is the first process */
        KASSERT(PID_INIT != proc_temp->p_pid || PID_IDLE == curproc->p_pid); /* pid can only be PID_INIT if the running process is the "idle" process */
        dbg(DBG_PRINT, "(GRADING1A 2.a)\n");


        strcpy(proc_temp->p_comm, name);

        list_init(&proc_temp->p_threads); 
        list_init(&proc_temp->p_children); 

        proc_temp->p_status = 0;
        proc_temp->p_state = PROC_RUNNING;

        list_link_init(&proc_temp->p_list_link);
        list_insert_tail(&_proc_list, &proc_temp->p_list_link);

        list_link_init(&proc_temp->p_child_link);
        if (strncmp(name, "idle", strlen("idle")) == 0){
                proc_temp->p_pproc = NULL;
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }else{
                proc_temp->p_pproc = curproc;
                list_insert_tail(&curproc->p_children, &proc_temp->p_child_link);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }

        sched_queue_init(&proc_temp->p_wait);
        proc_temp->p_pagedir = pt_create_pagedir();

        int i = 0;
        for (i = 0 ; i < NFILES ; i++) {
                proc_temp->p_files[i] = NULL;
        }
        proc_temp->p_cwd = NULL;          

        proc_temp->p_brk = NULL;          
        proc_temp->p_start_brk = NULL;    

        proc_temp->p_vmmap = vmmap_create();


        dbg(DBG_PRINT, "(GRADING1A)\n");

        // NOT_YET_IMPLEMENTED("PROCS: proc_create");
        return proc_temp;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{
        KASSERT(NULL != proc_initproc); /* "init" process must exist and proc_initproc initialized */
        KASSERT(1 <= curproc->p_pid); /* this process must not be "idle" process */
        KASSERT(NULL != curproc->p_pproc); /* this process must have a parent when this function is entered */
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");


        curproc->p_state = PROC_DEAD;
        curproc->p_status = status;

        proc_t* proc_child;
        list_iterate_begin(&curproc->p_children, proc_child, proc_t, p_child_link){
                proc_child->p_pproc = proc_initproc;
                list_remove(&proc_child->p_child_link);
                list_insert_tail(&proc_initproc->p_children, &proc_child->p_child_link);
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }list_iterate_end();

        for (int i=0 ; i<NFILES ; i++){
                if (curproc->p_files[i]){
                        dbg(DBG_ERROR, "file count: %d\n", i);
                        fput(curproc->p_files[i]);
                }
        }

        if (curproc->p_cwd){
                vput(curproc->p_cwd);
        }

        vmmap_destroy(curproc->p_vmmap);

        sched_wakeup_on(&curproc->p_pproc->p_wait);

        curthr->kt_state = KT_EXITED;

        KASSERT(NULL != curproc->p_pproc); /* this process must still have a parent when this function returns */
        KASSERT(KT_EXITED == curthr->kt_state);
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");

        // NOT_YET_IMPLEMENTED("PROCS: proc_cleanup");
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
        if (p == curproc){
                dbg(DBG_PRINT, "(GRADING1C)\n");
                do_exit(status);
        }

        kthread_t * thr_iter;
        list_iterate_begin(&p->p_threads, thr_iter, kthread_t, kt_plink ){
                kthread_cancel(thr_iter, (void*) status);
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }list_iterate_end();

        p->p_status = status;

        dbg(DBG_PRINT, "(GRADING1C)\n");

        // NOT_YET_IMPLEMENTED("PROCS: proc_kill");
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
        proc_t * proc_iter;
        list_iterate_begin(&_proc_list, proc_iter, proc_t, p_list_link ){
                if (proc_iter->p_pid != PID_IDLE && proc_iter->p_pid != PID_INIT && proc_iter != curproc && proc_iter->p_pid != 2){
                        proc_kill(proc_iter, proc_iter->p_status);
                        dbg(DBG_PRINT, "(GRADING1C)\n");
                }
        }list_iterate_end();

        dbg(DBG_PRINT, "(GRADING1C)\n");

        proc_kill(curproc, curproc->p_status);

        // NOT_YET_IMPLEMENTED("PROCS: proc_kill_all");
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
        // NOT_YET_IMPLEMENTED("PROCS: proc_thread_exited");

        proc_cleanup((int)retval);

        dbg(DBG_PRINT, "(GRADING1C)\n");

        sched_switch();

}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{
        if (list_empty(&curproc->p_children)){
                dbg(DBG_PRINT, "(GRADING1A)\n");
                return -ECHILD;
        }

        proc_t* proc_child;
        pid_t proc_child_id;

        if (pid == -1){
                proc_t* proc_iter;
                int all_children_running = 1;
                do{
                        list_iterate_begin(&curproc->p_children, proc_iter, proc_t, p_child_link){
                                if (proc_iter->p_state == PROC_DEAD){
                                        all_children_running = 0;
                                        
                                        if (status != NULL)
                                                *status = proc_iter->p_status;
                                        
                                        proc_child_id = proc_iter->p_pid;
                                        proc_child = proc_iter;
                                        dbg(DBG_PRINT, "(GRADING1C)\n");
                                        break;
                                }
                        }list_iterate_end();

                        if (all_children_running){
                                /* wait for one child die right here */ 
                                sched_sleep_on(&curproc->p_wait);
                                dbg(DBG_PRINT, "(GRADING1C)\n");
                        }else{
                                dbg(DBG_PRINT, "(GRADING1C)\n");
                                break;
                        }
                }while(1);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }else{
                /* search if pid is the children of current process */
                proc_t* proc_iter;
                int child_exist = 0;
                do{        
                        int child_dead = 0;   
                        list_iterate_begin(&curproc->p_children, proc_iter, proc_t, p_child_link) {
                                if (proc_iter->p_pid == pid ) {
                                        child_exist = 1;
                                        if (proc_iter->p_state == PROC_DEAD){
                                                child_dead = 1;
                                                dbg(DBG_PRINT, "(GRADING1D 1)\n");
                                        }

                                        if (status != NULL)
                                                *status = proc_iter->p_status;
                                                
                                        proc_child_id = proc_iter->p_pid;
                                        proc_child = proc_iter;

                                        dbg(DBG_PRINT, "(GRADING1D 1)\n");
                                        break;
                                }
                        } list_iterate_end();  

                        if (!child_exist){
                                dbg(DBG_PRINT, "(GRADING1D 1)\n");
                                return -ECHILD;
                        }else if(child_exist && !child_dead){
                                sched_sleep_on(&curproc->p_wait);
                                dbg(DBG_PRINT, "(GRADING1D 1)\n");
                        }else{
                                dbg(DBG_PRINT, "(GRADING1D 1)\n");
                                break;
                        }
                }while(1);
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }


        /* clean up thread */ 
        kthread_t* thr_itr;
        list_iterate_begin(&proc_child->p_threads, thr_itr, kthread_t, kt_plink){
                KASSERT(thr_itr->kt_state == KT_EXITED);
                kthread_destroy(thr_itr);
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }list_iterate_end();

        list_remove(&proc_child->p_list_link);
        list_remove(&proc_child->p_child_link);


        KASSERT(NULL != proc_child); /* must have found a dead child process */
        KASSERT(-1 == pid || proc_child->p_pid == pid); /* if the pid argument is not -1, then pid must be the process ID of the found dead child process */
        KASSERT(NULL != proc_child->p_pagedir); /* this process should have a valid pagedir before you destroy it */
        dbg(DBG_PRINT, "(GRADING1A 2.c)\n");

        pt_destroy_pagedir(proc_child->p_pagedir);
        slab_obj_free(proc_allocator, (void*) proc_child);

        dbg(DBG_PRINT, "(GRADING1A)\n");

        // NOT_YET_IMPLEMENTED("PROCS: do_waitpid");
        return proc_child_id;
}

/*
 * Cancel all threads and join with them (if supporting MTP), and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{       
        // kthread_t* thr_itr;
        // list_iterate_begin(&curproc->p_threads, thr_itr, kthread_t, kt_plink ){
        //         kthread_cancel(thr_itr, thr_itr->kt_retval);
        // }list_iterate_end();

        dbg(DBG_PRINT, "(GRADING1C)\n");

        kthread_exit((void*) status);

        // NOT_YET_IMPLEMENTED("PROCS: do_exit");
}
