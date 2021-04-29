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

#include "klvm/klvm.h"

void build_example_1(KLVMModuleRef mod)
{
    KLVMTypeRef proto = KLVMTypeProto(KLVMTypeInt32(), NULL, 0);
    KLVMValueRef fn = KLVMAddFunction(mod, "example_1", proto);
    KLVMValueRef x = KLVMAddLocal(fn, "x", KLVMTypeInt32());
    KLVMValueRef y = KLVMAddLocal(fn, "y", KLVMTypeInt32());
    KLVMValueRef z = KLVMAddLocal(fn, "z", KLVMTypeInt32());
    KLVMValueRef ret = KLVMAddLocal(fn, "RET", KLVMTypeInt32());

    KLVMBuilder bldr;
    KLVMBasicBlockRef b1 = KLVMAppendBlock(fn, "b1");
    KLVMSetBuilderAtEnd(&bldr, b1);
    KLVMBuildCopy(&bldr, x, KLVMConstInt32(3));
    KLVMBuildCopy(&bldr, y, x);

    KLVMValueRef tmp = KLVMBuildAdd(&bldr, y, KLVMConstInt32(4), "");
    KLVMBuildCopy(&bldr, z, tmp);

    tmp = KLVMBuildAdd(&bldr, x, z, "");
    KLVMBuildCopy(&bldr, z, tmp);

    KLVMBuildCopy(&bldr, ret, z);
    KLVMBuildRet(&bldr, ret);

    KLVMComputeInsnPositions(fn);
}

void show_mod_pos_reg(KLVMModuleRef m)
{
    KLVMValueRef *item;
    VectorForEach(item, &m->functions, {
        KLVMFuncRef fn = *(KLVMFuncRef *)item;
        printf("func: %s, regs: %d\n", fn->name, fn->regs);
        KLVMVarRef *var;
        VectorForEach(var, &fn->locals, {
            printf("var: %s, reg: %d\n", (*var)->name, (*var)->reg);
        });

        KLVMInsnRef insn;
        KLVMBasicBlockRef bb;
        BasicBlockForEach(bb, &fn->bb_list, {
            printf("bb: %s, start: %d, end: %d\n", bb->label, bb->start,
                   bb->end);
            InsnForEach(insn, &bb->insns,
                        { printf("insn-%d: reg:%d\n", insn->pos, insn->reg); });
        });
    });
}

void show_uses(KLVMModuleRef m)
{
    KLVMValueRef *item;
    VectorForEach(item, &m->functions, {
        KLVMFuncRef fn = *(KLVMFuncRef *)item;
        KLVMInsnRef insn;
        KLVMVarRef *var;
        KLVMIntervalRef interval;
        KLVMUseRef use;
        VectorForEach(var, &fn->locals, {
            printf("var: %s(reg: %d), uses:\n", (*var)->name, (*var)->reg);
            interval = (*var)->interval;
            UseForEach(use, &interval->use_list, {
                insn = use->insn;
                printf("insn-%d\n", insn->pos);
            });
        });

        KLVMBasicBlockRef bb;
        BasicBlockForEach(bb, &fn->bb_list, {
            InsnForEach(insn, &bb->insns, {
                if (insn->reg >= 0) {
                    printf("insn: %d(reg: %d), uses:\n", insn->pos, insn->reg);
                    interval = insn->interval;
                    UseForEach(use, &interval->use_list, {
                        insn = use->insn;
                        printf("insn-%d\n", insn->pos);
                    });
                }
            });
        });
    });
}

void test_liveness(KLVMModuleRef mod)
{
    build_example_1(mod);
}

int main(int argc, char *argv[])
{
    KLVMModuleRef m = KLVMCreateModule("liveness");
    test_liveness(m);
    KLVMDumpModule(m);

    KLVMPassGroup grp;
    KLVMInitPassGroup(&grp);
    KLVMAddDotPass(&grp);
    KLVMRunPassGroup(&grp, m);

    show_mod_pos_reg(m);
    show_uses(m);

    KLVMDestroyModule(m);
    return 0;
}
