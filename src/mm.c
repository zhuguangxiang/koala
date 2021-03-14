/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
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
} Block, *BlockRef;

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

    BlockRef blk = calloc(1, sizeof(*blk) + size);
    assert(blk);
    blk->size = size;
    blk->magic = 0xdeadbeaf;
    usedsize += size;

    return (void *)(blk + 1);
}

void mm_free(void *ptr)
{
    if (!ptr) return;

    BlockRef blk = (BlockRef)ptr - 1;

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
