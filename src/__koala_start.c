/*===-- __koala_start.c - Koala Application Start -----------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file is the start of koala application                                *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* koala main entry */
void __kl_main__(void);

/* init koala */
void koala_init(void);

/* fini koala */
void koala_fini(void);

/* __attribute__((section("_init_mod_"))) */
typedef void (*init_mod)(void);

/* defined in cc linker script */
extern init_mod __start__init_mod_[];
extern init_mod __stop__init_mod_[];

int main(int argc, char *argv[])
{
    /* init koala */
    koala_init();

    /* init modules */
    init_mod *func = __start__init_mod_;
    while (func < __stop__init_mod_) {
        (*func)();
        func++;
    }

    /* call koala main */
    __kl_main__();

    /* fini koala */
    koala_fini();
    return 0;
}
