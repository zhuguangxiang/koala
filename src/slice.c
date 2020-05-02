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

#include "slice.h"

#define MINIMUM_CAPACITY 8
#define DOUBLE_GROWTH_CAPACITY 1024
#define GROWTH_FACTOR  (5 / 4)

typedef struct sarray {
  // Pointer to the stored memory
  char *objs;
  // This struct's reference count
  int refcnt;
  // Size of each element in bytes
  int objsize;
  // Capacity in number of elements
  int capacity;
} SArray;

static SArray *new_sarray(int objsize, int capacity)
{
  SArray *sarr = kmalloc(sizeof(SArray));
  if (sarr == NULL) return NULL;
  capacity = MAX(MINIMUM_CAPACITY, capacity);
  char *objs = kmalloc(objsize * capacity);
  if (objs == NULL) {
    kfree(sarr);
    return NULL;
  }

  sarr->objs = objs;
  sarr->refcnt = 1;
  sarr->objsize = objsize;
  sarr->capacity = capacity;
  return sarr;
}

static inline void free_sarray(SArray *sarr)
{
  kfree(sarr->objs);
  kfree(sarr);
}

static int expand_sarray(SArray *sarr, int new_cap)
{
  int capacity = sarr->capacity;
  while (capacity < new_cap) {
    if (capacity < DOUBLE_GROWTH_CAPACITY) {
      capacity = capacity << 1;
    } else {
      capacity = capacity * GROWTH_FACTOR;
    }
  }

  void *objs = kmalloc(capacity * sarr->objsize);
  if (objs == NULL) return -1;
  if (sarr->objs != NULL) {
    memcpy(objs, sarr->objs, sarr->capacity * sarr->objsize);
    kfree(sarr->objs);
  }
  sarr->objs = objs;
  sarr->capacity = capacity;
  return 0;
}

int slice_init_capacity(Slice *self, int objsize, int capacity)
{
  SArray *sarr = new_sarray(objsize, capacity);
  if (sarr == NULL) return -1;
  self->ptr = sarr;
  self->offset = 0;
  self->length = 0;
  return 0;
}

#define __SARR(slice) ((SArray *)((slice)->ptr))
#define __OBJSIZE(slice) __SARR(slice)->objsize
#define __PTR(slice, index) \
  __SARR(slice)->objs + __OBJSIZE(slice) * ((slice)->offset + (index))
#define __SIZE(slice) __OBJSIZE(slice) * (slice)->length

int slice_expand(Slice *self, int length)
{
  SArray *sarr = __SARR(self);
  int new_cap = self->offset + self->length + length;
  if (new_cap > sarr->capacity) {
    if (expand_sarray(sarr, new_cap)) return -1;
  }
  self->length += length;
  return 0;
}

int slice_slice(Slice *self, Slice *src, int offset, int len)
{
  if (offset < 0 || len < 0) return -1;
  if (offset + len > src->length) return -1;
  SArray *sarr = __SARR(src);
  ++sarr->refcnt;
  self->ptr = src->ptr;
  self->offset = src->offset + offset;
  self->length = len;
  return 0;
}

void slice_fini(Slice *self)
{
  SArray *sarr = __SARR(self);
  if (--sarr->refcnt <= 0) free_sarray(sarr);
  memset(self, 0, sizeof(Slice));
}

int slice_clear(Slice *self)
{
  SArray *sarr = __SARR(self);
  void *ptr = __PTR(self, 0);
  int size = __SIZE(self);
  memset(ptr, 0, size);
  return 0;
}

int slice_reset(Slice *self)
{
  slice_clear(self);
  self->length = 0;
  return 0;
}

int slice_extend(Slice *dst, Slice *src)
{
  void *pdst = slice_ptr(dst, dst->length);
  void *psrc = slice_ptr(src, 0);
  if (slice_expand(dst, src->length)) return -1;
  memcpy(pdst, psrc, __SIZE(src));
  return 0;
}

int slice_push_array(Slice *self, void *arr, int len)
{
  void *pdst = slice_ptr(self, self->length);
  if (slice_expand(self, len)) return -1;
  int size = __OBJSIZE(self) * len;
  memcpy(pdst, arr, size);
  return 0;
}

void *slice_ptr(Slice *self, int index)
{
  return __PTR(self, index);
}

int slice_set(Slice *self, int index, void *obj)
{
  if (index < 0 || index > self->length) return -1;
  if (index == self->length && slice_expand(self, 1)) return -1;
  void *ptr = __PTR(self, index);
  memcpy(ptr, obj, __OBJSIZE(self));
  return 0;
}

int slice_get(Slice *self, int index, void *obj)
{
  if (index < 0 || index >= self->length) return -1;
  void *ptr = __PTR(self, index);
  memcpy(obj, ptr, __OBJSIZE(self));
  return 0;
}

static void __move_right(Slice *self, int index)
{
  /* the location to start to move */
  char *offset = slice_ptr(self, index);

  /* how many bytes to be moved to the right */
  int nbytes = (self->length - index) * __OBJSIZE(self);

  /* NOTES: do not use memcpy */
  memmove(offset + __OBJSIZE(self), offset, nbytes);
}

int slice_insert(Slice *self, int index, void *obj)
{
  if (index < 0 || index > self->length) return -1;
  if (slice_expand(self, 1)) return -1;
  void *pdst = slice_ptr(self, index);
  __move_right(self, index);
  memcpy(pdst, obj, __OBJSIZE(self));
  return 0;
}

static void __move_left(Slice *self, int index)
{
  /* the location to start to move */
  void *offset = slice_ptr(self, index);

  /* how many bytes to be moved to the left */
  int nbytes = (self->length - index - 1) * __OBJSIZE(self);

  /* NOTES: do not use memcpy */
  memmove(offset, offset + __OBJSIZE(self), nbytes);
}

int slice_remove(Slice *self, int index, void *obj)
{
  if (index < 0 || index >= self->length) return -1;
  void *psrc = slice_ptr(self, index);
  if (obj != NULL) memcpy(obj, psrc, __OBJSIZE(self));
  __move_left(self, index);
  --self->length;
  return 0;
}

int slice_swap(Slice *self, int idx1, int idx2)
{
  if (idx1 < 0 || idx1 >= self->length) return -1;
  if (idx2 < 0 || idx2 >= self->length) return -1;
  if (idx1 == idx2) return 0;
  int size = __OBJSIZE(self);
  char tmp[size];
  void *ptr1 = slice_ptr(self, idx1);
  memcpy(tmp, ptr1, size);
  void *ptr2 = slice_ptr(self, idx2);
  memcpy(ptr1, ptr2, size);
  memcpy(ptr2, tmp, size);
  return 0;
}

void slice_reverse(Slice *self)
{
  int i = 0;
  int j = self->length - 1;
  while (i < j) {
    slice_swap(self, i, j);
    ++i; --j;
  }
}

int slice_index(Slice *self, void *obj, __compar_fn_t cmp)
{
  void *p;
  int i;
  slice_foreach(p, i, self) {
    if (!cmp(p, obj)) return i;
  }
  return -1;
}

int slice_last_index(Slice *self, void *obj, __compar_fn_t cmp)
{
  void *p;
  int i;
  slice_foreach_rev(p, i, self) {
    if (!cmp(p, obj)) return i;
  }
  return -1;
}

void slice_sort(Slice *self, __compar_fn_t cmp)
{
  void *ptr = slice_ptr(self, 0);
  qsort(ptr, self->length, __OBJSIZE(self), cmp);
}
