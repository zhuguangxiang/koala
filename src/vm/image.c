/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct str_entry {
    HashMapEntry entry;
    int16_t idx;
    klc_str_t s;
} str_entry_t;

static int str_entry_equal(void *k1, void *k2)
{
    str_entry_t *e1 = k1;
    str_entry_t *e2 = k2;
    return e1->s.len == e2->s.len && !strcmp(e1->s.s, e2->s.s);
}

static int16_t klc_str_append(klc_image_t *klc, char *s, uint16_t len)
{
    klc_str_t str = { len, s };
    Vector *vec = &klc->strs.vec;
    int16_t idx = vector_size(vec);
    vector_push_back(vec, &str);

    str_entry_t *e = mm_alloc(sizeof(*e));
    hashmap_entry_init(e, memhash(s, len));
    e->idx = idx;
    e->s = str;
    hashmap_put_absent(&klc->strtab, e);

    return idx;
}

static int16_t klc_str_get(klc_image_t *klc, char *s, uint16_t len)
{
    str_entry_t key = { .s = { len, s } };
    hashmap_entry_init(&key, memhash(s, len));
    str_entry_t *e = hashmap_get(&klc->strtab, &key);
    return e ? e->idx : -1;
}

static int16_t klc_str_set(klc_image_t *klc, char *s, uint16_t len)
{
    int16_t idx = klc_str_get(klc, s, len);
    if (idx >= 0) return idx;
    return klc_str_append(klc, s, len);
}

static klc_str_t *klc_str_index(klc_image_t *klc, int16_t index)
{
    Vector *vec = &klc->strs.vec;
    return vector_get_ptr(vec, index);
}

typedef struct const_entry {
    HashMapEntry entry;
    int16_t idx;
    klc_const_t val;
} const_entry_t;

static int const_entry_equal(void *k1, void *k2)
{
    const_entry_t *e1 = k1;
    const_entry_t *e2 = k2;
    return !memcmp(&e1->val, &e2->val, sizeof(klc_const_t));
}

static int16_t klc_const_get(klc_image_t *klc, klc_const_t val)
{
    const_entry_t key = { .val = val };
    hashmap_entry_init(&key, memhash(&val, sizeof(val)));
    const_entry_t *e = hashmap_get(&klc->constab, &key);
    return e ? e->idx : -1;
}

static int16_t klc_const_set(klc_image_t *klc, klc_const_t val)
{
    int16_t idx = klc_const_get(klc, val);
    if (idx >= 0) return idx;

    Vector *vec = &klc->consts.vec;
    idx = vector_size(vec);
    vector_push_back(vec, &val);

    const_entry_t *e = mm_alloc(sizeof(*e));
    hashmap_entry_init(e, memhash(&val, sizeof(val)));
    e->idx = idx;
    e->val = val;
    hashmap_put_absent(&klc->constab, e);

    return idx;
}

int16_t klc_add_nil(klc_image_t *klc)
{
    klc_const_t kval = { .tag = CONST_NIL };
    return klc_const_set(klc, kval);
}

