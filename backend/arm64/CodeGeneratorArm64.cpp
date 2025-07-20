///
/// @file CodeGeneratorArm64.cpp
/// @brief ARM64的后端处理实现
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-11-21
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-11-21 <td>1.0     <td>zenglj  <td>新做
/// </table>
///
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <typeinfo>

#include "FormalParam.h"
#include "Function.h"
#include "GotoInstruction.h"
#include "Instruction.h"
#include "LocalVariable.h"
#include "Module.h"
// #include "PlatformArm64.h"
#include "CodeGeneratorArm64.h"
#include "InstSelectorArm64.h"
// #include "SimpleRegisterAllocator.h"
#include "ILocArm64.h"
// #include "RegVariable.h"
#include "FuncCallInstruction.h"
#include "ArgInstruction.h"
#include "MoveInstruction.h"
#include "PlatformArm64.h"
#include "ArrayType.h"
#include "Use.h"

#define DEBUG 1
#ifdef DEBUG
#define PNT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define PNT(fmt, ...)
#endif

extern "C" {
static int allocateStackSlot(Function *, Value *);
static void expireOldRanges(std::vector<LiveRange> & active, std::vector<int> & freeRegs, int pos);
static int findLastUse(Value * val, const std::vector<Instruction *> & insts, int startPos);
static void extendRangeIfExists(std::vector<LiveRange> & ranges, Value * value, int currentPos);
static const std::vector<LiveRange> & calculateLiveRanges(Function * func);
static void extendRange(std::vector<LiveRange> & ranges, int fromPos, int currPos);
}

/// @brief 构造函数
/// @param tab 符号表
CodeGeneratorArm64::CodeGeneratorArm64(Module * _module)
    : CodeGeneratorAsm(_module), simpleRegisterAllocator(PlatformArm64::maxUsableRegNum)
{
    ConstInt::setZeroReg(ARM64_ZR_REG_NO);
    simpleRegisterAllocator.Allocate(ARM64_ZR_REG_NO);
    simpleRegisterAllocator.Allocate(ARM64_FP_REG_NO);
    simpleRegisterAllocator.Allocate(ARM64_LR_REG_NO);
    simpleRegisterAllocator.Allocate(ARM64_TMP_REG_NO);
    simpleRegisterAllocator.Allocate(ARM64_TMP_REG_NO2);
}

/// @brief 析构函数
CodeGeneratorArm64::~CodeGeneratorArm64()
{
    ConstInt::setZeroReg(-1);
}

/// @brief 产生汇编头部分
void CodeGeneratorArm64::genHeader()
{
    // 定义算余数的宏
    fputs(".macro rem dst, divd, divr\n"
          "sdiv \\dst, \\divd, \\divr\n"
          "msub \\dst, \\dst, \\divr, \\divd\n"
          ".endm\n",
          fp);
}

/// @brief 全局变量Section，主要包含初始化的和未初始化过的
void CodeGeneratorArm64::genDataSection()
{

    // 可直接操作文件指针fp进行写操作

    // 目前不支持全局变量和静态变量，以及字符串常量
    // 全局变量分两种情况：初始化的全局变量和未初始化的全局变量
    // TODO 这里先处理未初始化的全局变量
    for (auto var: module->getGlobalVariables()) {

        if (var->isInBSSSection()) {

            // 在BSS段的全局变量，可以包含初值全是0的变量
            fprintf(fp, ".comm %s, %d, %d\n", var->getName().c_str(), var->getType()->getSize(), var->getAlignment());
        } else {

            // 有初值的全局变量
            fprintf(fp, ".type %s, @object\n", var->getName().c_str());
            fputs(".data\n", fp);
            fprintf(fp, ".globl %s\n", var->getName().c_str());
            fprintf(fp, ".align 2\n");
            fprintf(fp, "%s:\n", var->getName().c_str());
            if (var->getType()->isArrayType()) {
                int len = var->getType()->getSize() / 4;
                int rlen = var->intVal ? *(int*)(var->intVal) : 0;
                for (int i = 1; i <= rlen; i++) {
					int v = var->intVal[i];
					if (v == 0) {
						int zero_count = i;
						do {
							i++;
						} while (i <= rlen && var->intVal[i] == 0);
						zero_count = i - zero_count;
						fprintf(fp, ".zero %d\n", zero_count << 2);
						i--;
					} else
                    	fprintf(fp, ".word 0x%x\n", v);
                }
				if ((len -= rlen) > 0) {
					fprintf(fp, ".zero %d\n", len <<2);
				}
                delete var->intVal;
            } else {
                int32_t i = var->intVal ? *var->intVal : 0;
                fprintf(fp, ".word 0x%x\n", i);
                delete var->intVal;
            }
            // TODO 数组初值
        }
    }
}

