/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The head of user's memory */
typedef struct _Block {
    /* allocated size */
    size_t size;
    /* memory guard */
    uint32_t magic;
} Block;

/* allocated memory size */
static int usedsize = 0;

/* the heap size */
static const int maxsize = 512 * 1024;

void *mm_alloc(int size)
{
    if (usedsize >= maxsize) {
        printf("error: there is no more memory for heap allocator.\n");
        abort();
    }

    Block *blk = calloc(1, sizeof(*blk) + size);
    assert(blk);
    blk->size = size;
    blk->magic = 0xdeadbeaf;
    usedsize += size;

    return (void *)(blk + 1);
}

void mm_free(void *ptr)
{
    if (!ptr) return;

    Block *blk = (Block *)ptr - 1;

    if (blk->magic != 0xdeadbeaf) {
        printf("bug: memory is broken.\n");
        abort();
    }

    usedsize -= blk->size;
    free(blk);
}

void mm_stat(void)
{
    puts("------ Memory Usage ------");
    printf("%d bytes available\n", maxsize - usedsize);
    printf("%d bytes used\n", usedsize);
    puts("--------------------------");
}

#ifdef __cplusplus
}
#endif
