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

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"


#include "fs/file.h"
GDB_DEFINE_HOOK(initialized)

extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);

extern void *vfstest_main(int, void*);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

void      *bootstrap(int arg1, void *arg2);
void      *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void      *initproc_run(int arg1, void *arg2);
void      *final_shutdown(void);

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5f161415322b7887ee5e22f9d3613b73bea7afaf0538c69ea39c4ec7f655ccb2873c5ecc6313532ae6\n");
        /* necessary to finalize page table information */
        pt_template_init();

        // NOT_YET_IMPLEMENTED("PROCS: bootstrap");



        // create the idle process
        proc_t* idle_proc;
        idle_proc = proc_create("idle");
        curproc = idle_proc;


        /* !QUESTION: can I view idle_thr as a TCB? YES!!!! */
        kthread_t* idle_thr;
        idle_thr = kthread_create(idle_proc, idleproc_run, arg1, NULL);
        curthr = idle_thr;

        // KASSERT(NULL != curproc);
        // KASSERT(PID_IDLE == curproc->p_pid);
        // KASSERT(NULL != curthr);
        // dbg(DBG_PRINT, "(GRADING1A 1.a)\n");


        context_make_active(&(idle_thr->kt_ctx));

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");
        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);
        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        do_mkdir("/dev");
        do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID);
        do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);

        int num_terminal = vt_num_terminals();
        char* dev_path = "/dev/tty0";
        do_mknod((const char *) dev_path, S_IFCHR, MKDEVID(2, 0));

        dbg(DBG_PRINT, "(GRADING2B)\n");


#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);
        
        // int fd = do_open("/usr/bin/hello", O_RDWR);
        // if (fd >= 0){
        //         file_t *f_obj = fget(fd);
        //         vput(f_obj->f_vnode);
        //         vput(f_obj->f_vnode);
        //         vput(f_obj->f_vnode);
        // }
        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{

        proc_t* init_proc;
        init_proc = proc_create("init");

        kthread_t* init_thr;
        init_thr = kthread_create(init_proc, initproc_run, 0, NULL);

        KASSERT(NULL != init_proc);
        KASSERT(PID_INIT == init_proc->p_pid);
        KASSERT(NULL != init_thr);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

        // NOT_YET_IMPLEMENTED("PROCS: initproc_create");
        return init_thr;
}

#ifdef __DRIVERS__

