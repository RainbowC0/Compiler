#include "CodeGeneratorAsm.h"
#include "SimpleRegisterAllocator.h"
#include "Instruction.h"
#include <vector>

struct LiveRange {
    Value *value;     // 关联的变量/临时值
    int start;        // 起始指令位置（按指令序号）
    int end;          // 结束指令位置
    int reg = -1;     // 分配的寄存器编号（-1表示未分配）
    int stackOffset = -1; // 溢出时的栈偏移

    [[nodiscard]] bool overlaps(const LiveRange &other) const {
        return !(end < other.start || start > other.end);
    }
};

class CodeGeneratorArm64 : public CodeGeneratorAsm {

public:
    /// @brief 构造函数
    /// @param tab 符号表
    CodeGeneratorArm64(Module * module);

    /// @brief 析构函数
    ~CodeGeneratorArm64() override;

protected:
    /// @brief 产生汇编头部分
    void genHeader() override;

    /// @brief 全局变量Section，主要包含初始化的和未初始化过的
    void genDataSection() override;

    /// @brief 针对函数进行汇编指令生成，放到.text代码段中
    /// @param func 要处理的函数
    void genCodeSection(Function * func) override;

    /// @brief 寄存器分配
    /// @param func 要处理的函数
    void registerAllocation(Function * func) override;

    /// @brief 栈空间分配
    /// @param func 要处理的函数
    void stackAlloc(Function * func);

    /// @brief 寄存器分配前对函数内的指令进行调整，以便方便寄存器分配
    /// @param func 要处理的函数
    void adjustFuncCallInsts(Function * func);

    /// @brief 寄存器分配前对形参指令调整，便于栈内空间分配以及寄存器分配
    /// @param func 要处理的函数
    /// @return 栈传值大小
    int adjustFormalParamInsts(Function * func);

    ///
    /// @brief 获取IR变量相关信息字符串
    /// @param str
    ///
    void getIRValueStr(Value * val, std::string & str);

    void linearScanRegisterAllocation(std::vector<LiveRange> &ranges, Function *func);

private:
    ///
    /// @brief 简单的朴素寄存器分配方法
    ///
    SimpleRegisterAllocator simpleRegisterAllocator;
};
