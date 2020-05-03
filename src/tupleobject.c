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

#include <stdarg.h>
#include "tupleobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "log.h"

Object *tuple_new(int size)
{
  expect(size > 0);
  int msize = sizeof(TupleObject) + size * sizeof(Object *);
  TupleObject *tuple = gcnew(msize);
  init_object_head(tuple, &tuple_type);
  tuple->size = size;
  return (Object *)tuple;
}

static void tuple_clean(Object *ob)
{
  if (!tuple_check(ob)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(ob));
    return;
  }
  int size = tuple_size(ob);
  TupleObject *tuple = (TupleObject *)ob;
  Object *item;
  for (int i = 0; i < size; ++i) {
    item = tuple->items[i];
    OB_DECREF(item);
  }
}

static void tuple_free(Object *ob)
{
  tuple_clean(ob);
  gcfree(ob);
}

Object *tuple_pack(int size, ...)
{
  Object *ob = tuple_new(size);
  va_list ap;

  va_start(ap, size);
  Object *item;
  TupleObject *tuple = (TupleObject *)ob;
  for (int i = 0; i < size; ++i) {
    item = va_arg(ap, Object *);
    tuple->items[i] = OB_INCREF(item);
  }
  va_end(ap);

  return ob;
}

int tuple_size(Object *self)
{
  if (self == NULL) {
    warn("tuple pointer is null.");
    return -1;
  }

  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return -1;
  }

  return ((TupleObject *)self)->size;
}

Object *tuple_get(Object *self, int index)
{
  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  TupleObject *tuple = (TupleObject *)self;
  if (index < 0 || index > tuple->size) {
    error("tuple index out of range.");
    return NULL;
  }

  Object *ob = tuple->items[index];
  return OB_INCREF(ob);
}

int tuple_set(Object *self, int index, Object *val)
{
  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return -1;
  }

  TupleObject *tuple = (TupleObject *)self;
  if (index < 0 || index > tuple->size) {
    error("tuple index out of range.");
    return -1;
  }

  tuple->items[index] = OB_INCREF(val);
  return 0;
}

/* [i ..< j] */
Object *tuple_slice(Object *self, int i, int j)
{
  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  int size = tuple_size(self);
  if (i < 0)
    i = 0;
  if (j < 0 || j > size)
    j = size;
  if (j < i)
    j = i;

  if (i == 0 && j == size) {
    return OB_INCREF(self);
  }

  int len = j - i;
  TupleObject *tuple = (TupleObject *)tuple_new(len);
  Object **src = ((TupleObject *)self)->items + i;
  Object **dest = tuple->items;
  for (int k = 0; k < len; ++k) {
    dest[k] = OB_INCREF(src[k]);
  }
  return (Object *)tuple;
}

static Object *tuple_getitem(Object *self, Object *args)
{
  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  TupleObject *tuple = (TupleObject *)self;
  int index = integer_asint(args);
  if (index < 0 || index >= tuple->size) {
    error("index %d out of range(0..<%d)", index, tuple->size);
    return NULL;
  }
  Object *val = tuple->items[index];
  return OB_INCREF(val);
}

static Object *tuple_length(Object *self, Object *args)
{
  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  TupleObject *tuple = (TupleObject *)self;
  return integer_new(tuple->size);
}

Object *tuple_match(Object *self, Object *args)
{
  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!tuple_check(args)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(args));
    return NULL;
  }

  TupleObject *patt = (TupleObject *)self;
  TupleObject *some = (TupleObject *)args;

  if (patt->size != some->size)
    return bool_false();

  Object *res;
  Object *ob1, *ob2;
  for (int i = 0; i < patt->size; ++i) {
    ob1 = patt->items[i];
    ob2 = some->items[i];
    if (ob1 != NULL) {
      res = object_call(ob1, "__match__", ob2);
      if (bool_isfalse(res)) {
        return res;
      }
      OB_DECREF(res);
    }
  }

  return bool_true();
}

static MethodDef tuple_methods[] = {
  {"length",      NULL, "i",  tuple_length },
  {"__getitem__", "i",  "A",  tuple_getitem},
  {"__match__",   "Llang.Tuple;", "z", tuple_match},
  {NULL}
};

Object *tuple_str(Object *self, Object *ob)
{
  if (self == NULL)
    return NULL;

  if (!tuple_check(self)) {
    error("object of '%.64s' is not a Tuple", OB_TYPE_NAME(self));
    return NULL;
  }

  TupleObject *tuple = (TupleObject *)self;
  STRBUF(sbuf);
  strbuf_append_char(&sbuf, '(');
  Object *str;
  Object *tmp;
  int size = tuple->size;
  for (int i = 0; i < size; ++i) {
    tmp = tuple->items[i];
    if (string_check(tmp)) {
      strbuf_append_char(&sbuf, '"');
      strbuf_append(&sbuf, string_asstr(tmp));
      strbuf_append_char(&sbuf, '"');
    } else {
      str = object_call(tmp, "__str__", NULL);
      strbuf_append(&sbuf, string_asstr(str));
      OB_DECREF(str);
    }
    if (i < size - 1)
      strbuf_append(&sbuf, ", ");
  }
  strbuf_append(&sbuf, ")");
  str = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);

  return str;
}

TypeObject tuple_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Tuple",
  .clean   = tuple_clean,
  .free    = tuple_free,
  .str     = tuple_str,
  .match   = tuple_match,
  .methods = tuple_methods,
};

void init_tuple_type(void)
{
  tuple_type.desc = desc_from_tuple;
  if (type_ready(&tuple_type) < 0)
    panic("Cannot initalize 'Tuple' type.");
}

void *tuple_iter_next(struct iterator *iter)
{
  TupleObject *tuple = iter->iterable;
  if (tuple->size <= 0)
    return NULL;

  if (iter->index < tuple->size) {
    iter->item = tuple_get((Object *)tuple, iter->index);
    ++iter->index;
  } else {
    iter->item = NULL;
  }
  return iter->item;
}