void* kernel_hello(int argc, void *argv){
        char *const _argv[] = {"/usr/bin/hello", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/usr/bin/hello", _argv, envp);
        return 0;
}

int run_hello(kshell_t *kshell, int argc, char **argv)
{
        proc_t* hello_proc;
        hello_proc = proc_create("hellotest");
        hello_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* hello_thr;
        hello_thr = kthread_create(hello_proc, kernel_hello, 1, NULL);

        sched_make_runnable(hello_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_args(int argc, void *argv){
        char *const _argv[] = {"/usr/bin/args", "ab", "cde", "fghi", "j", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/usr/bin/args", _argv, envp);
        return 0;
}

int run_args(kshell_t *kshell, int argc, char **argv)
{
        proc_t* args_proc;
        args_proc = proc_create("argstest");
        args_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* args_thr;
        args_thr = kthread_create(args_proc, kernel_args, 1, NULL);

        sched_make_runnable(args_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_uname(int argc, void *argv){
        int fd_0 = do_open("/dev/tty0", O_RDONLY);
        int fd_1 = do_open("/dev/tty0", O_WRONLY);
        char *const _argv[] = {"/bin/uname", "-a", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/bin/uname", _argv, envp);
        do_close(fd_0);
        do_close(fd_1);
        return 0;
}

int run_uname(kshell_t *kshell, int argc, char **argv)
{
        proc_t* uname_proc;
        uname_proc = proc_create("unametest");
        uname_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* uname_thr;
        uname_thr = kthread_create(uname_proc, kernel_uname, 1, NULL);

        sched_make_runnable(uname_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_stat_readme(int argc, void *argv){
        int fd_0 = do_open("/dev/tty0", O_RDONLY);
        int fd_1 = do_open("/dev/tty0", O_WRONLY);
        char *const _argv[] = {"/bin/stat", "/README", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/bin/stat", _argv, envp);
        do_close(fd_0);
        do_close(fd_1);
        return 0;
}

int run_stat_readme(kshell_t *kshell, int argc, char **argv)
{
        proc_t* stat_readme_proc;
        stat_readme_proc = proc_create("stat_readmetest");
        stat_readme_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* stat_readme_thr;
        stat_readme_thr = kthread_create(stat_readme_proc, kernel_stat_readme, 1, NULL);

        sched_make_runnable(stat_readme_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_stat_usr(int argc, void *argv){
        int fd_0 = do_open("/dev/tty0", O_RDONLY);
        int fd_1 = do_open("/dev/tty0", O_WRONLY);
        char *const _argv[] = {"/bin/stat", "/usr", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/bin/stat", _argv, envp);
        do_close(fd_0);
        do_close(fd_1);
        return 0;
}

int run_stat_usr(kshell_t *kshell, int argc, char **argv)
{
        proc_t* stat_usr_proc;
        stat_usr_proc = proc_create("stat_usrtest");
        stat_usr_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* stat_usr_thr;
        stat_usr_thr = kthread_create(stat_usr_proc, kernel_stat_usr, 1, NULL);

        sched_make_runnable(stat_usr_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_fork_and_wait(int argc, void *argv){
        char *const _argv[] = {"/usr/bin/fork-and-wait", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/usr/bin/fork-and-wait", _argv, envp);
        return 0;
}

int run_fork_and_wait(kshell_t *kshell, int argc, char **argv)
{
        proc_t* fork_and_wait_proc;
        fork_and_wait_proc = proc_create("fork_and_waittest");
        fork_and_wait_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* fork_and_wait_thr;
        fork_and_wait_thr = kthread_create(fork_and_wait_proc, kernel_fork_and_wait, 1, NULL);

        sched_make_runnable(fork_and_wait_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_vfstest(int argc, void *argv){
        char *const _argv[] = {"/usr/bin/vfstest", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/usr/bin/vfstest", _argv, envp);
        return 0;
}

int run_vfstest(kshell_t *kshell, int argc, char **argv)
{
        proc_t* vfstest_proc;
        vfstest_proc = proc_create("vfstesttest");
        vfstest_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* vfstest_thr;
        vfstest_thr = kthread_create(vfstest_proc, kernel_vfstest, 1, NULL);

        sched_make_runnable(vfstest_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

void* kernel_memtest(int argc, void *argv){
        char *const _argv[] = {"/usr/bin/memtest", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/usr/bin/memtest", _argv, envp);
        return 0;
}

int run_memtest(kshell_t *kshell, int argc, char **argv)
{
        proc_t* memtest_proc;
        memtest_proc = proc_create("memtesttest");
        memtest_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        kthread_t* memtest_thr;
        memtest_thr = kthread_create(memtest_proc, kernel_memtest, 1, NULL);

        sched_make_runnable(memtest_thr);

        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD){
        }

        // vput(vfs_proc->p_cwd);
        return 0;
}

#endif /* __DRIVERS__ */

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
initproc_run(int arg1, void *arg2)
{

#ifdef __DRIVERS__
        // kshell_add_command("run_hello", run_hello, "Run /usr/bin/hello.");
        // kshell_add_command("run_args", run_args, "Run /usr/bin/args ab cde fghi j.");
        // kshell_add_command("run_uname", run_uname, "Run /bin/uname -a.");
        // kshell_add_command("run_stat_readme", run_stat_readme, "Run /bin/stat /README.");
        // kshell_add_command("run_stat_usr", run_stat_usr, "Run /bin/stat /usr.");
        // kshell_add_command("run_fork_and_wait", run_fork_and_wait, "Run /usr/bin/fork-and-wait.");

        // kshell_add_command("run_vfstest", run_vfstest, "Run /usr/bin/vfstest.");
        // kshell_add_command("run_memtest", run_memtest, "Run /usr/bin/memtest.");


        // kshell_t *kshell = kshell_create(0);
        // if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        // while (kshell_execute_next(kshell));
        // kshell_destroy(kshell);
#endif /* __DRIVERS__ */



        char *const argv[] = {"/sbin/init", NULL};
        char *const envp[] = {NULL};
        kernel_execve("/sbin/init", argv, envp);


        // int status;
        // while(do_waitpid(-1, 0, &status) != -ECHILD){
        // }
        // NOT_YET_IMPLEMENTED("PROCS: initproc_run");

        return NULL;
}
