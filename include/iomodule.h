/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_IO_MODULE_H_
#define _KOALA_IO_MODULE_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject io_writer_type;
extern TypeObject io_reader_type;
void init_io_types(void);
void fini_io_types(void);
void init_io_module(void);
void fini_io_module(void);

extern TypeObject bufio_r_type;
extern TypeObject bufio_w_type;
extern TypeObject line_w_type;
#define bufio_r_check(ob)  (OB_TYPE(ob) == &bufio_r_type)
#define bufio_w_check(ob)  (OB_TYPE(ob) == &bufio_w_type)
#define line_w_check(ob) (OB_TYPE(ob) == &line_w_type)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FMT_MODULE_H_ */
