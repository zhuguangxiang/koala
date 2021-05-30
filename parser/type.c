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

#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeRef ast_type_int8(void)
{
    TypeRef ty = mm_alloc(sizeof(*ty));
    ty->kind = KLIR_TYPE_INT8;
    return ty;
}

TypeRef ast_type_klass(Ident *pkg, Ident *name)
{
    TypeRef ty = mm_alloc(sizeof(*ty));
    ty->kind = KLIR_TYPE_KLASS;
    return ty;
}

TypeRef ast_type_proto(TypeRef ret, VectorRef params)
{
    TypeRef ty = mm_alloc(sizeof(*ty));
    ty->kind = KLIR_TYPE_PROTO;
    return ty;
}

TypeRef ast_type_valist(TypeRef subtype)
{
    TypeRef ty = mm_alloc(sizeof(*ty));
    ty->kind = KLIR_TYPE_VALIST;
    return ty;
}

#ifdef __cplusplus
}
#endif
