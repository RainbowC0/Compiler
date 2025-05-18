///
/// @file ILocArm64.cpp
/// @brief 指令序列管理的实现，ILOC的全称为Intermediate Language for Optimizing Compilers
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
#include <string>

#include "ILocArm64.h"
#include "Common.h"
#include "Function.h"
#include "PlatformArm64.h"
#include "Module.h"

#define emit(...) code.push_back(new ArmInst(__VA_ARGS__))

/// @brief 构造函数
/// @param _module 符号表
ILocArm64::ILocArm64(Module * _module)
{
    this->module = _module;
}

/// @brief 析构函数
ILocArm64::~ILocArm64()
{
    std::list<ArmInst *>::iterator pIter;

    for (pIter = code.begin(); pIter != code.end(); ++pIter) {
        delete (*pIter);
    }
}

/// @brief 删除无用的Label指令
void ILocArm64::deleteUsedLabel()
{
    std::list<ArmInst *> labelInsts;
    for (ArmInst * arm: code) {
        if ((!arm->dead) && (arm->opcode[0] == '.') && (arm->result == ":")) {
            labelInsts.push_back(arm);
        }
    }

    for (ArmInst * labelArm: labelInsts) {
        bool labelUsed = false;

        for (ArmInst * arm: code) {
            // TODO 转移语句的指令标识符根据定义修改判断
            if ((!arm->dead) && (arm->opcode[0] == 'b') && (arm->result == labelArm->opcode)) {
                labelUsed = true;
                break;
            }
        }

        if (!labelUsed) {
            labelArm->setDead();
        }
    }
}

/// @brief 输出汇编
/// @param file 输出的文件指针
/// @param outputEmpty 是否输出空语句
void ILocArm64::outPut(FILE * file, bool outputEmpty)
{
    for (auto arm: code) {

        std::string s = arm->outPut();

        if (arm->result == ":") {
            // Label指令，不需要Tab输出
            fprintf(file, "%s\n", s.c_str());
            continue;
        }

        if (!s.empty()) {
            fprintf(file, "\t%s\n", s.c_str());
        } else if ((outputEmpty)) {
            fprintf(file, "\n");
        }
    }
}

/// @brief 获取当前的代码序列
/// @return 代码序列
std::list<ArmInst *> & ILocArm64::getCode()
{
    return code;
}

/**
 * 数字变字符串，若flag为真，则变为立即数寻址（加#）
 */
std::string ILocArm64::toStr(int num, bool flag)
{
    std::string ret;

    if (flag) {
        ret = "#";
    }

    ret += std::to_string(num);

    return ret;
}

/*
    产生标签
*/
void ILocArm64::label(const std::string &name)
{
    // .L1:
    emit(name, ":");
}

/// @brief 0个源操作数指令
/// @param op 操作码
/// @param rs 操作数
void ILocArm64::inst(const std::string &op, const std::string &rs)
{
    emit(op, rs);
}

/// @brief 一个操作数指令
/// @param op 操作码
/// @param rs 操作数
/// @param arg1 源操作数
void ILocArm64::inst(
    const std::string &op,
    const std::string &rs,
    const std::string &arg1)
{
    emit(op, rs, arg1);
}

/// @brief 一个操作数指令
/// @param op 操作码
/// @param rs 操作数
/// @param arg1 源操作数
/// @param arg2 源操作数
void ILocArm64::inst(const std::string &op,
    const std::string &rs,
    const std::string &arg1,
    const std::string &arg2)
{
    emit(op, rs, arg1, arg2);
}

///
/// @brief 注释指令，不包含分号
///
void ILocArm64::comment(const std::string &str)
{
    emit("@", str);
}

/*
    加载立即数 ldr r0,=#100
*/
union _X {
    int32_t val;
    struct {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint16_t low, high;
    #else
    uint16_t high, low;
    #endif
    };
};
void ILocArm64::load_imm(int rs_reg_no, int constant)
{
    union _X v;
    v.val = constant;
    emit("mov", PlatformArm64::regName[rs_reg_no], "#"+std::to_string(v.low));
    if (v.high)
        emit("movk", PlatformArm64::regName[rs_reg_no], "#"+std::to_string(v.high), "lsl #16");
}

#define xregs(i) ("x"+std::to_string(i))
/// @brief 加载符号值 ldr r0,=g ldr r0,=.L1
/// @param rs_reg_no 结果寄存器编号
/// @param name 符号名
void ILocArm64::load_symbol(int rs_reg_no, std::string name)
{
    // movw r10, #:lower16:a
    // movt r10, #:upper16:a
    //emit("movw", PlatformArm64::regName[rs_reg_no], "#:lower16:" + name);
    //emit("movt", PlatformArm64::regName[rs_reg_no], "#:upper16:" + name);
    // adrp x0, a
    // ldr w0, [x0, :loc12:a]
    std::string x = xregs(rs_reg_no);
    emit("adrp", x, name);
    std::string adr = "[";
    adr += x;
    adr += ",:lo12:";
    adr += name;
    adr += "]";
    emit("ldr", PlatformArm64::regName[rs_reg_no], adr);
}

