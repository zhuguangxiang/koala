/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
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

/*
 * An Atom is a null-terminated string cached in internal hashmap.
 * With null-terminated, it's convenient to operate it like c-string.
 */

#ifndef _KOALA_ATOM_H_
#define _KOALA_ATOM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* New an Atom string with null-terminated string. */
char *AtomStr(char *str);

/* New an Atom string with length-ed string. */
char *AtomNStr(char *str, int len);

/* Concat 'arc' null-terminated strings into one Atom string. */
char *AtomVStr(int argc, ...);

/* Initialize Atom string internal hashmap */
void InitAtom(void);

/* Free hashmap and Atom string memory */
void FiniAtom(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_ATOM_H_ */
