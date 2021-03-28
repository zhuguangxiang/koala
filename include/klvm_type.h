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

#ifndef _KLVM_TYPE_H_
#define _KLVM_TYPE_H_

#if !defined(_KLVM_H_INSIDE_) && !define(_COMPILE_KLVM_)
#error "Only <klvm.h> can be included directly."
#endif

#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLVMTypeKind {
    KLVM_TYPE_INVALID,
    KLVM_TYPE_INT8,
    KLVM_TYPE_INT16,
    KLVM_TYPE_INT32,
    KLVM_TYPE_INT64,
    KLVM_TYPE_FLOAT32,
    KLVM_TYPE_FLOAT64,
    KLVM_TYPE_BOOL,
    KLVM_TYPE_CHAR,
    KLVM_TYPE_STR,
    KLVM_TYPE_ANY,
    KLVM_TYPE_ARRAY,
    KLVM_TYPE_MAP,
    KLVM_TYPE_TUPLE,
    KLVM_TYPE_VALIST,
    KLVM_TYPE_KLASS,
    KLVM_TYPE_PROTO,
} KLVMTypeKind;

/*
 * int: i, i8, i16, i32, i64
 * float: f32, f64
 * bool: b
 * char: c
 * str: s
 * any: A
 * array: [s
 * Map: M(s:s)
 * Tuple: T(is[s)
 * varg: ...s
 * klass: Lio.File;
 * proto: P(is:i)
 */
typedef struct _KLVMType {
    KLVMTypeKind kind;
} KLVMType, *KLVMTypeRef;

void fini_klvm_type(void);

KLVMTypeRef klvm_type_int8(void);
KLVMTypeRef klvm_type_int16(void);
KLVMTypeRef klvm_type_int32(void);
KLVMTypeRef klvm_type_int64(void);
KLVMTypeRef klvm_type_int(void);
KLVMTypeRef klvm_type_float32(void);
KLVMTypeRef klvm_type_float64(void);
KLVMTypeRef klvm_type_bool(void);
KLVMTypeRef klvm_type_char(void);
KLVMTypeRef klvm_type_str(void);
KLVMTypeRef klvm_type_any(void);

KLVMTypeRef klvm_type_klass(char *path, char *name);
char *klass_get_path(KLVMTypeRef type);
char *klass_get_name(KLVMTypeRef type);
VectorRef klass_get_typeparams(KLVMTypeRef type);
void klass_add_typeparam(KLVMTypeRef type, KLVMTypeRef param);

KLVMTypeRef klvm_type_proto(KLVMTypeRef ret, VectorRef params);
VectorRef klvm_proto_params(KLVMTypeRef ty);
KLVMTypeRef klvm_proto_ret(KLVMTypeRef ty);

/* new type from string */
KLVMTypeRef str_to_type(char *type);

/* new proto from string */
KLVMTypeRef str_to_proto(char *para, char *ret);

int klvm_type_equal(KLVMTypeRef ty1, KLVMTypeRef ty2);
char *klvm_type_tostr(KLVMTypeRef ty);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_TYPE_H_ */
