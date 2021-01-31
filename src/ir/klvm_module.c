/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static KLVMFunc *_new_func(KLVMType *ty, const char *name)
{
    KLVMFunc *fn = mm_alloc(sizeof(*fn));
    INIT_VALUE_HEAD(fn, VALUE_FUNC, ty);
    vector_init(&fn->locals, sizeof(void *));
    list_init(&fn->bblist);
    fn->name = name;

    // add parameters
    Vector *params = KLVMProtoTypeParams(ty);
    fn->num_params = vector_size(params);
    KLVMType **item;
    vector_foreach(item, params) KLVMAddLocVar((KLVMValue *)fn, *item, NULL);

    return fn;
}

KLVMModule *KLVMCreateModule(const char *name)
{
    KLVMModule *m = mm_alloc(sizeof(*m));
    m->name = name;
    vector_init(&m->vars, sizeof(KLVMVar *));
    vector_init(&m->funcs, sizeof(KLVMFunc *));
    return m;
}

KLVMValue *KLVMModuleFunc(KLVMModule *m)
{
    KLVMValue *fn = m->fn;
    if (!fn) {
        KLVMFunc *_fn = _new_func(NULL, "__init__");
        _fn->tag_index = m->tag_index;
        fn = (KLVMValue *)_fn;
        m->fn = fn;
    }
    return fn;
}

void KLVMDestroyModule(KLVMModule *m)
{
    mm_free(m);
}

static inline KLVMVar *_new_var(KLVMType *ty, const char *name)
{
    KLVMVar *var = mm_alloc(sizeof(*var));
    INIT_VALUE_HEAD(var, VALUE_VAR, ty);
    var->name = name;
    return var;
}

KLVMValue *KLVMAddVar(KLVMModule *m, KLVMType *ty, const char *name)
{
    if (!name) {
        printf("error: global variable must have name");
        abort();
    }
    KLVMVar *var = _new_var(ty, name);
    var->flags = KLVM_FLAGS_GLOBAL;
    vector_push_back(&m->vars, &var);
    return (KLVMValue *)var;
}

KLVMValue *KLVMAddLocVar(KLVMValue *fn, KLVMType *ty, const char *name)
{
    KLVMVar *var = _new_var(ty, name);
    vector_push_back(&((KLVMFunc *)fn)->locals, &var);
    if (!name) { var->tag = ((KLVMFunc *)fn)->tag_index++; }
    return (KLVMValue *)var;
}

KLVMValue *KLVMAddFunc(KLVMModule *m, KLVMType *ty, const char *name)
{
    if (!name) {
        printf("error: function must have name");
        abort();
    }

    KLVMFunc *fn = _new_func(ty, name);

    // add to module
    vector_push_back(&m->funcs, &fn);

    return (KLVMValue *)fn;
}

void KLVMSetVarName(KLVMValue *_var, const char *name)
{
    KLVMVar *var = (KLVMVar *)_var;
    var->name = name;
}

KLVMValue *KLVMGetParam(KLVMValue *fn, int index)
{
    KLVMFunc *_fn = (KLVMFunc *)fn;
    int num_params = _fn->num_params;
    if (index < 0 || index >= num_params) {
        printf("error: index %d out of range(0 ..< %d)\n", index, num_params);
        abort();
    }

    KLVMValue *para = NULL;
    vector_get(&_fn->locals, index, &para);
    return para;
}

KLVMBasicBlock *_append_basicblock(KLVMValue *fn, const char *label)
{
    KLVMBasicBlock *bb = mm_alloc(sizeof(*bb));
    list_node_init(&bb->bbnode);
    bb->fn = fn;
    list_init(&bb->insts);
    KLVMFunc *_fn = (KLVMFunc *)fn;
    list_push_back(&_fn->bblist, &bb->bbnode);
    _fn->num_bbs++;
    if (!label || !label[0])
        bb->tag = _fn->bb_tag_index++;
    else
        bb->label = label;
    return bb;
}

KLVMBasicBlock *KLVMAppendBasicBlock(KLVMValue *fn)
{
    return _append_basicblock(fn, NULL);
}

KLVMBasicBlock *KLVMFuncEntryBasicBlock(KLVMValue *_fn)
{
    KLVMFunc *fn = (KLVMFunc *)_fn;
    KLVMBasicBlock *e = fn->entry;
    if (!e) {
        e = _append_basicblock(_fn, "entry");
        fn->entry = e;
    }
    return e;
}

KLVMBuilder *KLVMBasicBlockBuilder(KLVMBasicBlock *bb)
{
    KLVMBuilder *bldr = &bb->bldr;
    bldr->bb = bb;
    bldr->rover = &bb->insts;
    return bldr;
}

KLVMConst *_new_const(void)
{
    KLVMConst *k = mm_alloc(sizeof(*k));
    k->kind = VALUE_CONST;
    return k;
}

KLVMValue *KLVMConstInt32(int32_t ival)
{
    KLVMConst *k = _new_const();
    k->type = KLVMInt32Type();
    k->i32val = ival;
    return (KLVMValue *)k;
}

#ifdef __cplusplus
}
#endif