/// @brief 基址寻址 ldr r0,[fp,#100]
/// @param rsReg 结果寄存器
/// @param base_reg_no 基址寄存器
/// @param offset 偏移
void ILocArm64::load_base(int rs_reg_no, int base_reg_no, int offset)
{
    std::string rsReg = PlatformArm64::regName[rs_reg_no];
    std::string base = PlatformArm64::regName[base_reg_no];
    if (base[0]=='w') base.data()[0] = 'x';

    if (PlatformArm64::isDisp(offset)) {
        // 有效的偏移常量
        if (offset) {
            // [fp,#-16] [fp]
            base += "," + toStr(offset);
        }
    } else {

        // ldr r8,=-4096
        load_imm(rs_reg_no, offset);

        // fp,r8
        base += "," + rsReg;
    }

    // 内存寻址
    base = "[" + base + "]";

    // ldr r8,[fp,#-16]
    // ldr r8,[fp,r8]
    emit("ldr", rsReg, base);
}

/// @brief 基址寻址 str r0,[fp,#100]
/// @param srcReg 源寄存器
/// @param base_reg_no 基址寄存器
/// @param disp 偏移
/// @param tmp_reg_no 可能需要临时寄存器编号
void ILocArm64::store_base(int src_reg_no, int base_reg_no, int disp, int tmp_reg_no)
{
    std::string base = PlatformArm64::regName[base_reg_no];
    if (base[0]=='w') base.data()[0] = 'x';

    if (PlatformArm64::isDisp(disp)) {
        // 有效的偏移常量

        // 若disp为0，则直接采用基址，否则采用基址+偏移
        // [fp,#-16] [fp]
        if (disp) {
            base += "," + toStr(disp);
        }
    } else {
        // 先把立即数赋值给指定的寄存器tmpReg，然后采用基址+寄存器的方式进行

        // ldr r9,=-4096
        load_imm(tmp_reg_no, disp);

        // fp,r9
        base += "," + PlatformArm64::regName[tmp_reg_no];
    }

    // 内存间接寻址
    base = "[" + base + "]";

    // str r8,[fp,#-16]
    // str r8,[fp,r9]
    emit("str", PlatformArm64::regName[src_reg_no], base);
}

/// @brief 寄存器Mov操作
/// @param rs_reg_no 结果寄存器
/// @param src_reg_no 源寄存器
void ILocArm64::mov_reg(int rs_reg_no, int src_reg_no)
{
    emit("mov", PlatformArm64::regName[rs_reg_no], PlatformArm64::regName[src_reg_no]);
}

/// @brief 加载变量到寄存器，保证将变量放到reg中
/// @param rs_reg_no 结果寄存器
/// @param src_var 源操作数
void ILocArm64::load_var(int rs_reg_no, Value * src_var)
{

    if (Instanceof(constVal, ConstInt *, src_var)) {
        // 整型常量

        // TODO 目前只考虑整数类型 100
        // ldr r8,#100
        load_imm(rs_reg_no, constVal->getVal());
    } else if (src_var->getRegId() != -1) {

        // 源操作数为寄存器变量
        int32_t src_regId = src_var->getRegId();

        if (src_regId != rs_reg_no) {

            // mov r8,r2 | 这里有优化空间——消除r8
            emit("mov", PlatformArm64::regName[rs_reg_no], PlatformArm64::regName[src_regId]);
        }
    } else if (Instanceof(globalVar, GlobalVariable *, src_var)) {
        // 全局变量

        // 读取全局变量的地址
        // movw r8, #:lower16:a
        // movt r8, #:lower16:a
        load_symbol(rs_reg_no, globalVar->getName());

        // ldr r8, [r8]
        //emit("ldr", PlatformArm64::regName[rs_reg_no], "[" + PlatformArm64::regName[rs_reg_no] + "]");

    } else {

        // 栈+偏移的寻址方式

        // 栈帧偏移
        int32_t var_baseRegId = -1;
        int64_t var_offset = -1;

        bool result = src_var->getMemoryAddr(&var_baseRegId, &var_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }

        // 对于栈内分配的局部数组，可直接在栈指针上进行移动与运算
        // 但对于形参，其保存的是调用函数栈的数组的地址，需要读取出来

        // ldr r8,[sp,#16]
        load_base(rs_reg_no, var_baseRegId, var_offset);
    }
}