///
/// @brief 获取IR变量相关信息字符串
/// @param str
///
void CodeGeneratorArm64::getIRValueStr(Value * val, std::string & str)
{
    std::string name = val->getName();
    std::string IRName = val->getIRName();
    int32_t regId = val->getRegId();
    int32_t baseRegId;
    int64_t offset;
    std::string showName;

    if (name.empty() && (!IRName.empty())) {
        showName = IRName;
    } else if ((!name.empty()) && IRName.empty()) {
        showName = name;
    } else if ((!name.empty()) && (!IRName.empty())) {
        showName = name + ":" + IRName;
    } else {
        showName = "";
    }

    if (regId != -1) {
        // 寄存器
        str += "\t@ " + showName + ":" + PlatformArm64::regName[regId];
    } else if (val->getMemoryAddr(&baseRegId, &offset)) {
        // 栈内寻址，[fp,#4]
        str += "\t@ " + showName + ":[" + PlatformArm64::regName[baseRegId] + ",#" + std::to_string(offset) + "]";
    }
}

/// @brief 针对函数进行汇编指令生成，放到.text代码段中
/// @param func 要处理的函数
void CodeGeneratorArm64::genCodeSection(Function * func)
{
    // 生成代码段
    fputs(".text\n", fp);
    // 寄存器分配以及栈内局部变量的站内地址重新分配
    registerAllocation(func);

    // 获取函数的指令列表
    std::vector<Instruction *> & IrInsts = func->getInterCode().getInsts();

    // 汇编指令输出前要确保Label的名字有效，必须是程序级别的唯一，而不是函数内的唯一。要全局编号。
    for (auto inst: IrInsts) {
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            inst->setName(IR_LABEL_PREFIX + std::to_string(labelIndex++));
        }
    }

    // ILOC代码序列
    ILocArm64 iloc(module);

    // 指令选择生成汇编指令
    InstSelectorArm64 instSelector(IrInsts, iloc, func, simpleRegisterAllocator);
    instSelector.setShowLinearIR(false);
    // this->showLinearIR);
    instSelector.run();

    // 删除无用的Label和Goto指令
    iloc.deleteUsed();

    // ILOC代码输出为汇编代码
    fprintf(fp, ".align 2\n"); // 函数统一按2对齐
    fprintf(fp, ".globl %s\n", func->getName().c_str());
    fprintf(fp, ".type %s, @function\n", func->getName().c_str());
    fprintf(fp, "%s:\n", func->getName().c_str());

    // 开启时输出IR指令作为注释
    if (this->showLinearIR) {

        // 输出有关局部变量的注释，便于查找问题
        for (auto localVar: func->getVarValues()) {
            std::string str;
            getIRValueStr(localVar, str);
            if (!str.empty()) {
                fprintf(fp, "%s\n", str.c_str());
            }
        }

        // 输出指令关联的临时变量信息
        for (auto inst: func->getInterCode().getInsts()) {
            if (inst->hasResultValue()) {
                std::string str;
                getIRValueStr(inst, str);
                if (!str.empty()) {
                    fprintf(fp, "%s\n", str.c_str());
                }
            }
        }
    }

    iloc.outPut(fp);
}

