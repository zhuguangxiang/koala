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

// clang-format off
#include <sys/utsname.h>
#include "version.h"
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "readline.h"
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

#define PROMPT      ">>> "
#define MORE_PROMPT "    "

static yyscan_t scanner;
static ParserState cmdps;

static void show_banner(void)
{
    printf("Koala %s (%s, %s)\n", KOALA_VERSION, __DATE__, __TIME__);

    struct utsname sysinfo;
    if (!uname(&sysinfo)) {
#if defined(__clang__)
        printf("[clang %d.%d.%d on %s/%s]\r\n", __clang_major__,
               __clang_minor__, __clang_patchlevel__, sysinfo.sysname,
               sysinfo.machine);
#elif defined(__GNUC__)
        printf("[gcc %d.%d.%d on %s/%s]\r\n", __GNUC__, __GNUC_MINOR__,
               __GNUC_PATCHLEVEL__, sysinfo.sysname, sysinfo.machine);
#elif defined(_MSC_VER)
#else
#endif
    }
}

static int empty(char *buf, int len)
{
    char ch;
    for (int i = 0; i < len; i++) {
        ch = buf[i];
        if (ch != '\r' && ch != '\n' && ch != ' ') return 0;
    }
    return 1;
}

int stdin_input(ParserStateRef ps, char *buf, int size)
{
    int len;
    if (ps->more) {
        len = readline(MORE_PROMPT, buf, size);
    } else {
        len = readline(PROMPT, buf, size);
    }

    if (!empty(buf, len)) {
        sbuf_nprint(&ps->linebuf, buf, len - 2);
        ps->more = 1;
    }

    return len;
}

void kl_cmdline(void)
{
    show_banner();

    init_line();
    init_atom();

    cmdps.interactive = 1;
    cmdps.line = 1;
    cmdps.col = 1;
    cmdps.filename = "stdin";
    cmdps.in = stdin;

    RESET_SBUF(cmdps.linebuf);
    RESET_SBUF(cmdps.sbuf);
    vector_init(&cmdps.svec, sizeof(SBuf));

    yylex_init_extra(&cmdps, &scanner);
    yyset_in(stdin, scanner);
    cmdps.lexer = scanner;
    yyparse(&cmdps, scanner);
    yylex_destroy(scanner);

    FINI_SBUF(cmdps.linebuf);
    FINI_SBUF(cmdps.sbuf);
    fini_atom();
    fini_line();
}

#ifdef __cplusplus
}
#endif
