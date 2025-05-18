///
/// @file InstSelectorArm64.cpp
/// @brief 指令选择器-ARM64的实现
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
#include <cstdio>

#include "Common.h"
#include "ILocArm64.h"
#include "InstSelectorArm64.h"
#include "PlatformArm64.h"

#include "PointerType.h"
#include "RegVariable.h"
#include "Function.h"

#include "LabelInstruction.h"
#include "GotoInstruction.h"
#include "FuncCallInstruction.h"
#include "MoveInstruction.h"
#include "BinaryInstruction.h"

static char *cmpmap[] = {"eq", "ne", "gt", "le", "ge", "lt"};
#define CSTRJ(C) cmpmap[(C-IRINST_OP_IEQ)^1]
#define CSTR(C) cmpmap[(C-IRINST_OP_IEQ)]

//static GotoInstruction *lastBranch;
/// @brief 构造函数
/// @param _irCode 指令
/// @param _iloc ILoc
/// @param _func 函数
InstSelectorArm64::InstSelectorArm64(vector<Instruction *> & _irCode,
                                     ILocArm64 & _iloc,
                                     Function * _func,
                                     SimpleRegisterAllocator & allocator)
    : ir(_irCode), iloc(_iloc), func(_func), simpleRegisterAllocator(allocator)
{
    translator_handlers[IRINST_OP_ENTRY] = &InstSelectorArm64::translate_entry;
    translator_handlers[IRINST_OP_EXIT] = &InstSelectorArm64::translate_exit;

    translator_handlers[IRINST_OP_LABEL] = &InstSelectorArm64::translate_label;
    translator_handlers[IRINST_OP_GOTO] = &InstSelectorArm64::translate_goto;

    translator_handlers[IRINST_OP_ASSIGN] = &InstSelectorArm64::translate_assign;

    translator_handlers[IROP(IADD)] = &InstSelectorArm64::translate_add_int32;
    translator_handlers[IROP(ISUB)] = &InstSelectorArm64::translate_sub_int32;
    translator_handlers[IROP(IMUL)] = &InstSelectorArm64::translate_mul_int32;
    translator_handlers[IROP(IDIV)] = &InstSelectorArm64::translate_div_int32;
    translator_handlers[IROP(IMOD)] = &InstSelectorArm64::translate_rem_int32;

    translator_handlers[IRINST_OP_FUNC_CALL] = &InstSelectorArm64::translate_call;
    translator_handlers[IRINST_OP_ARG] = &InstSelectorArm64::translate_arg;

    translator_handlers[IRINST_OP_IEQ] = &InstSelectorArm64::translate_bi_op;
    translator_handlers[IRINST_OP_INE] = &InstSelectorArm64::translate_bi_op;
    translator_handlers[IRINST_OP_IGT] = &InstSelectorArm64::translate_bi_op;
    translator_handlers[IRINST_OP_IGE] = &InstSelectorArm64::translate_bi_op;
    translator_handlers[IRINST_OP_ILT] = &InstSelectorArm64::translate_bi_op;
    translator_handlers[IRINST_OP_ILE] = &InstSelectorArm64::translate_bi_op;
}

///
/// @brief 析构函数
///
InstSelectorArm64::~InstSelectorArm64()
{}

/// @brief 指令选择执行
void InstSelectorArm64::run()
{
    for (auto inst: ir) {

        // 逐个指令进行翻译
        if (!inst->isDead()) {
            translate(inst);
        }
    }
}

/// @brief 指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate(Instruction * inst)
{
    // 操作符
    IRInstOperator op = inst->getOp();

    translate_handler pIter;
    if (op >= IRINST_OP_MAX || (pIter = translator_handlers[op]) == nullptr) {
        // 没有找到，则说明当前不支持
        printf("Translate: Operator(%d) not support", (int) op);
        return;
    }

    // 开启时输出IR指令作为注释
    if (showLinearIR) {
        outputIRInstruction(inst);
    }

    (this->*(pIter))(inst);
}