/// @brief 寄存器分配
/// @param func 函数指针
void CodeGeneratorArm64::registerAllocation(Function * func)
{
    // 内置函数不需要处理
    if (func->isBuiltin())
        return;

    // 最简单/朴素的寄存器分配策略：局部变量和临时变量都保存在栈内，全局变量在静态存储.data区中
    // R0,R1,R2和R3寄存器不需要保护，可直接使用
    // SP寄存器预留，不需要保护，但需要保证值的正确性
    // R4-R10, fp(11), lx(14)都需要保护，没有函数调用的函数可不用保护lx寄存器
    // 被保留的寄存器主要有：
    //  (1) FP寄存器用于栈寻址，x29
    //  (2) LR寄存器用于函数调用，即x30。没有函数调用的函数可不用保护lx寄存器
    //  (3) R10寄存器用于立即数过大时要通过寄存器寻址，这里简化处理进行预留

    std::vector<int32_t> & protectedRegNo = func->getProtectedReg();
    if (func->getExistFuncCall()) {
        protectedRegNo.push_back(ARM64_LR_REG_NO);
    }

    // 1. 计算活跃区间
    std::vector<LiveRange> ranges = calculateLiveRanges(func);

    // 2. 按起始位置排序
    std::sort(ranges.begin(), ranges.end(), [](const LiveRange & a, const LiveRange & b) { return a.start < b.start || (a.start == b.start && a.end < b.end); });

    // 3. 分配寄存器
    linearScanRegisterAllocation(ranges, func);
    /*for (auto & rng : ranges) {
        int b; int64_t f;
        rng.value->getMemoryAddr(&b, &f);
        fprintf(stderr, "%s %d %d %d\n", rng.value->getName().c_str(), rng.reg, b, f);
    }*/

    // 4. 处理剩余逻辑（如保护寄存器）
    adjustFuncCallInsts(func);

    // 5. 对齐sp
    int32_t dep = func->getMaxDep();
    func->setMaxDep((dep + 15) & ~15);
    if (dep)
        protectedRegNo.push_back(ARM64_FP_REG_NO);
    // 6. 调整形参
    adjustFormalParamInsts(func);

#if 0
    // 临时输出调整后的IR指令，用于查看当前的寄存器分配、栈内变量分配、实参入栈等信息的正确性
    std::string irCodeStr;
    func->toString(irCodeStr);
    std::cout << irCodeStr << std::endl;
#endif
}

/// @brief 寄存器分配前对函数内的指令进行调整，以便方便寄存器分配
/// @param func 要处理的函数
/// @param usedArgs 调用其余函数时最大实参数
void CodeGeneratorArm64::adjustFormalParamInsts(Function * func)
{
    // 请注意这里所得的所有形参都是对应的实参的值关联的临时变量
    // 如果不是不能使用这里的代码
    auto & params = func->getParams();

    // 形参的前8个通过寄存器来传值R0-R7
    // 先排除需要函数调用的参数，这些参数需要重新分配寄存器
    int pm = func->getExistFuncCall() ? params.size() : 0;
    int k;
    std::vector<Instruction *> moves(std::min(8, pm));
    for (k = 0; k < std::min(8, pm); k++) {
        auto param = params[k];
        Type * type = param->getType();
        moves[k] = new MoveInstruction(func, param, PlatformArm64::regVal[type->isFloatType() ? k + ARM64_F0 : k]);
        int32_t reg;
        if (type->isArrayType() && (reg = params[k]->getRegId()) >= 0)
            params[k]->setMemoryAddr(reg, 0);
        simpleRegisterAllocator.Allocate(k);
    }
    auto & insts = func->getInterCode().getInsts();
    insts.insert(insts.begin() + 1, moves.begin(), moves.end());

    auto & protects = func->getProtectedReg();
    pm = params.size();

    for (int j = std::min(pm, 8); k < j; k++) {

        // 前8个设置分配寄存器
        FormalParam * param = params[k];
        int32_t reg = param->getRegId();
        if (ARM64_CALLER_SAVE(reg)) {
            std::remove(protects.begin(), protects.end(), reg);
            protects.pop_back();
        }
        param->setRegId(k + param->getType()->isFloatType() * ARM64_F0);
        simpleRegisterAllocator.Allocate(k);
    }

    // 根据ARM64版C语言的调用约定，除前8个外的实参进行值传递，逆序入栈
    for (; k < pm; k++) {
        auto param = params[k];
        // 目前假定变量大小都是8字节。
        int32_t reg = param->getRegId();
        if (ARM64_CALLER_SAVE(reg)) {
            std::remove(protects.begin(), protects.end(), reg);
            protects.pop_back();
        }
        param->setRegId(-1);
    }
    int32_t fp_esp = func->getMaxDep() + ((protects.size() + 1) & ~1) * 8;
    for (k = 8; k < pm; k++) {
        params[k]->setMemoryAddr(ARM64_SP_REG_NO, fp_esp);
        fp_esp += 8;
    }
}

