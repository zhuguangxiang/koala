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

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct codeobject {
  OBJECT_HEAD
  char *name;
  Object *owner;
  TypeDesc *proto;
  int anony;
  int locals;
  /* local variables */
  Vector locvec;
  /* index of free variables */
  Vector freevec;
  /* index of upvals */
  Vector upvec;
  Object *consts;
  int size;
  uint8_t codes[0];
} CodeObject;

extern TypeObject code_type;
#define code_check(ob) (OB_TYPE(ob) == &code_type)
Object *code_new(char *name, TypeDesc *proto, int locals,
  uint8_t *codes, int size);
void code_add_local(Object *code, Object *ob);
static inline void code_set_consts(Object *code, Object *consts)
{
  CodeObject *co = (CodeObject *)code;
  co->consts = OB_INCREF(consts);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
