/*===-- init.c - Koala Virtual Machine Initializer ----------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file is the initializer of koala virtual machine                      *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "module.h"
#include "task.h"

void kl_init(void)
{
    /* init strtab */

    /* init gc */
    gc_init();

    /* init core modules */
    init_core_modules();

    /* init coroutine */
    init_procs(1);
}

void kl_fini(void)
{
    /* fini coroutine */
    fini_procs();

    /* fini gc */
    gc_fini();

    /* fini modules */
    fini_modules();
}