/// @brief 寄存器分配前对函数内的指令进行调整，以便方便寄存器分配
/// @param func 要处理的函数
void CodeGeneratorArm64::adjustFuncCallInsts(Function * func)
{
    std::vector<Instruction *> newInsts;

    // 当前函数的指令列表
    auto & insts = func->getInterCode().getInsts();
    // auto & protects = func->getProtectedReg();

    // 函数返回值用R0寄存器，若函数调用有返回值，则赋值R0到对应寄存器
    for (auto pIter = insts.begin(); pIter != insts.end(); pIter++) {

        Instruction * inst = *pIter;
        int32_t opnum = inst->getOperandsNum();

        // 检查是否是函数调用指令，并且含有返回值
        if (Instanceof(callInst, FuncCallInstruction *, inst)) {

            //  实参前8个要寄存器传值，其它参数通过栈传递
            //  前8个的后面参数采用栈传递
            int esp = 0;
            for (int32_t k = 8; k < opnum; k++) {

                auto arg = callInst->getOperand(k);
                if (arg == callInst)
                    break;

                // 新建一个内存变量，用于栈传值到形参变量中
                LocalVariable * newVal = func->newLocalVarValue(arg->getType());
                newVal->setMemoryAddr(ARM64_SP_REG_NO, esp);
                esp += 8;

                Instruction * assignInst = new MoveInstruction(func, newVal, arg);

                callInst->setOperand(k, newVal);

                // 函数调用指令前插入后，pIter仍指向函数调用指令
                pIter = insts.insert(pIter, assignInst);
                pIter++;
            }
            auto piter = callInst->calledFunction->getParams().begin();
            for (int k = 0, l = std::min(opnum, 8); k < l; k++, piter++) {

                // 检查实参的类型是否是临时变量。
                // 如果是临时变量，该变量可更改为寄存器变量即可，或者设置寄存器号
                // 如果不是，则必须开辟一个寄存器变量，然后赋值即可
                auto arg = callInst->getOperand(k);
                if (arg == callInst)
                    break;

                int regno = k + (*piter)->getType()->isFloatType() * ARM64_F0;

                if (arg->getRegId() == regno) {
                    // 则说明寄存器已经是实参传递的寄存器，不用创建赋值指令
                    continue;
                } else {
                    // 创建临时变量，指定寄存器
                    Value * reg = PlatformArm64::regVal[regno];
                    Instruction * assignInst = new MoveInstruction(func, reg, arg);
                    callInst->setOperand(k, reg);

                    // 函数调用指令前插入后，pIter仍指向函数调用指令
                    pIter = insts.insert(pIter, assignInst);
                    pIter++;
                }
            }

            for (int k = 0; k < opnum; k++) {
                auto arg = callInst->getOperand(k);
                if (arg == callInst)
                    continue;

                // 再产生ARG指令
                pIter = insts.insert(pIter, new ArgInstruction(func, arg));
                pIter++;
            }

            // 有arg指令后可不用参数，展示不删除
            // args.clear();

            // 赋值指令
            if (callInst->hasResultValue()) {
                int regRet = callInst->getType()->isFloatType() * ARM64_F0;
                if (callInst->getRegId() == regRet) {
                    // 结果变量的寄存器和返回值寄存器一样，则什么都不需要做
                    ;
                } else {
                    // 其它情况，需要产生赋值指令
                    // 新建一个赋值操作
                    Instruction * assignInst = new MoveInstruction(func, callInst, PlatformArm64::regVal[regRet]);

                    // 函数调用指令的下一个指令的前面插入指令，因为有Exit指令，+1肯定有效
                    pIter = insts.insert(pIter + 1, assignInst);
                }
            }
        }
    }
}

