/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "fib.c"
#include "klvm_passes.h"

/*
gcc -g -fPIC -shared -Wall src/klvm_module.c src/klvm_type.c src/klvm_inst.c \
src/klvm_printer.c src/binheap.c src/hashmap.c src/vector.c \
src/passes/klvm_dot.c src/passes/klvm_block.c -I./include -o libklvm.so

gcc -g test/test_dot.c -I./include -lklvm -L./ -o test_dot
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
*/

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    klvm_module_t *m;

    m = klvm_create_module("test");
    test_fib(m);
    klvm_dump_module(m);

    dot_pass_register(m);
    klvm_run_passes(m);

    klvm_destroy_module(m);

    return 0;
}

#ifdef __cplusplus
}
#endif