///
/// @brief 输出IR指令
///
void InstSelectorArm64::outputIRInstruction(Instruction * inst)
{
    std::string irStr;
    inst->toString(irStr);
    if (!irStr.empty()) {
        iloc.comment(irStr);
    }
}

/// @brief NOP翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_nop(Instruction * inst)
{
    (void) inst;
    iloc.nop();
}

/// @brief Label指令指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_label(Instruction * inst)
{
    Instanceof(labelInst, LabelInstruction *, inst);

    ArmInst *ai = iloc.getCode().back();
    if (ai->opcode[0]=='b' && ai->result == labelInst->getName())
        ai->setDead();
    iloc.label(labelInst->getName());
}

/// @brief goto指令指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_goto(Instruction * inst)
{
    Instanceof(gotoInst, GotoInstruction *, inst);
    Value *v = gotoInst->getCondiValue();

    if (v) {
        // 条件分支
        if (lstcmp != IRINST_OP_MAX) {
            iloc.branch(CSTR(lstcmp), gotoInst->iftrue->getName());
            iloc.branch(CSTRJ(lstcmp), gotoInst->iffalse->getName());
            lstcmp = IRINST_OP_MAX;
        } else {
            iloc.branch("eq", gotoInst->iffalse->getName());
            iloc.branch("ne", gotoInst->iftrue->getName());
        }
    } else
        // 无条件跳转
        iloc.jump(gotoInst->iftrue->getName());
    //lastBranch = gotoInst;
}

/// @brief 函数入口指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_entry(Instruction * inst)
{
    // 查看保护的寄存器
    auto & protectedReg = func->getProtectedReg();

    int i=0, m = protectedReg.size() - 1;
    while (i<m) {
        int32_t xa = protectedReg[i], xb = protectedReg[i+1];
        i += 2;
        iloc.inst("stp",
                  "x"+std::to_string(xa),
                  "x"+std::to_string(xb),
                  "[sp,#-16]!");
    }
    if (i<=m)
        iloc.inst("str","[sp,#-16]!");
    
    // 为fun分配栈帧，含局部变量、函数调用值传递的空间等
    iloc.allocStack(func, ARM64_TMP_REG_NO);
}

/// @brief 函数出口指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_exit(Instruction * inst)
{
    if (inst->getOperandsNum()) {
        // 存在返回值
        Value * retVal = inst->getOperand(0);

        // 赋值给寄存器R0
        iloc.load_var(0, retVal);
    }

    // 恢复栈空间
    iloc.inst("add", "sp", "sp", iloc.toStr(func->getMaxDep()));

    auto & protectedReg = func->getProtectedReg();
    if (!protectedReg.empty()) {
        int i=0, m = protectedReg.size() - 1;
        while (i<m) {
            int32_t xa = protectedReg[i], xb = protectedReg[i+1];
            i += 2;
            iloc.inst("ldp",
                      "x"+std::to_string(xa),
                      "x"+std::to_string(xb),
                      "[sp], 16");
        }
        if (i<=m)
            iloc.inst("ldr", "x"+std::to_string(protectedReg[i]),
                      "[sp], 16");
    }

    iloc.inst("ret","");
}

/// @brief 赋值指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_assign(Instruction * inst)
{
    Value * result = inst->getOperand(0);
    Value * arg1 = inst->getOperand(1);

    int32_t arg1_regId = arg1->getRegId();
    int32_t result_regId = result->getRegId();

    if (arg1_regId != -1) {
        // 寄存器 => 内存
        // 寄存器 => 寄存器

        // r8 -> rs 可能用到r9
        iloc.store_var(arg1_regId, result, ARM64_TMP_REG_NO);
    } else if (result_regId != -1) {
        // 内存变量 => 寄存器

        iloc.load_var(result_regId, arg1);
    } else {
        // 内存变量 => 内存变量

        int32_t temp_regno = simpleRegisterAllocator.Allocate();

        // arg1 -> r8
        iloc.load_var(temp_regno, arg1);

        // r8 -> rs 可能用到r9
        iloc.store_var(temp_regno, result, ARM64_TMP_REG_NO);

        simpleRegisterAllocator.free(temp_regno);
    }
}