const std::vector<LiveRange> & calculateLiveRanges(Function * func)
{
    auto ranges = new std::vector<LiveRange>();
    const auto & insts = func->getInterCode().getInsts();
    std::vector<std::pair<Instruction *, int>> labels;
    const bool hasFuncCall = func->getExistFuncCall();

    // 遍历指令，记录变量的定义和使用位置
    for (int pos = 0, l = insts.size(); pos < l; ++pos) {
        Instruction * inst = insts[pos];

        // 处理定义（赋值）
        if (inst->hasResultValue()) {
            LiveRange range;
            range.value = inst;
            range.start = pos;
            range.end = findLastUse(inst, insts, pos);
            ranges->push_back(range);
        } else if (inst->getOp() == IRINST_OP_LABEL) {
            // 记录Label的位置
            // TODO 外部声明无效
            labels.emplace_back(inst, pos);
        } else if (inst->getOp() == IRINST_OP_GOTO) {
            // 对于Goto指令，检查其前向跳转的范围
            int fromPos = pos;
            Instanceof(go, GotoInstruction *, inst);
            for (auto ed = labels.begin(); ed != labels.end(); ed++) {
                if (go->iftrue == ed->first || go->iffalse == ed->first) {
                    fromPos = ed->second;
                    break;
                }
            }
            if (fromPos < pos) {
                extendRange(*ranges, fromPos, pos);
            }
        }

        // 处理操作数（使用）
        for (int i = 0; i < inst->getOperandsNum(); ++i) {
            Value * operand = inst->getOperand(i);
            if (operand == inst)
                continue;
            if (dynamic_cast<Instruction *>(operand) || dynamic_cast<LocalVariable *>(operand) ||
                (hasFuncCall && dynamic_cast<FormalParam *>(operand))
                || (dynamic_cast<GlobalVariable *>(operand)
                    && (i!=0 || (inst->getOp() != IRINST_OP_ASSIGN && inst->getOp() != IRINST_OP_STORE)))) {
                extendRangeIfExists(*ranges, operand, pos);
            }
        }
    }
    return *ranges;
}

static inline bool usedByFunc0(Instruction * inst)
{
    int32_t opnum = inst->getOperandsNum();
    return inst->hasResultValue() && opnum && ({
               User * u = inst->getOperands()[opnum - 1]->getUser();
               dynamic_cast<FuncCallInstruction *>(u) && u->getOperandsNum() && u->getOperand(0) == inst;
           });
}

