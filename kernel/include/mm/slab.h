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

/* Define SLAB_REDZONE to add top and bottom redzones to every object.
 * Use kmem_check_redzones() liberally throughout your code to test
 * for memory pissing. */
#define SLAB_REDZONE            0xdeadbeef

/* Define SLAB_CHECK_FREE to add extra book keeping to make sure there
 * are no double frees. */
#define SLAB_CHECK_FREE

/*
 * The slab allocator. A "cache" is a store of objects; you create one by
 * specifying a constructor, destructor, and the size of an object. The
 * "alloc" function allocates one object, and the "free" function returns
 * it to the free list *without calling the destructor*. This lets you save
 * on destruction/construction calls; the idea is that every free object in
 * the cache is in a known state.
 */
typedef struct slab_allocator slab_allocator_t;

slab_allocator_t *slab_allocator_create(const char *name, size_t size);
int slab_allocators_reclaim(int target);

void *slab_obj_alloc(slab_allocator_t *allocator);
void slab_obj_free(slab_allocator_t *allocator, void *obj);
