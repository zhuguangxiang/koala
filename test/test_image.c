/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "image.h"

void test_klc(void)
{
    klc_image_t *klc = klc_create();
    klc_add_var(klc, "foo", &kl_type_int, ACCESS_FLAGS_PUB);
    klc_add_var(klc, "bar", &kl_type_bool, ACCESS_FLAGS_FINAL);
    TypeDesc *proto = to_proto("ii", "z");
    klc_add_var(klc, "fn1", proto, 0);
    proto = to_proto("Pi:i", "Pz:");
    klc_add_var(klc, "fn2", proto, ACCESS_FLAGS_PUB);
    klc_show(klc);
    klc_write_file(klc, "foo.klc");
    klc_destroy(klc);

    klc = klc_read_file("foo.klc");
    klc_show(klc);
    klc_destroy(klc);
}

int main(int argc, char *argv[])
{
    test_klc();
    return 0;
}