void CodeGeneratorArm64::linearScanRegisterAllocation(std::vector<LiveRange> & ranges, Function * func)
{
    // 使用被调用者保留寄存器
    std::vector<int32_t> freeRegs = {19, 20, 21, 22, 23, 24, 25, 26, 27, 28};
    for (int i : freeRegs) {
        simpleRegisterAllocator.Allocate(i);
    }
    /**
     * 参考https://learn.microsoft.com/zh-cn/cpp/build/arm64-windows-abi-conventions?view=msvc-170#floating-pointsimd-registers，
     * 对于浮点数，优先使用非易失寄存器(v8-v15低64位，即d8-v15), 保留v16和v17为临时寄存器，其余为易失可分配寄存器
     */
    std::vector<int32_t> floatRegs = {40, 41, 42, 43, 44, 45, 46, 47};
    if (!func->getExistFuncCall()) {
        freeRegs.insert(freeRegs.end(), {9, 10, 11, 12, 13, 14, 15});
        // v18-v31, 50-63
        floatRegs.insert(floatRegs.end(), {50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63});
    }
    std::vector<LiveRange> activei, activef;
    auto & protects = func->getProtectedReg();

    for (auto & range: ranges) {
        bool isFloat = range.value->getType()->isFloatType();
        auto & frees = isFloat ? floatRegs : freeRegs;
        auto & active = isFloat ? activef : activei;
        // 1. 过期已结束的区间
        expireOldRanges(active, frees, range.start);
        Instruction * inst;

        // 2. 溢出到栈
        if ((range.value->getType()->isArrayType() && dynamic_cast<LocalVariable *>(range.value)) || frees.empty()) {
            range.stackOffset = allocateStackSlot(func, range.value);
            continue;
        } else if ((inst = dynamic_cast<Instruction *>(range.value))) {
            int32_t opnum = inst->getOperandsNum();
            if (inst->hasResultValue() && opnum) {
                User * user = inst->getOperands()[opnum-1]->getUser();
                // 当该指令作为其他函数的第一个参数时，设置结果寄存器0
                if (dynamic_cast<FuncCallInstruction*>(user) && user->getOperandsNum()==1 && user->getOperand(0) == inst) {
                    range.reg = isFloat ? ARM64_F0 : 0;
                    continue;
                }
            }
        }
        // 3. 分配寄存器
        range.reg = frees.back();
        frees.pop_back();
        active.push_back(range);
    }

    // 4. 更新变量的寄存器或栈偏移
    for (const auto & range: ranges) {
        if (range.reg != -1) {
            if (dynamic_cast<GlobalVariable*>(range.value))
                range.value->setLoadRegId(0x80|range.reg);
            else
                range.value->setRegId(range.reg);
            simpleRegisterAllocator.Allocate(range.reg);
            if (ARM64_CALLER_SAVE(range.reg) &&
                std::find(protects.begin(), protects.end(), range.reg) == protects.end()) {
                protects.push_back(range.reg);
            }
        } else {
            range.value->setMemoryAddr(ARM64_FP_REG_NO, range.stackOffset);
        }
    }
}

// 查找变量的最后一次使用
int findLastUse(Value * val, const std::vector<Instruction *> & insts, int startPos)
{
    for (int i = insts.size() - 1; i >= startPos; --i) {
        for (int j = 0; j < insts[i]->getOperandsNum(); ++j) {
            if (insts[i]->getOperand(j) == val) {
                return i;
            }
        }
    }
    return startPos;
}

// 释放已结束的区间
void expireOldRanges(std::vector<LiveRange> & active, std::vector<int> & freeRegs, int pos)
{
    for (auto it = active.begin(); it != active.end();) {
        if (it->end <= pos) {
            freeRegs.push_back(it->reg);
            it = active.erase(it);
        } else {
            ++it;
        }
    }
}

// 分配栈槽
int allocateStackSlot(Function * func, Value * value)
{
    int offset = func->getMaxDep();
    Type * tp = value->getType();
    int sz;
    if ((tp->isArrayType() && dynamic_cast<Instruction *>(value)) || dynamic_cast<GlobalVariable*>(value)) {
        sz = 0;
    } else {
        sz = tp->getSize();
    }
    func->setMaxDep(offset + sz); // 假设4字节对齐
    return offset;
}

void extendRangeIfExists(std::vector<LiveRange> & ranges, Value * value, int currentPos)
{
    for (auto & range: ranges) {
        if (range.value == value) {
            // 扩展活跃范围到当前指令位置
            range.end = std::max(range.end, currentPos);
            return;
        }
    }
    // 如果未找到匹配的LiveRange，新的定义
    // Instanceof(cz, ConstInt*, value);
    if (!dynamic_cast<ConstInt *>(value)) {
        LiveRange lr;
        lr.value = value;
        lr.start = dynamic_cast<FormalParam *>(value) == nullptr ? currentPos : 0;
        if (dynamic_cast<GlobalVariable*>(value))
            lr.start --;
        lr.end = currentPos;
        ranges.push_back(lr);
    }
}

void extendRange(std::vector<LiveRange> & ranges, int fromPos, int pos)
{
    for (auto & i: ranges) {
        if (i.start < fromPos && fromPos < i.end && i.end < pos) {
            i.end = pos;
        }
    }
}