/// @brief 加载变量地址到寄存器
/// @param rs_reg_no
/// @param var
/*
void ILocArm64::lea_var(int rs_reg_no, Value * var)
{
    // 被加载的变量肯定不是常量！
    // 被加载的变量肯定不是寄存器变量！

    // 目前只考虑局部变量

    // 栈帧偏移
    int32_t var_baseRegId = -1;
    int64_t var_offset = -1;

    bool result = var->getMemoryAddr(&var_baseRegId, &var_offset);
    if (!result) {
        minic_log(LOG_ERROR, "BUG");
    }

    // lea r8, [fp,#-16]
    leaStack(rs_reg_no, var_baseRegId, var_offset);
}
*/
/// @brief 保存寄存器到变量，保证将计算结果（r8）保存到变量
/// @param src_reg_no 源寄存器
/// @param dest_var  变量
/// @param tmp_reg_no 第三方寄存器
void ILocArm64::store_var(int src_reg_no, Value * dest_var, int tmp_reg_no)
{
    // 被保存目标变量肯定不是常量

    if (dest_var->getRegId() != -1) {

        // 寄存器变量

        // -1表示非寄存器，其他表示寄存器的索引值
        int dest_reg_id = dest_var->getRegId();

        // 寄存器不一样才需要mov操作
        if (src_reg_no != dest_reg_id) {

            // mov r2,r8 | 这里有优化空间——消除r8
            emit("mov", PlatformArm64::regName[dest_reg_id], PlatformArm64::regName[src_reg_no]);
        }

    } else if (Instanceof(globalVar, GlobalVariable *, dest_var)) {
        // 全局变量

        // 读取符号的地址到寄存器x10
        //load_symbol(tmp_reg_no, globalVar->getName());
        std::string x = xregs(tmp_reg_no);
        emit("adrp", x, globalVar->getName());
        // str r8, [x10, :lo12:a]
        emit("str", PlatformArm64::regName[src_reg_no], "[" + x + ",:lo12:"+globalVar->getName()+"]");

    } else {

        // 对于局部变量，则直接从栈基址+偏移寻址

        // TODO 目前只考虑局部变量

        // 栈帧偏移
        int32_t dest_baseRegId = -1;
        int64_t dest_offset = -1;

        bool result = dest_var->getMemoryAddr(&dest_baseRegId, &dest_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }

        // str r8,[r9]
        // str r8, [fp, # - 16]
        store_base(src_reg_no, dest_baseRegId, dest_offset, tmp_reg_no);
    }
}

/// @brief 加载栈内变量地址
/// @param rsReg 结果寄存器号
/// @param base_reg_no 基址寄存器
/// @param off 偏移
void ILocArm64::leaStack(int rs_reg_no, int base_reg_no, int off)
{
    std::string rs_reg_name = xregs(rs_reg_no);
    std::string base_reg_name = xregs(base_reg_no);

    if (PlatformArm64::constExpr(off))
        // add r8,fp,#-16
        emit("add", rs_reg_name, base_reg_name, toStr(off));
    else {
        // ldr r8,=-257
        load_imm(rs_reg_no, off);

        // add r8,fp,r8
        emit("add", rs_reg_name, base_reg_name, rs_reg_name);
    }
}

/// @brief 函数内栈内空间分配（局部变量、形参变量、函数参数传值，或不能寄存器分配的临时变量等）
/// @param func 函数
/// @param tmp_reg_No
void ILocArm64::allocStack(Function * func, int tmp_reg_no)
{
    // 超过四个的函数调用参数个数，多余4个，则需要栈传值
    int funcCallArgCnt = func->getMaxFuncCallArgCnt() - 8;
    if (funcCallArgCnt < 0) {
        funcCallArgCnt = 0;
    }

    // 计算栈帧大小
    int off = func->getMaxDep();

    off += funcCallArgCnt * 4;

    // 不需要在栈内额外分配空间，则什么都不做
    if (0 == off)
        return;

    if (PlatformArm64::constExpr(off)) {
        // sub sp,sp,#16
        emit("sub", "sp", "sp", toStr(off));
    } else {
        // ldr r8,=257
        load_imm(tmp_reg_no, off);

        // sub sp,sp,r8
        emit("sub", "sp", "sp", PlatformArm64::regName[tmp_reg_no]);
    }

    // 函数调用通过栈传递的基址寄存器设置
    inst("add", ARM64_FP, "sp", toStr(funcCallArgCnt * 4));
}

/// @brief 调用函数fun
/// @param fun
void ILocArm64::call_fun(const std::string &name)
{
    // 函数返回值在r0,不需要保护
    emit("bl", name);
}

/// @brief NOP操作
void ILocArm64::nop()
{
    // FIXME 无操作符，要确认是否用nop指令
    emit("");
}

///
/// @brief 无条件跳转指令
/// @param label 目标Label名称
///
void ILocArm64::jump(const std::string &label)
{
    emit("b", label);
}

void ILocArm64::branch(const std::string &cond, const std::string &label) {
    emit("b"+cond, label);
}