/// @brief 二元操作指令翻译成ARM64汇编
/// @param inst IR指令
/// @param operator_name 操作码
/// @param rs_reg_no 结果寄存器号
/// @param op1_reg_no 源操作数1寄存器号
/// @param op2_reg_no 源操作数2寄存器号
void InstSelectorArm64::translate_two_operator(Instruction * inst, string operator_name)
{
    Value * result = inst;
    Value * arg1 = inst->getOperand(0);
    Value * arg2 = inst->getOperand(1);

    int32_t arg1_reg_no = arg1->getRegId();
    int32_t arg2_reg_no = arg2->getRegId();
    int32_t result_reg_no = inst->getRegId();
    int32_t load_result_reg_no, load_arg1_reg_no, load_arg2_reg_no;

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {

        // 分配一个寄存器r8
        load_arg1_reg_no = simpleRegisterAllocator.Allocate(arg1);

        // arg1 -> r8，这里可能由于偏移不满足指令的要求，需要额外分配寄存器
        iloc.load_var(load_arg1_reg_no, arg1);
    } else {
        load_arg1_reg_no = arg1_reg_no;
    }

    // 看arg2是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg2_reg_no == -1) {

        // 分配一个寄存器r9
        load_arg2_reg_no = simpleRegisterAllocator.Allocate(arg2);

        // arg2 -> r9
        iloc.load_var(load_arg2_reg_no, arg2);
    } else {
        load_arg2_reg_no = arg2_reg_no;
    }

    // 看结果变量是否是寄存器，若不是则需要分配一个新的寄存器来保存运算的结果
    if (result_reg_no == -1) {
        // 分配一个寄存器r10，用于暂存结果
        load_result_reg_no = simpleRegisterAllocator.Allocate(result);
    } else {
        load_result_reg_no = result_reg_no;
    }

    // r8 + r9 -> r10
    iloc.inst(operator_name,
              PlatformArm64::regName[load_result_reg_no],
              PlatformArm64::regName[load_arg1_reg_no],
              PlatformArm64::regName[load_arg2_reg_no]);

    // 结果不是寄存器，则需要把rs_reg_name保存到结果变量中
    if (result_reg_no == -1) {

        // 这里使用预留的临时寄存器，因为立即数可能过大，必须借助寄存器才可操作。

        // r10 -> result
        iloc.store_var(load_result_reg_no, result, ARM64_TMP_REG_NO);
    }

    // 释放寄存器
    simpleRegisterAllocator.free(arg1);
    simpleRegisterAllocator.free(arg2);
    simpleRegisterAllocator.free(result);
}

/// @brief 整数加法指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_add_int32(Instruction * inst)
{
    translate_two_operator(inst, "add");
}

/// @brief 整数减法指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_sub_int32(Instruction * inst)
{
    translate_two_operator(inst, "sub");
}

void InstSelectorArm64::translate_mul_int32(Instruction * inst) {
    translate_two_operator(inst, "mul");
}

void InstSelectorArm64::translate_div_int32(Instruction * inst) {
    translate_two_operator(inst, "sdiv");
}

void InstSelectorArm64::translate_rem_int32(Instruction * inst) {
    translate_two_operator(inst, "rem");
}

void InstSelectorArm64::translate_bi_op(Instruction *inst) {
    //const char *op;
    switch (inst->getOp()) {
        case IRINST_OP_IEQ:
        case IRINST_OP_INE:
        case IRINST_OP_IGT:
        case IRINST_OP_IGE:
        case IRINST_OP_ILT:
        case IRINST_OP_ILE:
            lstcmp = inst->getOp();
            translate_two_operator(inst, "subs");
            return;
        default: return;
    }
}

