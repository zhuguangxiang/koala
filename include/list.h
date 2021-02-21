/*
 * This file is part of the koala project, under the MIT License.
 * Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_LIST_H_
#define _KOALA_LIST_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _List {
    /* Previous list entry */
    struct _List *prev;
    /* Next list entry */
    struct _List *next;
} List;

typedef List *ListRef;

// clang-format off
#define LIST_INIT(name) { &(name), &(name) }
// clang-format on

static inline void init_list(ListRef list)
{
    list->prev = list;
    list->next = list;
}

/* Test whether a list is empty */
static inline int list_empty(const ListRef list)
{
    return list->next == list;
}

/* Insert a new entry between two known consecutive entries */
static inline void __list_add(ListRef new__, ListRef prev, ListRef next)
{
    next->prev = new__;
    new__->next = next;
    new__->prev = prev;
    prev->next = new__;
}

/* Insert a new entry after 'prev' entry */
static inline void list_add_after(ListRef new__, ListRef prev)
{
    __list_add(new__, prev, prev->next);
}

/* Insert a new entry before 'next' entry */
static inline void list_add_before(ListRef new__, ListRef next)
{
    __list_add(new__, next->prev, next);
}

/* Insert a new entry at front */
static inline void list_push_front(ListRef list, ListRef new__)
{
    __list_add(new__, list, list->next);
}

/* Insert a new entry at tail */
static inline void list_push_back(ListRef list, ListRef new__)
{
    __list_add(new__, list->prev, list);
}

/* Remove a entry by making the prev/next entries pointer to each other. */
static inline void __list_remove(ListRef prev, ListRef next)
{
    next->prev = prev;
    prev->next = next;
}

/* Remove an entry from list */
static inline void list_remove(ListRef entry)
{
    __list_remove(entry->prev, entry->next);
    init_list(entry);
}

/* Pop an entry at front */
static inline ListRef list_pop_front(ListRef list)
{
    ListRef entry = list->next;
    if (entry == list) return NULL;
    list_remove(entry);
    return entry;
}

/* Pop an entry at tail */
static inline ListRef list_pop_back(ListRef list)
{
    ListRef entry = list->prev;
    if (entry == list) return NULL;
    list_remove(entry);
    return entry;
}

// clang-format off

/* Get the entry in which is embedded */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* Get the first element from a list */
#define list_first(list, type, member) ({ \
    ListRef head__ = (list); \
    ListRef pos__ = head__->next; \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

/* Get the last element from a list */
#define list_last(list, type, member) ({ \
    ListRef head__ = (list); \
    ListRef pos__ = head__->prev; \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

/* Get the next element from a list */
#define list_next(pos, member, list) ({ \
    ListRef head__ = (list); \
    ListRef nxt__ = (pos)->member.next; \
    nxt__ != head__ ? list_entry(nxt__, typeof(*(pos)), member) : NULL; \
})

/* Get the previous element from a list */
#define list_prev(pos, member, list) ({ \
    ListRef head__ = (list); \
    ListRef prev__ = (pos)->member.prev; \
    prev__ != head__ ? list_entry(prev__, typeof(*(pos)), member) : NULL; \
})

#define list_foreach(v__, member, list, closure) \
    for (v__ = list_first(list, typeof(*(v__)), member); \
         v__; v__ = list_next(v__, member, list)) closure;

#define list_foreach_safe(v__, n__, member, list, closure) \
    for (v__ = list_first(list, typeof(*(v__)), member), \
         n__ = v__ ? list_next(v__, member, list) : NULL; \
         v__ && ({ n__ = list_next(v__, member, list); 1;}); v__ = n__) closure;

// clang-format on

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_LIST_H_ */
