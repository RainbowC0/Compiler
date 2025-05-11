#include "Function.h"
#include "CFG.h"
#include "GotoInstruction.h"

void CFG::buildCFG(Function *func) {
    std::vector<Instruction*> *insts = &func->getInterCode().getInsts();
    int num_lab = 0;
    std::string g;
    for (int i=0, j=insts->size(); i<j; i++) {
        auto blk = new BasicBlock({i++});
        while (i<j && insts->at(i)->getOp()!=IRINST_OP_LABEL) {
            i++;
        }
        if (i>=j) {
            blk->endCode = j-1;
            inters.push_back(blk);
            break;
        }
        ((LabelInstruction*)insts->at(i))->labIndex = ++num_lab;
        if (insts->at(i-1)->getOp() != IRINST_OP_GOTO) {
            insts->insert(insts->begin()+i, new GotoInstruction(func, insts->at(i)));
            i++;
            j++;
        }
        blk->endCode = --i;
        inters.push_back(blk);
    }
}

void putLabel(Instruction *ins, FILE *f) {
    if (ins->getOp()==IRINST_OP_LABEL)
        fprintf(f, "L%d", ((LabelInstruction*)ins)->labIndex);
    else
        fputs("L0_", f);
}

void CFG::dumpCFG(Function *func, const char *file) {
    auto insts = func->getInterCode().getInsts();
    FILE *f = fopen(file, "w");
    fputs("digraph {\n", f);
    std::string lab;
    for (int i=0, l=inters.size(); i<l; i++) {
        BasicBlock *blk = inters[i];
        int j=blk->beginCode, k=blk->endCode;
        putLabel(insts[j], f);
        fputs(" [shape=record,label=\"{", f);
        for (;j<=k;j++)
        {
            insts[j]->toString(lab);
            fputs(lab.c_str(), f);
            fputs("\\l", f);
        }
        fputs("}\"];\n", f);
        j = blk->beginCode;
        if (insts[k]->getOp()==IRINST_OP_GOTO) {
            auto x=(GotoInstruction*)insts[k];
            putLabel(insts[j], f);
            fputs(" -> ", f);
            putLabel(x->iftrue, f);
            fputs("\n", f);
            if (x->iffalse) {
                putLabel(insts[j], f);
                fputs(" -> ", f);
                putLabel(x->iffalse, f);
                fputs("\n", f);
            }
        }
    }
    fputs("}", f);
    fclose(f);
}
