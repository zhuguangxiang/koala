/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "atom.h"
#include "memory.h"
#include "log.h"

void literal_show(literal *val, StrBuf *sbuf)
{
  char buf[128];
  switch (val->kind) {
  case 0:
    strbuf_append(sbuf, "(nil)");
    break;
  case BASE_INT:
    sprintf(buf, "%ld", val->ival);
    strbuf_append(sbuf, buf);
    break;
  case BASE_FLOAT:
    sprintf(buf, "%.8f", val->fval);
    strbuf_append(sbuf, buf);
    break;
  case BASE_BOOL:
    strbuf_append(sbuf, val->bval ? "true" : "false");
    break;
  case BASE_STR:
    sprintf(buf, "'%s'", val->str);
    strbuf_append(sbuf, buf);
    break;
  case BASE_CHAR:
    sprintf(buf, "%d", val->cval.val);
    strbuf_append(sbuf, buf);
    break;
  default:
    panic("invalid branch %d", val->kind);
    break;
  }
}

static typedesc bases[] = {
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_INT  },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_STR  },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_ANY  },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_BOOL },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_BYTE },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_CHAR },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_FLOAT},
};

static struct {
  char base;
  char *str;
} basestrs[] = {
  {BASE_INT,   "int"   },
  {BASE_STR,   "string"},
  {BASE_ANY,   "any"   },
  {BASE_BOOL,  "bool"  },
  {BASE_BYTE,  "byte"  },
  {BASE_CHAR,  "char"  },
  {BASE_FLOAT, "float" },
};

char *base_str(int kind)
{
  for (int i = 0; i < COUNT_OF(basestrs); ++i) {
    if (basestrs[i].base == kind)
      return basestrs[i].str;
  }
  return NULL;
}

typedesc *desc_from_base(int kind)
{
  typedesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->base == kind)
      return desc_incref(p);
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  typedesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->refcnt != 1)
      panic("type of '%s' refcnt is %d, but not 1",
        base_str(p->base), p->refcnt);
  }
}

void init_typedesc(void)
{

}

void fini_typedesc(void)
{
  check_base_refcnt();
}

typedesc *desc_from_klass(char *path, char *type, vector *types)
{
  typedesc *desc = kmalloc(sizeof(typedesc));
  desc->kind = TYPE_KLASS;
  desc->refcnt = 1;
  desc->klass.path = path;
  desc->klass.type = type;
  desc->klass.types = types;
  return desc;
}

typedesc *desc_from_proto(vector *args, typedesc *ret)
{
  typedesc *desc = kmalloc(sizeof(typedesc));
  desc->kind = TYPE_PROTO;
  desc->refcnt = 1;
  desc->proto.args = args;
  desc->proto.ret = desc_incref(ret);
  return desc;
}

typedesc *desc_from_array(typedesc *para)
{
  vector *types = vector_new();
  vector_push_back(types, para);
  return desc_from_klass("lang", "Array", types);
}

void desc_free(typedesc *desc)
{
  int kind = desc->kind;
  switch (kind) {
  case TYPE_KLASS: {
    vector *vec = desc->klass.types;
    vector_iterator(iter, vec);
    typedesc *tmp;
    iter_for_each(&iter, tmp) {
      desc_decref(tmp);
    }
    vector_free(vec, NULL, NULL);
    kfree(desc);
    break;
  }
  case TYPE_PROTO: {
    vector *vec = desc->proto.args;
    vector_iterator(iter, vec);
    typedesc *tmp;
    iter_for_each(&iter, tmp) {
      desc_decref(tmp);
    }
    vector_free(vec, NULL, NULL);
    desc_decref(desc->proto.ret);
    kfree(desc);
    break;
  }
  default:
    panic("invalid type '%d' branch.", kind);
    break;
  }
}

void desc_tostr(typedesc *desc, StrBuf *buf)
{

}

void desc_show(typedesc *desc)
{
  if (desc != NULL) {
    STRBUF(sbuf);
    desc_tostr(desc, &sbuf);
    puts(strbuf_tostr(&sbuf)); /* with newline */
    strbuf_fini(&sbuf);
  } else {
    puts("<undefined-type>");  /* with newline */
  }
}

/* desc1 <- desc2 */
int desc_check(typedesc *desc1, typedesc *desc2)
{
  if (desc1 == desc2)
    return 1;

  if (desc_isany(desc1))
    return 1;

  if (desc1->kind != desc2->kind)
    return 0;

  switch (desc1->kind) {
  case TYPE_KLASS:
    break;
  case TYPE_PROTO:
    break;
  default:
    break;
  }
  return 0;
}

static typedesc *__to_klass(char *s, int len)
{
  char *dot = strrchr(s, '.');
  if (!dot)
    panic("null pointer");
  char *path = atom_nstring(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return desc_from_klass(path, type, NULL);
}

static typedesc *__to_desc(char **str, int _dims, int _varg)
{
  char *s = *str;
  char ch = *s;
  typedesc *desc;

  switch (ch) {
  case 'L': {
    s++;
    char *k = s;
    while (*s != ';') s++;
    desc = __to_klass(k, s - k);
    s++;
    break;
  }
  default: {
    if (_dims > 0 && _varg > 0)
     panic("invalid dims '%d' & varg '%d'.", _dims, _varg);
    desc = desc_from_base(ch);
    s++;
    break;
  }
  }
  *str = s;
  return desc;
}

typedesc *str_to_desc(char *s)
{
  if (s == NULL)
    return NULL;
  return __to_desc(&s, 0, 0);
}

typedesc *str_to_proto(char *ptype, char *rtype)
{
  vector *args = NULL;
  if (ptype != NULL) {
    args = vector_new();
    char *s = ptype;
    typedesc *desc;
    while (*s) {
      desc = __to_desc(&s, 0, 0);
      vector_push_back(args, desc);
    }
  }

  typedesc *ret = NULL;
  if (rtype != NULL) {
    char *s = rtype;
    ret = __to_desc(&s, 0, 0);
  }

  typedesc *desc;
  desc = desc_from_proto(args, ret);
  desc_decref(ret);
  return desc;
}
