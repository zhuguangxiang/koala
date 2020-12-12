/*===-- test_gc.c -------------------------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* Test gc in `gc.h` and `gc.c`                                               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "gc.h"
#include "mm.h"
#include <stdio.h>
#include <string.h>

/* memory at least 340 bytes */
char *ga;
void *gb;

ROOT_TRACE(test_gc, &ga, &gb);

void test_raw_gc(void)
{
    void *obj = NULL;
    void *obj2 = NULL;
    gc_push(&obj, &obj2);
    obj = gc_alloc(110);
    obj2 = gc_alloc(50);
    gc_pop();
}

void test_array_gc(void)
{
    GcArray *arr = NULL;
    gc_push(&arr);
    arr = gc_alloc_array(2, 1, 1);
    void **pobj = (void **)arr->ptr;
    pobj[0] = gc_alloc(8);
    pobj[0] = gc_alloc(6);
    arr = gc_expand_array(arr, 4);
    gc_pop();

    {
        arr = NULL;
        gc_push(&arr);
        arr = gc_alloc_array(3, 0, 1);
        pobj = (void **)arr->ptr;
        pobj[0] = gc_alloc(8);
        gc_pop();
    }
}

struct Bar {
    int value;
};

struct Foo {
    int value;
    char *str;
    struct Bar *bar;
};

OBJECT_MAP(foo, offsetof(struct Foo, str), offsetof(struct Foo, bar));

struct Foo *foo;
ROOT_TRACE(test_foo, &foo);

void test_struct_gc(void)
{
    struct Bar *bar = NULL;
    gc_push(&bar);

    foo = gc_alloc_struct(__foo__, sizeof(struct Foo));
    bar = gc_alloc_struct(NULL, sizeof(struct Bar));
    bar->value = 0xbeaf;
    bar = gc_alloc_struct(NULL, sizeof(struct Bar));
    bar->value = 100;
    foo->bar = bar;
    char *str = gc_alloc(20);
    str = gc_alloc(40);
    strcpy(str, "hello in foo");
    foo->str = str;
    foo->value = 200;
    gc_pop();
}

int main(int argc, char *argv[])
{
    gc_init();

    gc_add_root(test_gc);
    gc_add_root(test_foo);

    ga = gc_alloc(80);
    ga = gc_alloc(100);
    ga = gc_alloc(60);
    strcpy(ga, "hello, world");

    test_raw_gc();

    gb = gc_alloc(120);
    gb = gc_alloc(100);

    printf("%s\n", ga);

    test_array_gc();

    gc();

    gb = NULL;
    test_struct_gc();

    gc();

    printf("%s\n", ga);
    printf("%s, %d, %d\n", foo->str, foo->bar->value, foo->value);

    gc_fini();

    return 0;
}
