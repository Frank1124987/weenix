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

/* "kernel" utility things */

/* fprintf */
#define fprintf(fd, fmt, args...) dbg(DBG_TEST, fmt, ## args)
#define printf(fmt, args...) dbg(DBG_TEST, fmt, ## args)

/* errno */
#define errno (curthr->kt_errno)

/* malloc/free */
#define malloc kmalloc
#define free kfree

/* The "kernel" system calls */
#define ksyscall(name, formal, actual)          \
        static int ksys_ ## name formal {       \
                int ret = do_ ## name actual ;  \
                if(ret < 0) {                   \
                        errno = -ret;           \
                        return -1;              \
                }                               \
                return ret;                     \
        }

ksyscall(close, (int fd), (fd))
ksyscall(read, (int fd, void *buf, size_t nbytes), (fd, buf, nbytes))
ksyscall(write, (int fd, const void *buf, size_t nbytes), (fd, buf, nbytes))
ksyscall(dup, (int fd), (fd))
ksyscall(dup2, (int ofd, int nfd), (ofd, nfd))
ksyscall(mkdir, (const char *path), (path))
ksyscall(rmdir, (const char *path), (path))
ksyscall(link, (const char *old, const char *new), (old, new))
ksyscall(unlink, (const char *path), (path))
ksyscall(rename, (const char *oldname, const char *newname), (oldname, newname))
ksyscall(chdir, (const char *path), (path))
ksyscall(lseek, (int fd, int offset, int whence), (fd, offset, whence))
ksyscall(getdent, (int fd, struct dirent *dirp), (fd, dirp))
ksyscall(stat, (const char *path, struct stat *uf), (path, uf))
ksyscall(open, (const char *filename, int flags), (filename, flags))
#define ksys_exit do_exit

int ksys_getdents(int fd, struct dirent *dirp, unsigned int count)
{
        size_t numbytesread = 0;
        int nbr = 0;
        dirent_t tempdirent;

        if (count < sizeof(dirent_t)) {
                curthr->kt_errno = EINVAL;
                return -1;
        }

        while (numbytesread < count) {
                if ((nbr = do_getdent(fd, &tempdirent)) < 0) {
                        curthr->kt_errno = -nbr;
                        return -1;
                }
                if (nbr == 0) {
                        return numbytesread;
                }
                memcpy(dirp, &tempdirent, sizeof(dirent_t));

                KASSERT(nbr == sizeof(dirent_t));

                dirp++;
                numbytesread += nbr;
        }
        return numbytesread;
}

/*
 * Redirect system calls to kernel system calls.
 */
#define mkdir(a,b)      ksys_mkdir(a)
#define rmdir           ksys_rmdir
#define mount           ksys_mount
#define umount          ksys_umount
#define open(a,b,c)     ksys_open(a,b)
#define close           ksys_close
#define link            ksys_link
#define rename          ksys_rename
#define unlink          ksys_unlink
#define read            ksys_read
#define write           ksys_write
#define lseek           ksys_lseek
#define dup             ksys_dup
#define dup2            ksys_dup2
#define chdir           ksys_chdir
#define stat(a,b)       ksys_stat(a,b)
#define getdents(a,b,c) ksys_getdents(a,b,c)
#define exit(a)         ksys_exit(a)

/* Random numbers */
/* Random int between lo and hi inclusive */
#define RAND_MAX INT_MAX
#define RANDOM(lo,hi) ((lo)+(((hi)-(lo)+1)*(randseed = (randseed*4096+150889)%714025))/714025)

static unsigned long long randseed = 123456L;

static int rand(void)
{
        randseed = (randseed * 4096 + 150889) % RAND_MAX;
        return randseed;
}

static void srand(unsigned int seed)
{
        randseed = seed;
}
