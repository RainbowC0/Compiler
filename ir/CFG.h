#pragma once

#include <vector>

class Function;

struct BasicBlock {
    /// @brief 开始代码(Label)，结束代码(Goto)索引
    int beginCode, endCode;
    /// @brief 后继代码索引(Label)
    union {int next, nextT;};
    int nextF;
};

struct CFG {
    std::vector<BasicBlock*> inters;
    int start, curr;
    void buildCFG(Function *);
    void dumpCFG(Function *, const char *file);
};