int16_t klc_add_int8(klc_image_t *klc, int8_t val)
{
    klc_const_t kval = { .tag = CONST_INT8, .i8 = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_int16(klc_image_t *klc, int16_t val)
{
    klc_const_t kval = { .tag = CONST_INT16, .i16 = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_int32(klc_image_t *klc, int32_t val)
{
    klc_const_t kval = { .tag = CONST_INT32, .i32 = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_int64(klc_image_t *klc, int64_t val)
{
    klc_const_t kval = { .tag = CONST_INT64, .i64 = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_float32(klc_image_t *klc, float val)
{
    klc_const_t kval = { .tag = CONST_FLOAT32, .f32 = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_float64(klc_image_t *klc, double val)
{
    klc_const_t kval = { .tag = CONST_FLOAT64, .f64 = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_bool(klc_image_t *klc, int8_t val)
{
    klc_const_t kval = { .tag = CONST_BOOL, .bval = val };
    return klc_const_set(klc, kval);
}

int16_t klc_add_char(klc_image_t *klc, uint32_t ch)
{
    klc_const_t kval = { .tag = CONST_CHAR, .ch = ch };
    return klc_const_set(klc, kval);
}

int16_t klc_add_string(klc_image_t *klc, char *s)
{
    uint16_t len = strlen(s);
    int16_t idx = klc_str_set(klc, s, len);
    klc_const_t kval = { .tag = CONST_STRING, .str = idx };
    return klc_const_set(klc, kval);
}

typedef struct type_entry {
    HashMapEntry entry;
    int16_t idx;
    klc_type_t type;
} type_entry_t;

static int type_entry_equal(void *k1, void *k2)
{
    type_entry_t *e1 = k1;
    type_entry_t *e2 = k2;
    return e1->type.tag == e2->type.tag;
}

static inline int _isbase(uint8_t kind)
{
    return (kind == TYPE_INT8 || kind == TYPE_INT16 || kind == TYPE_INT32 ||
        kind == TYPE_INT64 || kind == TYPE_FLOAT32 || kind == TYPE_FLOAT64 ||
        kind == TYPE_BOOL || kind == TYPE_STRING || kind == TYPE_CHAR ||
        kind == TYPE_ANY);
}

static inline int _isproto(uint8_t kind)
{
    return kind == TYPE_PROTO;
}

static klc_type_t *_type_index(klc_image_t *klc, int16_t index)
{
    klc_type_t *type = NULL;
    Vector *vec = &klc->types.vec;
    vector_get(vec, index, &type);
    return type;
}

static int16_t _get_base(klc_image_t *klc, uint8_t tag)
{
    type_entry_t key = { .type.tag = tag };
    hashmap_entry_init(&key, tag);
    type_entry_t *e = hashmap_get(&klc->typtab, &key);
    return e ? e->idx : -1;
}

static int16_t _add_base(klc_image_t *klc, uint8_t tag)
{
    int16_t idx = _get_base(klc, tag);
    if (idx >= 0) return idx;

    /* base type */
    klc_type_t *base = mm_alloc(sizeof(*base));
    base->tag = tag;

    /* append type */
    Vector *vec = &klc->types.vec;
    idx = vector_size(vec);
    vector_push_back(vec, &base);

    /* add to type table */
    type_entry_t *e = mm_alloc(sizeof(*e));
    hashmap_entry_init(e, tag);
    e->idx = idx;
    e->type.tag = tag;
    hashmap_put_absent(&klc->typtab, e);

    return idx;
}

static int16_t _add_proto(klc_image_t *klc, int16_t rtype, Vector *ptypes)
{
    klc_type_proto_t *p = mm_alloc(sizeof(*p));
    p->tag = TYPE_PROTO;
    p->rtype = rtype;
    p->ptypes = ptypes;

    /* append type */
    Vector *vec = &klc->types.vec;
    int16_t idx = vector_size(vec);
    vector_push_back(vec, &p);
    return idx;
}

static int16_t klc_type_set(klc_image_t *klc, TypeDesc *type)
{
    if (!type) return -1;

    if (_isbase(type->kind))
        return _add_base(klc, type->kind);
    else if (_isproto(type->kind)) {
        ProtoDesc *proto = (ProtoDesc *)type;

        /* ret type */
        int16_t rtype = klc_type_set(klc, proto->rtype);

        /* para types */
        Vector *ptypes = vector_new(sizeof(int16_t));
        TypeDesc **item;
        int16_t idx;
        vector_foreach(item, proto->ptypes)
        {
            idx = klc_type_set(klc, *item);
            vector_push_back(ptypes, &idx);
        }

        return _add_proto(klc, rtype, ptypes);
    }
    abort();
}

static inline void _add_var(
    klc_image_t *klc, int16_t name_idx, int16_t type_idx, uint8_t flags)
{
    klc_var_t var = { flags, name_idx, type_idx };
    klc_sect_t *vars = &klc->vars;
    vector_push_back(&vars->vec, &var);
}

void klc_add_var(klc_image_t *klc, char *name, TypeDesc *type, uint8_t flags)
{
    int16_t name_idx = klc_str_set(klc, name, strlen(name));
    int16_t type_idx = klc_type_set(klc, type);
    _add_var(klc, name_idx, type_idx, flags);
}

void klc_add_func(klc_image_t *klc, codeinfo_t *ci)
{
    int16_t type_idx = klc_type_set(klc, ci->desc);
}

klc_image_t *klc_create(void)
{
    klc_image_t *klc = mm_alloc(sizeof(*klc));

    /* init string table */
    hashmap_init(&klc->strtab, str_entry_equal);

    /* init const table */
    hashmap_init(&klc->constab, const_entry_equal);

    /* init string table */
    hashmap_init(&klc->typtab, type_entry_equal);

    /* init sections */
    klc->strs.id = SECT_STR;
    vector_init(&klc->strs.vec, sizeof(klc_str_t));

    klc->consts.id = SECT_CONST;
    vector_init(&klc->consts.vec, sizeof(klc_const_t));

    klc->types.id = SECT_TYPE;
    vector_init(&klc->types.vec, sizeof(klc_type_t *));

    klc->vars.id = SECT_VAR;
    vector_init(&klc->vars.vec, sizeof(klc_var_t));

    klc->funcs.id = SECT_FUNC;
    vector_init(&klc->funcs.vec, sizeof(klc_func_t));

    klc->codes.id = SECT_CODE;
    vector_init(&klc->codes.vec, sizeof(klc_code_t *));

    return klc;
}

void klc_destroy(klc_image_t *klc)
{
    mm_free(klc);
}

static void make_pkgs_dir(char *dir, int len)
{
    char cmd[4096] = "mkdir -p ";
    strncat(cmd, dir, len);
    int status = system(cmd);
    if (status) {
        printf("cmd `%s` executed failed.\n", cmd);
        abort();
    }
}

static FILE *open_klc_file(char *path, char *mode)
{
    FILE *fp = fopen(path, mode);
    if (fp == NULL) {
        char *end = strrchr(path, '/');
        make_pkgs_dir(path, end - path);
        fp = fopen(path, "w");
    }
    return fp;
}

#define WRITE_OBJECT(obj) fwrite(&(obj), sizeof(obj), 1, fp)
#define READ_OBJECT(obj)  fread(&(obj), sizeof(obj), 1, fp)

#define WRITE_STRING(str, len) fwrite(str, len + 1, 1, fp)
#define READ_STRING(str, len)  fread(str, len + 1, 1, fp)

static void write_str_sect(klc_image_t *klc, FILE *fp)
{
    uint8_t id;
    Vector *vec;
    klc_str_t *str;
    vec = &klc->strs.vec;
    uint16_t count = vector_size(vec);
    if (count > 0) {
        id = SECT_STR;
        WRITE_OBJECT(id);
        WRITE_OBJECT(count);
    }

    vector_foreach(str, vec)
    {
        WRITE_OBJECT(str->len);
        WRITE_STRING(str->s, str->len);
    }
}

static void read_str_sect(klc_image_t *klc, FILE *fp)
{
    uint16_t count = 0;
    READ_OBJECT(count);
    if (count <= 0) return;

    uint16_t len;
    char *s;
    while (count-- > 0) {
        READ_OBJECT(len);
        s = mm_alloc(len + 1);
        READ_STRING(s, len);
        klc_str_set(klc, s, len);
    }
}

static void write_const_sect(klc_image_t *klc, FILE *fp)
{
    uint8_t id;
    Vector *vec;
    klc_const_t *val;
    vec = &klc->consts.vec;
    uint16_t count = vector_size(vec);
    if (count > 0) {
        id = SECT_CONST;
        WRITE_OBJECT(id);
        WRITE_OBJECT(count);
    }

    vector_foreach(val, vec)
    {
        WRITE_OBJECT(val->tag);
        switch (val->tag) {
            case CONST_INT8:
                WRITE_OBJECT(val->i8);
                break;
            case CONST_INT16:
                WRITE_OBJECT(val->i16);
                break;
            case CONST_INT32:
                WRITE_OBJECT(val->i32);
                break;
            case CONST_INT64:
                WRITE_OBJECT(val->i64);
                break;
            case CONST_FLOAT32:
                WRITE_OBJECT(val->f32);
                break;
            case CONST_FLOAT64:
                WRITE_OBJECT(val->f64);
                break;
            case CONST_BOOL:
                WRITE_OBJECT(val->bval);
                break;
            case CONST_CHAR:
                WRITE_OBJECT(val->ch);
                break;
            case CONST_STRING:
                WRITE_OBJECT(val->str);
                break;
            case CONST_NIL:
                break;
            default:
                abort();
        }
    }
}

static void read_const_sect(klc_image_t *klc, FILE *fp)
{
    uint16_t count = 0;
    READ_OBJECT(count);
    if (count <= 0) return;

    uint8_t tag;
    int8_t i8;
    klc_const_t val;
    while (count-- > 0) {
        READ_OBJECT(tag);
        val = (klc_const_t) {};
        val.tag = tag;
        switch (tag) {
            case CONST_INT8:
                READ_OBJECT(val.i8);
                break;
            case CONST_INT16:
                READ_OBJECT(val.i16);
                break;
            case CONST_INT32:
                READ_OBJECT(val.i32);
                break;
            case CONST_INT64:
                READ_OBJECT(val.i64);
                break;
            case CONST_FLOAT32:
                READ_OBJECT(val.f32);
                break;
            case CONST_FLOAT64:
                READ_OBJECT(val.f64);
                break;
            case CONST_BOOL:
                READ_OBJECT(val.bval);
                break;
            case CONST_CHAR:
                READ_OBJECT(val.ch);
                break;
            case CONST_STRING:
                READ_OBJECT(val.str);
                break;
            case CONST_NIL:
                break;
            default:
                abort();
        }
        klc_const_set(klc, val);
    }
}

static void write_type_sect(klc_image_t *klc, FILE *fp)
{
    uint8_t id;
    Vector *vec;
    klc_type_t **type;
    vec = &klc->types.vec;
    uint16_t count = vector_size(vec);
    if (count > 0) {
        id = SECT_TYPE;
        WRITE_OBJECT(id);
        WRITE_OBJECT(count);
    }

    uint8_t tag;
    vector_foreach(type, vec)
    {
        tag = (*type)->tag;
        WRITE_OBJECT(tag);
        if (_isproto(tag)) {
            klc_type_proto_t *proto = (klc_type_proto_t *)*type;
            WRITE_OBJECT(proto->rtype);
            uint16_t count = vector_size(proto->ptypes);
            WRITE_OBJECT(count);
            int16_t index;
            int i = 0;
            while (count-- > 0) {
                vector_get(proto->ptypes, i++, &index);
                WRITE_OBJECT(index);
            }
        }
    }
}

static void read_type_sect(klc_image_t *klc, FILE *fp)
{
    uint16_t count = 0;
    READ_OBJECT(count);
    if (count <= 0) return;

    uint8_t tag;
    while (count-- > 0) {
        READ_OBJECT(tag);
        if (_isbase(tag))
            _add_base(klc, tag);
        else if (_isproto(tag)) {
            /* ret type */
            int16_t rtype;
            READ_OBJECT(rtype);

            /* para count */
            int16_t count;
            READ_OBJECT(count);

            /* para types */
            Vector *ptypes = NULL;
            if (count > 0) {
                ptypes = vector_new(sizeof(int16_t));
                int16_t idx;
                while (count-- > 0) {
                    READ_OBJECT(idx);
                    vector_push_back(ptypes, &idx);
                }
            }

            _add_proto(klc, rtype, ptypes);
        }
        else
            abort();
    }
}

static void write_var_sect(klc_image_t *klc, FILE *fp)
{
    uint8_t id;
    Vector *vec;
    klc_var_t *var;
    vec = &klc->vars.vec;
    uint16_t count = vector_size(vec);
    if (count > 0) {
        id = SECT_VAR;
        WRITE_OBJECT(id);
        WRITE_OBJECT(count);
    }

    vector_foreach(var, vec)
    {
        WRITE_OBJECT(var->flags);
        WRITE_OBJECT(var->name);
        WRITE_OBJECT(var->type);
    }
}

static void read_var_sect(klc_image_t *klc, FILE *fp)
{
    uint16_t count = 0;
    READ_OBJECT(count);
    if (count <= 0) return;

    uint8_t flags;
    int16_t name_idx;
    int16_t type_idx;
    while (count-- > 0) {
        READ_OBJECT(flags);
        READ_OBJECT(name_idx);
        READ_OBJECT(type_idx);
        _add_var(klc, name_idx, type_idx, flags);
    }
}

void klc_write_file(klc_image_t *klc, char *path)
{
    FILE *fp = open_klc_file(path, "w");
    klc_header_t hdr = { "klc", VERSION_MAJOR, VERSION_MINOR };
    fwrite(&hdr, sizeof(hdr), 1, fp);
    write_str_sect(klc, fp);
    write_const_sect(klc, fp);
    write_type_sect(klc, fp);
    write_var_sect(klc, fp);
    fclose(fp);
}

static int check_header(klc_header_t *hdr)
{
    if (strcmp(hdr->magic, "klc")) return -1;
    if (hdr->major != VERSION_MAJOR) return -1;
    if (hdr->minor != VERSION_MINOR) return -1;
    return 0;
}

klc_image_t *klc_read_file(char *path)
{
    FILE *fp = open_klc_file(path, "r");
    if (!fp) {
        printf("error: cannot open `%s` file\n", path);
        return NULL;
    }

    klc_header_t hdr;
    fread(&hdr, sizeof(hdr), 1, fp);
    if (check_header(&hdr)) {
        printf("error: `%s` is not a valid koala byte code\n", path);
        fclose(fp);
        return NULL;
    }

    klc_image_t *klc = klc_create();

    uint8_t id;
    while (fread(&id, sizeof(id), 1, fp)) {
        switch (id) {
            case SECT_STR:
                read_str_sect(klc, fp);
                break;
            case SECT_CONST:
                read_const_sect(klc, fp);
                break;
            case SECT_TYPE:
                read_type_sect(klc, fp);
                break;
            case SECT_VAR:
                read_var_sect(klc, fp);
                break;
            default:
                printf("error: invalid section(%d)\n", id);
                abort();
        }
    }

    fclose(fp);
    return klc;
}

static void klc_str_show(klc_image_t *klc)
{
    klc_sect_t *sec = &klc->strs;
    printf("\nStrings:\n\n");
    klc_str_t *str;
    vector_foreach(str, &sec->vec)
    {
        printf("  #%u: \"%s\"\n", i, str->s);
    }
}

static void _type_str(klc_image_t *klc, klc_type_t *type, char *buf);

static void _proto_str(klc_image_t *klc, klc_type_t *type, char *buf)
{
    klc_type_proto_t *proto = (klc_type_proto_t *)type;

    strcat(buf, "func(");

    klc_type_t *item;
    int16_t *type_index;
    vector_foreach(type_index, proto->ptypes)
    {
        item = _type_index(klc, *type_index);
        if (i > 0) strcat(buf, ", ");
        _type_str(klc, item, buf);
    }

    strcat(buf, ")");

    item = _type_index(klc, proto->rtype);
    if (item) {
        strcat(buf, " ");
        _type_str(klc, item, buf);
    }
}

static void _type_str(klc_image_t *klc, klc_type_t *type, char *buf)
{
    if (_isbase(type->tag))
        strcat(buf, base_desc_str(type->tag));
    else if (_isproto(type->tag))
        _proto_str(klc, type, buf);
}

static void klc_type_show(klc_image_t *klc)
{
    klc_sect_t *sec = &klc->types;
    printf("\nTypes:\n\n");
    char buf[4096];
    klc_type_t **type;
    vector_foreach(type, &sec->vec)
    {
        buf[0] = '\0';
        _type_str(klc, *type, buf);
        printf("  #%u: %s\n", i, buf);
    }
}

static void klc_const_show(klc_image_t *klc)
{
    klc_sect_t *sec = &klc->consts;
    printf("\nConstant Pool:\n\n");
    klc_const_t *val;
    vector_foreach(val, &sec->vec)
    {
        switch (val->tag) {
            case CONST_INT8:
                printf("  #%u = int8 %d\n", i, val->i8);
                break;
            case CONST_INT16:
                printf("  #%u = int16 %d\n", i, val->i16);
                break;
            case CONST_INT32:
                printf("  #%u = int32 %d\n", i, val->i32);
                break;
            case CONST_INT64:
                printf("  #%u = int64 %ld\n", i, val->i64);
                break;
            case CONST_FLOAT32:
                printf("  #%u = float32 %f\n", i, val->f32);
                break;
            case CONST_FLOAT64:
                printf("  #%u = float64 %f\n", i, val->f64);
                break;
            case CONST_BOOL:
                printf("  #%u = bool %s\n", i, val->bval ? "true" : "false");
                break;
            case CONST_CHAR: {
                char wch[8] = { ((char *)&val->ch)[2], ((char *)&val->ch)[1],
                    ((char *)&val->ch)[0], 0 };
                printf("  #%u = char %x '%s'\n", i, val->ch, wch);
                break;
            }
            case CONST_STRING: {
                klc_str_t *str = klc_str_index(klc, val->str);
                printf("  #%u = string \"%s\"\n", i, str->s);
                break;
            }
            case CONST_NIL:
                printf("  #%u = nil\n", i);
                break;
            default:
                abort();
        }
    }
}

static void klc_var_show(klc_image_t *klc)
{
    klc_sect_t *sec = &klc->vars;
    Vector *strs = &klc->strs.vec;
    Vector *types = &klc->types.vec;
    char buf[4096];
    klc_str_t *str;
    klc_type_t *type;
    klc_var_t *var;
    char *s1;
    vector_foreach(var, &sec->vec)
    {
        str = vector_get_ptr(strs, var->name);
        vector_get(types, var->type, &type);
        buf[0] = '\0';
        _type_str(klc, type, buf);
        s1 = (var->flags & ACCESS_FLAGS_FINAL) ? "const" : "var";
        if (var->flags & ACCESS_FLAGS_PUB)
            printf("  pub %s %s %s;\n", s1, str->s, buf);
        else
            printf("  %s %s %s;\n", s1, str->s, buf);
    }
}

void klc_show(klc_image_t *klc)
{
    printf("\nVersion: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
    klc_str_show(klc);
    klc_type_show(klc);
    klc_const_show(klc);
    printf("\n{\n");
    klc_var_show(klc);
    printf("}\n\n");
}

#ifdef __cplusplus
}
#endif
