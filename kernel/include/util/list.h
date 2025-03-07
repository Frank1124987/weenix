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

#include "kernel.h"

/*
 * Generic circular doubly linked list implementation.
 *
 * list_t is the head of the list.
 * list_link_t should be included in structures which want to be
 * linked on a list_t.
 *
 * All of the list functions take pointers to list_t and list_link_t
 * types, unless otherwise specified.
 *
 * list_init(list) initializes a list_t to an empty list.
 *
 * list_empty(list) returns 1 iff the list is empty.
 *
 * Insertion functions.
 *   list_insert_head(list, link) inserts at the front of the list.
 *   list_insert_tail(list, link) inserts at the end of the list.
 *   list_insert_before(olink, nlink) inserts nlink before olink in list.
 *
 * Removal functions.
 * Head is list->l_next.  Tail is list->l_prev.
 * The following functions should only be called on non-empty lists.
 *   list_remove(link) removes a specific element from the list.
 *   list_remove_head(list) removes the first element.
 *   list_remove_tail(list) removes the last element.
 *
 * Item accessors.
 *   list_item(link, type, member)
 * Given a list_link_t* and the name of the type of structure which contains
 * the list_link_t and the name of the member corresponding to the list_link_t,
 * returns a pointer (of type "type*") to the item.
 *
 * To iterate over a list,
 *    list_link_t *link;
 *    for (link = list->l_next;
 *         link != list; link = link->l_next)
 *       ...
 *
 * Or, use the macros, which will work even if you list_remove() the
 * current link:
 *    type iterator;
 *    list_iterate_begin(list, iterator, type, member) {
 *        ... use iterator ...
 *    } list_iterate_end();
 */

typedef struct list {
        struct list *l_next;
        struct list *l_prev;
} list_t, list_link_t;

#define list_link_init(link)                                            \
        do {                                                            \
                (link)->l_next = (link)->l_prev = NULL;                 \
        } while (0);

#define list_link_is_linked(link)                                       \
        (((link)->l_next != NULL) && ((link)->l_prev != NULL))

#define list_init(list)                                                 \
        do {                                                            \
                (list)->l_next = (list)->l_prev = (list);               \
        } while (0);

#define list_empty(list)                                                \
        ((list)->l_next == (list))

#define list_insert_before(old, new)                                    \
        do {                                                            \
                list_link_t *prev = (new);                              \
                list_link_t *next = (old);                              \
                prev->l_next = next;                                    \
                prev->l_prev = next->l_prev;                            \
                next->l_prev->l_next = prev;                            \
                next->l_prev = prev;                                    \
        } while(0)

#define list_insert_head(list, link)                                    \
        list_insert_before((list)->l_next, link)

#define list_insert_tail(list, link)                                    \
        list_insert_before(list, link)

#define list_remove(link)                                               \
        do {                                                            \
                list_link_t *ll = (link);                               \
                list_link_t *prev = ll->l_prev;                         \
                list_link_t *next = ll->l_next;                         \
                prev->l_next = next;                                    \
                next->l_prev = prev;                                    \
                ll->l_next = ll->l_prev = NULL;                         \
        } while(0)

#define list_remove_head(list)                                          \
        list_remove((list)->l_next)

#define list_remove_tail(list)                                          \
        list_remove((list)->l_prev)

#define list_item(link, type, member)                                   \
        (type*)((char*)(link) - offsetof(type, member))

#define list_head(list, type, member)                                   \
        list_item((list)->l_next, type, member)

#define list_tail(list, type, member)                                   \
        list_item((list)->l_prev, type, member)

#define list_iterate_begin(list, var, type, member)                     \
        do {                                                            \
                list_link_t *__link;                                    \
                list_link_t *__next;                                    \
                for (__link = (list)->l_next;                           \
                     __link != (list);                                  \
                     __link = __next) {                                 \
                        var = list_item(__link, type, member);          \
                        __next = __link->l_next;                        \
                        do

#define list_iterate_reverse(list, var, type, member)                   \
        do {                                                            \
                list_link_t *__link;                                    \
                list_link_t *__prev;                                    \
                for (__link = (list)->l_prev;                           \
                     __link != (list);                                  \
                     __link = __prev) {                                 \
                        var = list_item(__link, type, member);          \
                        __prev = __link->l_prev;                        \
                        do

#define list_iterate_end()                                              \
                        while(0);                                       \
                }                                                       \
        } while(0)
