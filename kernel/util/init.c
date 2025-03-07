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

#include "mm/kmalloc.h"

#include "util/list.h"
#include "util/string.h"
#include "util/debug.h"
#include "util/init.h"

static int _init_search_count = 0;

struct init_function {
        init_func_t  if_func;
        const char  *if_name;
        list_link_t  if_link;

        int          if_search;
        int          if_called;
        list_t       if_deps;
};

struct init_depends {
        const char  *id_name;
        list_link_t  id_link;
};

static void _init_call(list_t *funcs, struct init_function *func)
{
        struct init_depends *dep;
        list_iterate_begin(&func->if_deps, dep, struct init_depends, id_link) {
                int found = 0;
                struct init_function *f;
                list_iterate_begin(funcs, f, struct init_function, if_link) {
                        if (0 == strcmp(dep->id_name, f->if_name)) {
                                found = 1;
                                goto done;
                        }
                } list_iterate_end();
done:

                if (!found) {
                        panic("'%s' dependency for '%s' does not exist",
                              dep->id_name, func->if_name);
                }

                if (func->if_search == f->if_search) {
                        panic("circular dependency between '%s' and '%s'",
                              func->if_name, f->if_name);
                }

                dbg(DBG_INIT, "'%s' depends on '%s': ", func->if_name, f->if_name);
                if (!f->if_called) {
                        dbgq(DBG_INIT, "calling\n");
                        f->if_search = func->if_search;
                        _init_call(funcs, f);
                } else {
                        dbgq(DBG_INIT, "already called\n");
                }
        } list_iterate_end();

        KASSERT(!func->if_called);

        dbg(DBG_INIT, "Calling %s (0x%p)\n", func->if_name, func->if_func);
        func->if_func();
        func->if_called = 1;
}

void init_call_all()
{
        list_t funcs;
        char *buf, *end;

        list_init(&funcs);
        buf = (char *) &kernel_start_init;
        end = (char *) &kernel_end_init;

        while (buf < end) {
                struct init_function *curr = kmalloc(sizeof(*curr));
                KASSERT(NULL != curr);

                list_insert_tail(&funcs, &curr->if_link);
                list_init(&curr->if_deps);

                KASSERT(NULL != *(uintptr_t *)buf);
                curr->if_func = (init_func_t) * (uintptr_t *)buf;
                curr->if_name = buf + sizeof(curr->if_func);
                curr->if_search = 0;
                curr->if_called = 0;

                buf += sizeof(curr->if_func) + strlen(curr->if_name) + 1;

                while ((NULL == *(uintptr_t *)buf) && (buf < end)) {
                        struct init_depends *dep = kmalloc(sizeof(*dep));
                        KASSERT(NULL != dep);

                        list_insert_tail(&curr->if_deps, &dep->id_link);

                        dep->id_name = buf + sizeof(curr->if_func);
                        buf += sizeof(curr->if_func) + strlen(dep->id_name) + 1;
                }
        }

        KASSERT(buf == end);

        struct init_function *func;
        dbg(DBG_INIT, "Initialization functions and dependencies:\n");
        list_iterate_begin(&funcs, func, struct init_function, if_link) {
                dbgq(DBG_INIT, "%s (0x%p): ", func->if_name, func->if_func);
                struct init_depends *dep;
                list_iterate_begin(&func->if_deps, dep, struct init_depends, id_link) {
                        dbgq(DBG_INIT, "%s ", dep->id_name);
                } list_iterate_end();
                dbgq(DBG_INIT, "\n");
        } list_iterate_end();

        list_iterate_begin(&funcs, func, struct init_function, if_link) {
                if (!func->if_called) {
                        func->if_search = ++_init_search_count;
                        _init_call(&funcs, func);
                }
        } list_iterate_end();

        list_iterate_begin(&funcs, func, struct init_function, if_link) {
                struct init_depends *dep;
                list_iterate_begin(&func->if_deps, dep, struct init_depends, id_link) {
                        kfree(dep);
                } list_iterate_end();
                kfree(func);
        } list_iterate_end();
}
