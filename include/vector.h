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

#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#include "memory.h"
#include "iterator.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_MINIMUM_CAPACITY 16

/* a dynamic array */
typedef struct vector {
  /* total slots */
  int capacity;
  /* used slots */
  int size;
  /* array of items */
  void *items;
} Vector;

/* Declare an empty vector with name. */
#define VECTOR(name) \
  Vector name = {0, 0, NULL}

/* Initialize a vector with item size. */
static inline void vector_init(Vector *self)
{
  memset(self, 0, sizeof(Vector));
}

/* Initialize a vector with capacity, and items ara allocated.
 * It's accessed randomly at 0 ..< capacity
 */
void vector_init_capacity(Vector *self, int capacity);

/* Destroy a vector */
void vector_fini(Vector *self);

/* Create a new vector */
#define vector_new() \
  (Vector *)kmalloc(sizeof(Vector));

/* Destroy a vector, and free vector memory */
#define vector_free(self) \
({                        \
  vector_fini(self);      \
  kfree(self);            \
  (self) = NULL;          \
})

/* Reserve 'size' spaces */
int vector_reserve(Vector *self, int size);

/*
 * Remove all items from the vector, leaving the container with a size of 0.
 * The vector does not resize after removing the items. The memory is still
 * allocated for future uses.
 */
static inline void vector_clear(Vector *self)
{
  memset(self->items, 0, self->capacity * sizeof(void *));
  self->size = 0;
}

/*
 * Concatenate a vector's items into the end of another vector.
 * The source vector is unchanged.
 */
int vector_concat(Vector *self, Vector *other);

/*
 * Create a new vector from a range of an existing vector.
 * The source vector is unchanged.
 */
Vector *vector_slice(Vector *self, int start, int size);

/*
 * Copy the vector's items to a new vector.
 * The source vector is unchanged.
 */
#define vector_clone(self) \
  vector_slice(self, 0, (self)->size)

/* Shrink the vector's memory to it's size for saving memory. */
int vector_shrink_to_fit(Vector *self);

/* Get the vector size. */
#define vector_size(self) \
  ((self) ? (self)->size : 0)

/* Get the vector capacity(allocated spaces). */
#define vector_capacity(self) \
  ((self) ? (self)->capacity : 0)

/* Store an item at an index. The old item is returned. */
void *vector_set(Vector *self, int index, void *item);

/* Get the item stored at an index. Index bound is checked. */
void *vector_get(Vector *self, int index);

/*
 * Insert an item into the in-bound of the vector.
 * This is relatively expensive operation.
 */
int vector_insert(Vector *self, int index, void *item);

/*
 * Remove the item at the index and shrink the vector by one.
 * The removed item is stored va 'prev'.
 * This is relatively expensive operation.
 */
void *vector_remove(Vector *self, int index);

/* Add an item at the end of the vector. */
int vector_push_back(Vector *self, void *item);

/*
 * Remove an item at the end of the vector.
 * When used with 'push_back', the vector can be used as a stack.
 */
void *vector_pop_back(Vector *self);

/* Get an item at the end of the vector, but not remove it. */
static inline void *vector_top_back(Vector *self)
{
  if (self == NULL) return NULL;
  return vector_get(self, self->size - 1);
}

/*
 * Add an integer at the end of the vector and return its index
 */
int vector_append_int(Vector *self, int val);

/*
 * Sort a vector in-place.
 *
 * self
 * compare - The function used to compare two items.
 *           Returns -1, 0, or 1 if the item is less than, equal to,
 *           or greater than the other one.
 *
 * Returns nothing.
 *
 * Examples:
 *   int str_cmp(const void *v1, const void *v2) {
 *     const char *ch1 = *(const char **)v1;
 *     const char *ch2 = *(const char **)v2;
 *     return strcmp(ch1, ch2);
 *   }
 *   vector_sort(vec, str_cmp);
 */
#define vector_sort(self, compare) \
  qsort((self)->items, (self)->size, compare)

/* Convert vector to array with null-terminated item. */
void *vector_toarr(Vector *self);

/*
 * Iterator callback function for vector iteration.
 * See iterator.h.
 */
void *vector_iter_next(Iterator *iter);

/* Declare an iterator of the vector. Deletion is not safe. */
#define VECTOR_ITERATOR(name, vector) \
  ITERATOR(name, vector, vector_iter_next)

/*
 * Reverse iterator callback function for vector iteration.
 * See iterator.h.
 */
void *vector_iter_prev(Iterator *iter);

/* Declare an reverse iterator of the vector. Deletion is not safe. */
#define VECTOR_REVERSE_ITERATOR(name, vector) \
  ITERATOR(name, vector, vector_iter_prev)

/* Vector foreach */
#define vector_for_each(item, vector) \
  for (int idx = 0; idx < vector_size(vector) && \
    ({item = vector_get(vector, idx); 1;}); ++idx)

/* Vector foreach reversely */
#define vector_for_each_reverse(item, vector) \
  for (int idx = vector_size(vector) - 1; idx >= 0 && \
    ({item = vector_get(vector, idx); 1;}); --idx)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VECTOR_H_ */
