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

#include "drivers/tty/n_tty.h"

#include "errno.h"

#include "drivers/tty/driver.h"
#include "drivers/tty/ldisc.h"
#include "drivers/tty/tty.h"

#include "mm/kmalloc.h"

#include "proc/kthread.h"

#include "util/debug.h"

/* helpful macros */
#define EOFC            '\x4'
#define TTY_BUF_SIZE    128
#define ldisc_to_ntty(ldisc) \
        CONTAINER_OF(ldisc, n_tty_t, ntty_ldisc)

static void n_tty_attach(tty_ldisc_t *ldisc, tty_device_t *tty);
static void n_tty_detach(tty_ldisc_t *ldisc, tty_device_t *tty);
static int n_tty_read(tty_ldisc_t *ldisc, void *buf, int len);
static const char *n_tty_receive_char(tty_ldisc_t *ldisc, char c);
static const char *n_tty_process_char(tty_ldisc_t *ldisc, char c);

static tty_ldisc_ops_t n_tty_ops = {
        .attach       = n_tty_attach,
        .detach       = n_tty_detach,
        .read         = n_tty_read,
        .receive_char = n_tty_receive_char,
        .process_char = n_tty_process_char
};

struct n_tty {
        kmutex_t            ntty_rlock;
        ktqueue_t           ntty_rwaitq;
        char               *ntty_inbuf;
        int                 ntty_rhead;
        int                 ntty_rawtail;
        int                 ntty_ckdtail;

        tty_ldisc_t         ntty_ldisc;
};


tty_ldisc_t *
n_tty_create()
{
        n_tty_t *ntty = (n_tty_t *)kmalloc(sizeof(n_tty_t));
        if (NULL == ntty) return NULL;
        ntty->ntty_ldisc.ld_ops = &n_tty_ops;
        return &ntty->ntty_ldisc;
}

void
n_tty_destroy(tty_ldisc_t *ldisc)
{
        KASSERT(NULL != ldisc);
        kfree(ldisc_to_ntty(ldisc));
}

/*
 * Initialize the fields of the n_tty_t struct, allocate any memory
 * you will need later, and set the tty_ldisc field of the tty.
 */
void
n_tty_attach(tty_ldisc_t *ldisc, tty_device_t *tty)
{
        NOT_YET_IMPLEMENTED("DRIVERS: n_tty_attach");
}

/*
 * Free any memory allocated in n_tty_attach and set the tty_ldisc
 * field of the tty.
 */
void
n_tty_detach(tty_ldisc_t *ldisc, tty_device_t *tty)
{
        NOT_YET_IMPLEMENTED("DRIVERS: n_tty_detach");
}

/*
 * Read a maximum of len bytes from the line discipline into buf. If
 * the buffer is empty, sleep until some characters appear. This might
 * be a long wait, so it's best to let the thread be cancellable.
 * Return -EINTR when the thread gets cancelled.
 *
 * Then, read from the head of the buffer up to the tail, stopping at
 * len bytes or a newline character, and leaving the buffer partially
 * full if necessary. Return the number of bytes you read into the
 * buf.

 * In this function, you will be accessing the input buffer, which
 * could be modified by other threads. Make sure to make the
 * appropriate calls to ensure that no one else will modify the input
 * buffer when we are not expecting it.
 *
 * Remember to handle newline characters and CTRL-D, or ASCII 0x04,
 * properly.
 */
int
n_tty_read(tty_ldisc_t *ldisc, void *buf, int len)
{
        NOT_YET_IMPLEMENTED("DRIVERS: n_tty_read");
        return 0;
}

/*
 * The tty subsystem calls this when the tty driver has received a
 * character. Now, the line discipline needs to store it in its read
 * buffer and move the read tail forward.
 *
 * Special cases to watch out for: backspaces (both ASCII characters
 * 0x08 and 0x7F should be treated as backspaces), newlines ('\r' or
 * '\n'), and full buffers. 
 * Hints: For newlines (either '\r' or '\n'), you
 * need to return '\r\n' to make it display successfully (carriage 
 * return, followed by a line feed).
 *
 * Return a null terminated string containing the characters which
 * need to be echoed to the screen. For a normal, printable character,
 * just the character to be echoed.
 */
const char *
n_tty_receive_char(tty_ldisc_t *ldisc, char c)
{
        NOT_YET_IMPLEMENTED("DRIVERS: n_tty_receive_char");
        return NULL;
}

/*
 * Process a character to be written to the screen.
 *
 * The only special case is '\r' and '\n'. Weenix needs
 * '\r\n' to make a new line on screen.
 */
const char *
n_tty_process_char(tty_ldisc_t *ldisc, char c)
{
        NOT_YET_IMPLEMENTED("DRIVERS: n_tty_process_char");

        return NULL;
}