/// @brief 函数调用指令翻译成ARM64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_call(Instruction * inst)
{
    FuncCallInstruction * callInst = dynamic_cast<FuncCallInstruction *>(inst);

    int32_t operandNum = callInst->getOperandsNum();

    if (operandNum != realArgCount) {

        // 两者不一致 也可能没有ARG指令，正常
        if (realArgCount != 0) {

            minic_log(LOG_ERROR, "ARG指令的个数与调用函数个数不一致");
        }
    }

    if (operandNum) {

        // 强制占用这几个寄存器参数传递的寄存器
        simpleRegisterAllocator.Allocate(0);
        simpleRegisterAllocator.Allocate(1);
        simpleRegisterAllocator.Allocate(2);
        simpleRegisterAllocator.Allocate(3);

        // 前四个的后面参数采用栈传递
        int esp = 0;
        for (int32_t k = 4; k < operandNum; k++) {

            auto arg = callInst->getOperand(k);

            // 新建一个内存变量，用于栈传值到形参变量中
            MemVariable * newVal = func->newMemVariable((Type *) PointerType::get(arg->getType()));
            newVal->setMemoryAddr(ARM64_SP_REG_NO, esp);
            esp += 4;

            Instruction * assignInst = new MoveInstruction(func, newVal, arg);

            // 翻译赋值指令
            translate_assign(assignInst);

            delete assignInst;
        }

        for (int32_t k = 0; k < operandNum && k < 4; k++) {

            auto arg = callInst->getOperand(k);

            // 检查实参的类型是否是临时变量。
            // 如果是临时变量，该变量可更改为寄存器变量即可，或者设置寄存器号
            // 如果不是，则必须开辟一个寄存器变量，然后赋值即可

            Instruction * assignInst = new MoveInstruction(func, PlatformArm64::intRegVal[k], arg);

            // 翻译赋值指令
            translate_assign(assignInst);

            delete assignInst;
        }
    }

    iloc.call_fun(callInst->getName());

    if (operandNum) {
        simpleRegisterAllocator.free(0);
        simpleRegisterAllocator.free(1);
        simpleRegisterAllocator.free(2);
        simpleRegisterAllocator.free(3);
    }

    // 赋值指令
    if (callInst->hasResultValue()) {

        // 新建一个赋值操作
        Instruction * assignInst = new MoveInstruction(func, callInst, PlatformArm64::intRegVal[0]);

        // 翻译赋值指令
        translate_assign(assignInst);

        delete assignInst;
    }

    // 函数调用后清零，使得下次可正常统计
    realArgCount = 0;
}

///
/// @brief 实参指令翻译成ARM64汇编
/// @param inst
///
void InstSelectorArm64::translate_arg(Instruction * inst)
{
    // 翻译之前必须确保源操作数要么是寄存器，要么是内存，否则出错。
    Value * src = inst->getOperand(0);

    // 当前统计的ARG指令个数
    int32_t regId = src->getRegId();

    if (realArgCount < 4) {
        // 前四个参数
        if (regId != -1) {
            if (regId != realArgCount) {
                // 肯定寄存器分配有误
                minic_log(LOG_ERROR, "第%d个ARG指令对象寄存器分配有误: %d", argCount + 1, regId);
            }
        } else {
            minic_log(LOG_ERROR, "第%d个ARG指令对象不是寄存器", argCount + 1);
        }
    } else {
        // 必须是内存分配，若不是则出错
        int32_t baseRegId;
        bool result = src->getMemoryAddr(&baseRegId);
        if ((!result) || (baseRegId != ARM64_SP_REG_NO)) {

            minic_log(LOG_ERROR, "第%d个ARG指令对象不是SP寄存器寻址", argCount + 1);
        }
    }

    realArgCount++;
}
