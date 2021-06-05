/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

int tp_size(uint32 tp_map, int index)
{
    static int __size__[] = { 0, PTR_SIZE, 1, 2, 4, 8, 2, 4, 8, 1, 4 };
    return __size__[tp_index(tp_map, index)];
}

/* var obj T(Any) */
int32 generic_any_hash(uintptr obj, int isref)
{
    if (!isref) {
        return (int32)mem_hash(&obj, sizeof(uintptr));
    } else {
        TypeInfo *type = ((Object *)obj)->type;
        int32 (*fn)(uintptr) = (void *)type->vtbl[0][0];
        return fn(obj);
    }
}

int8 generic_any_equal(uintptr obj, uintptr other, int isref)
{
    if (obj == other) return 1;

    if (isref) {
        TypeInfo *type = ((Object *)obj)->type;
        int32 (*fn)(uintptr, uintptr) = (void *)type->vtbl[0][1];
        return fn(obj, other);
    }
}

Object *generic_any_type(uintptr obj, int tpkind)
{
    return NULL;
}

Object *generic_any_tostr(uintptr obj, int tpkind)
{
    return NULL;
}

/* `Any` type */
static TypeInfo any_type = {
    .name = "Any",
    .flags = TF_TRAIT | TF_PUB,
};

int32 any_hash(Object *self)
{
    return (int32)mem_hash(&self, sizeof(void *));
}

bool any_equal(Object *self, Object *other)
{
    if (self->type != other->type) return 0;
    return self == other;
}

Object *any_get_type(Object *self)
{
    return NULL;
}

Object *any_tostr(Object *self)
{
    char buf[64];
    TypeInfo *type = self->type;
    snprintf(buf, sizeof(buf) - 1, "%.32s@%x", type->name, PTR2INT(self));
    return NULL;
}

// clang-format off

/* DON'T change the order */
static MethodDef any_methods[] = {
    METHOD_DEF("__hash__", NIL, "i32",    any_hash),
    METHOD_DEF("__eq__",   "A", "b",      any_equal),
    METHOD_DEF("__type__", NIL, "LType;", any_get_type),
    METHOD_DEF("__str__",  NIL, "s",      any_tostr),
};

// clang-format on

typedef struct _LroInfo LroInfo;

struct _LroInfo {
    TypeInfo *type;
    /* index of virtual table */
    int index;
};

static HashMap *mtbl_create(void)
{
    HashMap *mtbl = mm_alloc_obj(mtbl);
    hashmap_init(mtbl, NULL);
    return mtbl;
}

static inline HashMap *__get_mtbl(TypeInfo *type)
{
    HashMap *mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mtbl_create();
        type->mtbl = mtbl;
    }
    return mtbl;
}

static int lro_find(Vector *lro, TypeInfo *type)
{
    LroInfo *item;
    vector_foreach(item, lro, {
        if (item->type == type) return 1;
    });
    return 0;
}

static inline Vector *__get_lro(TypeInfo *type)
{
    Vector *vec = type->lro;
    if (!vec) {
        vec = vector_create(sizeof(LroInfo));
        type->lro = vec;
    }
    return vec;
}

static void build_one_lro(TypeInfo *type, TypeInfo *one)
{
    if (!one) return;

    Vector *vec = __get_lro(type);

    LroInfo lro = { NULL, -1 };
    LroInfo *item;
    vector_foreach(item, one->lro, {
        if (!lro_find(vec, item->type)) {
            lro.type = item->type;
            vector_push_back(vec, &lro);
        }
    });

    if (!lro_find(vec, one)) {
        lro.type = one;
        vector_push_back(vec, &lro);
    }
}

static void build_lro(TypeInfo *type)
{
    /* add Any type */
    build_one_lro(type, &any_type);

    /* add first class or trait */
    build_one_lro(type, type->base);

    /* add traits */
    TypeInfo **trait;
    vector_foreach(trait, type->traits, { build_one_lro(type, *trait); });

    /* add self */
    build_one_lro(type, type);
}

static void inherit_methods(TypeInfo *type)
{
    Vector *lro = __get_lro(type);
    int size = vector_size(lro);
    HashMap *mtbl = __get_mtbl(type);
    Vector *vec;
    MethodObject **m;
    LroInfo *item;
    for (int i = size - 2; i >= 0; i--) {
        /* omit type self */
        item = vector_get_ptr(lro, i);
        vec = item->type->methods;
        vector_foreach(m, vec, { hashmap_put_absent(mtbl, &(*m)->entry); });
    }
}

static int vtbl_same(Vector *v1, int len1, Vector *v2)
{
    if (vector_size(v2) != len1) return 0;

    LroInfo *item1, *item2;
    for (int i = 0; i <= len1; i++) {
        item1 = vector_get_ptr(v1, i);
        item2 = vector_get_ptr(v2, i);
        if (!item1 || !item2) return 0;
        if (item1->type != item2->type) return 0;
    }
    return 1;
}

static uintptr *build_one_vtbl(TypeInfo *type, HashMap *mtbl)
{
    Vector *lro = __get_lro(type);
    LroInfo *item;
    Vector *vec;

    /* calculae length */
    int length = 0;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        length += vector_size(vec);
    });

    /* build virtual table */
    uintptr *slots = mm_alloc(sizeof(uintptr) * (length + 1));
    int index = 0;
    MethodObject **m;
    MethodObject *n;
    HashMapEntry *e;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        vector_foreach(m, vec, {
            e = hashmap_get(mtbl, &(*m)->entry);
            assert(e);
            n = CONTAINER_OF(e, MethodObject, entry);
            *(slots + index++) = (uintptr)n->ptr;
        });
    });

    return slots;
}

static void build_vtbl(TypeInfo *type)
{
    /* first class or trait is the same virtual table, at 0 */
    Vector *lro = __get_lro(type);
    TypeInfo *base = type;
    LroInfo *item;
    while (base) {
        vector_foreach(item, lro, {
            if (item->type == base) {
                item->index = 0;
                break;
            }
        });
        base = base->base;
    }

    /* vtbl of 'Any' is also at 0 */
    item = vector_first_ptr(lro);
    item->index = 0;

    /* update some traits indexes */
    vector_foreach_reverse(item, lro, {
        if (item->index == -1) {
            if (vtbl_same(lro, i, item->type->lro)) item->index = 0;
        }
    });

    /* calculate number of slots */
    int num_slot = 2;
    vector_foreach(item, lro, {
        if (item->index == -1) num_slot++;
    });

    uintptr **slots = mm_alloc(sizeof(uintptr *) * num_slot);
    HashMap *mtbl = __get_mtbl(type);

    /* build #0 slot virtual table */
    slots[0] = build_one_vtbl(type, mtbl);

    /* build other traits virtual tables */
    int j = 0;
    vector_foreach(item, lro, {
        if (item->index == -1) {
            item->index = ++j;
            slots[j] = build_one_vtbl(item->type, mtbl);
            assert(j < num_slot);
        }
    });

    /* set virtual table */
    type->vtbl = slots;
}

TypeInfo *kl_type_new(char *path, char *name, int flags, Vector *params,
                      TypeInfo *base, Vector *traits)
{
    TypeInfo *tp = mm_alloc_obj(tp);
    // tp->type = type_type;
    tp->name = name;
    tp->flags = flags;
    tp->params = params;
    tp->base = base;
    tp->traits = traits;
    return tp;
}

void kl_add_field(TypeInfo *type, Object *field)
{
}

void kl_add_method(TypeInfo *type, Object *meth)
{
}

void kl_type_ready(TypeInfo *type)
{
    build_lro(type);
    inherit_methods(type);
    build_vtbl(type);
}

void kl_init_types(void)
{
}

void kl_fini_types(void)
{
}

void kl_type_show(TypeInfo *type)
{
    if (kl_is_pub(type)) printf("pub ");

    if (kl_is_final(type)) printf("final ");

    if (kl_is_class(type))
        printf("class ");
    else if (kl_is_trait(type))
        printf("trait ");
    else if (kl_is_enum(type))
        printf("enum ");
    else if (kl_is_mod(type))
        printf("module ");
    else
        assert(0);

    printf("%s:\n", type->name);

    /* show lro */
    printf("lro:\n");
    LroInfo *item;
    vector_foreach(item, type->lro, {
        if (i < len - 1)
            printf("%s <- ", item->type->name);
        else
            printf("%s", item->type->name);
    });
    printf("\n");
    printf("----============----\n");
}

#ifdef __cplusplus
}
#endif
