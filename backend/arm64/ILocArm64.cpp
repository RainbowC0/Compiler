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

#define ARRTYPE(v)                                                                                                     \
    ({                                                                                                                 \
        Type * _tp = v->getType();                                                                                     \
        _tp && _tp->isArrayType();                                                                                     \
    })

ArmInst::ArmInst(cstr _opcode, cstr _result, cstr _arg1, cstr _arg2, cstr _cond, cstr _addition)
    : opcode(_opcode), cond(_cond), result(_result), arg1(_arg1), arg2(_arg2), addition(_addition), dead(false)
{}

/*
指令内容替换
*/
void ArmInst::replace(cstr _opcode, cstr _result, cstr _arg1, cstr _arg2, cstr _cond, cstr _addition)
{
    opcode = _opcode;
    result = _result;
    arg1 = _arg1;
    arg2 = _arg2;
    cond = _cond;
    addition = _addition;

#if 0
// 空操作，则设置为dead
if (op == "") {
dead = true;
}
#endif
}

/*
设置为无效指令
*/
void ArmInst::setDead()
{
    dead = true;
}

/*
输出函数
*/
std::string ArmInst::outPut()
{
    // 无用代码，什么都不输出
    if (dead) {
        return "";
    }

    // 占位指令,可能需要输出一个空操作，看是否支持 FIXME
    if (opcode.empty()) {
        return "";
    }

    std::string ret = opcode;

    if (!cond.empty()) {
        ret += cond;
    }

    // 结果输出
    if (!result.empty()) {
        if (result == ":") {
            ret += result;
        } else {
            ret += " " + result;
        }
    }

    // 第一元参数输出
    if (!arg1.empty()) {
        ret += "," + arg1;
    }

    // 第二元参数输出
    if (!arg2.empty()) {
        ret += "," + arg2;
    }

    // 其他附加信息输出
    if (!addition.empty()) {
        ret += "," + addition;
    }

    return ret;
}

/// @brief 构造函数
/// @param _module 符号表
ILocArm64::ILocArm64(Module * _module)
{
    this->module = _module;
}

/// @brief 析构函数
ILocArm64::~ILocArm64()
{
    for (auto pIter: code) {
        delete pIter;
    }
}

/// @brief 删除无用的Label和Goto指令
void ILocArm64::deleteUsed()
{
    ArmInsts labelInsts;
    bool reachable = true;
    // ArmInst *arm;
    for (auto arm: code) {
        if (!arm->dead) {
            if ((arm->opcode[0] == '.') && (arm->result == ":")) {
                labelInsts.push_back(arm);
                reachable = true;
            } else if (!reachable) {
                arm->setDead();
            } else if (arm->opcode == "b") {
                reachable = false;
            }
        }
    }

    for (ArmInst * labelArm: labelInsts) {
        bool labelUsed = false;

        for (auto arm: code) {
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
ArmInsts & ILocArm64::getCode()
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
void ILocArm64::label(cstr name)
{
    // .L1:
    emit(name, ":");
}

/// @brief 0个源操作数指令
/// @param op 操作码
/// @param rs 操作数
void ILocArm64::inst(cstr op, cstr rs)
{
    emit(op, rs);
}

/// @brief 一个操作数指令
/// @param op 操作码
/// @param rs 操作数
/// @param arg1 源操作数
void ILocArm64::inst(cstr op, cstr rs, cstr arg1)
{
    emit(op, rs, arg1);
}

/// @brief 一个操作数指令
/// @param op 操作码
/// @param rs 操作数
/// @param arg1 源操作数
/// @param arg2 源操作数
void ILocArm64::inst(cstr op, cstr rs, cstr arg1, cstr arg2)
{
    emit(op, rs, arg1, arg2);
}

///
/// @brief 注释指令，不包含分号
///
void ILocArm64::comment(cstr str)
{
    emit("@", str);
}

/*
    加载立即数 ldr r0,=#100
*/

void ILocArm64::load_imm(int rs_reg_no, int constant, bool wide)
{
    union {
        int32_t val;
        struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            uint16_t low, high;
#else
            uint16_t high, low;
#endif
        };
    } z, n;
    cstr reg = (wide ? PlatformArm64::xregName : PlatformArm64::regName)[rs_reg_no];
    if (constant == 0) {
        emit("mov", reg, "wzr");
        return;
    }
    z.val = constant;
    n.val = ~constant;
    if (z.high && z.low && n.high && n.low) {
        emit("mov", reg, "#" + std::to_string(z.low));
        emit("movk", reg, "#" + std::to_string(z.high), "lsl #16");
    } else
        emit("mov", reg, "#" + std::to_string(constant));
}

void ILocArm64::load_fimm(int rs_reg, float f, bool wide)
{
    union {
        float f;
        int i;
    } num;
    num.f = f;
    load_imm(ARM64_TMP_REG_NO, num.i, wide);
    emit("fmov", PlatformArm64::regName[rs_reg], PlatformArm64::regName[ARM64_TMP_REG_NO]);
}

#define xregs(i) PlatformArm64::xregName[i]
/// @brief 加载符号值 ldr r0,=g ldr r0,=.L1
/// @param rs_reg_no 结果寄存器编号
/// @param name 符号名
void ILocArm64::load_symbol(int rs_reg_no, cstr name)
{
    // movw r10, #:lower16:a
    // movt r10, #:upper16:a
    // emit("movw", PlatformArm64::regName[rs_reg_no], "#:lower16:" + name);
    // emit("movt", PlatformArm64::regName[rs_reg_no], "#:upper16:" + name);
    // adrp x0, a
    // ldr w0, [x0, :loc12:a]

    std::string x = xregs(rs_reg_no >= 32 ? ARM64_TMP_REG_NO : rs_reg_no);
    emit("adrp", x, name);
    std::string adr = "[";
    adr += x;
    adr += ",:lo12:";
    adr += name;
    adr += "]";
    emit("ldr", rs_reg_no == ARM64_ZR_REG_NO ? "sp" : PlatformArm64::regName[rs_reg_no], adr);
}

/// @brief 基址寻址 ldr r0,[fp,#100]
/// @param rsReg 结果寄存器
/// @param base_reg_no 基址寄存器
/// @param offset 偏移
void ILocArm64::load_base(int rs_reg_no, int base_reg_no, int offset, bool wide)
{
    auto * regs = wide ? PlatformArm64::xregName : PlatformArm64::regName;
    std::string rsReg = regs[rs_reg_no];
    std::string base = base_reg_no == ARM64_ZR_REG_NO ? "sp" : PlatformArm64::xregName[base_reg_no];

    if (PlatformArm64::isDisp(offset)) {
        // 有效的偏移常量
        if (offset) {
            // [fp,#-16] [fp]
            base += "," + toStr(offset);
        }
    } else {

        // ldr r8,=-4096
        load_imm(ARM64_TMP_REG_NO2, offset);

        // fp,r8
        base += "," + PlatformArm64::xregName[ARM64_TMP_REG_NO2];
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
void ILocArm64::store_base(int src_reg_no, int base_reg_no, int disp, int tmp_reg_no, bool wide)
{
    std::string base = base_reg_no == ARM64_ZR_REG_NO ? "sp" : PlatformArm64::xregName[base_reg_no];

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
        base += "," + PlatformArm64::regName[tmp_reg_no] + ",sxtw";
    }

    // 内存间接寻址
    base = "[" + base + "]";

    // str r8,[fp,#-16]
    // str r8,[fp,r9]
    emit("str", (wide ? PlatformArm64::xregName : PlatformArm64::regName)[src_reg_no], base);
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
void ILocArm64::load_var(int rs_reg_no, Value * src_var, bool wide)
{

    if (Instanceof(constVal, ConstInt *, src_var)) {
        // 整型常量

        // TODO 目前只考虑整数类型 100
        // ldr r8,#100
        load_imm(rs_reg_no, constVal->getVal(), wide);
    } else if (Instanceof(constFloat, ConstFloat *, src_var)) {
        load_fimm(rs_reg_no, constFloat->getVal(), wide);
    } else if (src_var->getRegId() != -1) {

        // 源操作数为寄存器变量
        int32_t src_regId = src_var->getRegId();

        if (src_regId != rs_reg_no) {
            auto * regs = wide ? PlatformArm64::xregName : PlatformArm64::regName;
            // mov r8,r2 | 这里有优化空间——消除r8
            cstr op = rs_reg_no >= ARM64_F0 ? "fmov" : "mov";
            emit(op, regs[rs_reg_no], regs[src_regId]);
        }
    } else if (Instanceof(globalVar, GlobalVariable *, src_var)) {
        // 全局变量

        // 读取全局变量的地址
        // movw r8, #:lower16:a
        // movt r8, #:lower16:a
        load_symbol(rs_reg_no, globalVar->getName());

        // ldr r8, [r8]
        // emit("ldr", PlatformArm64::regName[rs_reg_no], "[" + PlatformArm64::regName[rs_reg_no] + "]");

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
        load_base(rs_reg_no, var_baseRegId, var_offset, wide);
    }
}

/// @brief 加载变量地址到寄存器
/// @param rs_reg_no
/// @param var

void ILocArm64::lea_var(int rs_reg_no, Value * var)
{
    // 被加载的变量肯定不是常量！
    // 被加载的变量肯定不是寄存器变量！

    // 目前只考虑局部变量

    // 栈帧偏移
    int32_t var_baseRegId = -1;
    int64_t var_offset = -1;

    if (dynamic_cast<GlobalVariable *>(var)) {
        cstr xreg = xregs(rs_reg_no);
        emit("adrp", xreg, var->getName());
        emit("add", xreg, xreg, ":lo12:" + var->getName());
    } else {
        bool result = var->getMemoryAddr(&var_baseRegId, &var_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }
        // lea r8, [fp,#-16]
        leaStack(rs_reg_no, var_baseRegId, var_offset);
    }
}

/// @brief 保存寄存器到变量，保证将计算结果（r8）保存到变量
/// @param src_reg_no 源寄存器
/// @param dest_var  变量
/// @param tmp_reg_no 第三方寄存器
void ILocArm64::store_var(int src_reg_no, Value * dest_var, int tmp_reg_no, bool wide)
{
    // 被保存目标变量肯定不是常量

    if (dest_var->getRegId() != -1) {

        // 寄存器变量

        // -1表示非寄存器，其他表示寄存器的索引值
        int dest_reg_id = dest_var->getRegId();

        // 寄存器不一样才需要mov操作
        if (src_reg_no != dest_reg_id) {
            cstr op = dest_reg_id >= ARM64_F0 ? "fmov" : "mov";
            if (wide || ARRTYPE(dest_var))
                emit(op, xregs(dest_reg_id), xregs(src_reg_no));
            else

                // mov r2,r8 | 这里有优化空间——消除r8
                emit(op, PlatformArm64::regName[dest_reg_id], PlatformArm64::regName[src_reg_no]);
        }

    } else if (Instanceof(globalVar, GlobalVariable *, dest_var)) {
        // 全局变量

        // 读取符号的地址到寄存器x10
        // load_symbol(tmp_reg_no, globalVar->getName());
        std::string x = tmp_reg_no == ARM64_ZR_REG_NO ? "sp" : xregs(tmp_reg_no);
        emit("adrp", x, globalVar->getName());
        // str r8, [x10, :lo12:a]
        emit("str",
             (wide ? PlatformArm64::xregName : PlatformArm64::regName)[src_reg_no],
             "[" + x + ",:lo12:" + globalVar->getName() + "]");

    } else {

        // 对于局部变量，则直接从栈基址+偏移寻址

        // TODO 目前只考虑局部变量

        // 栈帧偏移
        int32_t dest_baseRegId = -1;
        int64_t dest_offset = -1;

        bool result = dest_var->getMemoryAddr(&dest_baseRegId, &dest_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG getMemoryAddr");
            return;
        }

        // str r8,[r9]
        // str r8, [fp, # - 16]
        store_base(src_reg_no, dest_baseRegId, dest_offset, tmp_reg_no, wide);
    }
}

/// @brief 加载栈内变量地址
/// @param rsReg 结果寄存器号
/// @param base_reg_no 基址寄存器
/// @param off 偏移
void ILocArm64::leaStack(int rs_reg_no, int base_reg_no, int off)
{
    std::string rs_reg_name = xregs(rs_reg_no);
    std::string base_reg_name = base_reg_no == ARM64_ZR_REG_NO ? "sp" : xregs(base_reg_no);

    if (off == 0)
        emit("mov", rs_reg_name, base_reg_name);
    else if (PlatformArm64::constExpr(off))
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
    // 超过四个的函数调用参数个数，多余8个，则需要栈传值
    int funcCallArgCnt = func->getMaxFuncCallArgCnt() - 8;
    if (funcCallArgCnt < 0) {
        funcCallArgCnt = 0;
    }

    int argSize = funcCallArgCnt * 8;
    argSize = (argSize + 15) & ~15;

    // 计算栈帧大小
    int off = func->getMaxDep() + argSize;

    // 不需要在栈内额外分配空间，则什么都不做
    if (0 == off)
        return;

    // 16对齐
    off = (off + 15) & ~15;
    func->setMaxDep(off);
    if (PlatformArm64::constExpr(off)) {
        // sub sp,sp,#16
        emit("sub", "sp", "sp", toStr(off));
    } else {
        // ldr r8,=257
        load_imm(tmp_reg_no, off);

        // sub sp,sp,r8
        emit("sub", "sp", "sp", xregs(tmp_reg_no));
    }

    // 函数调用通过栈传递的基址寄存器设置
    if (PlatformArm64::constExpr(argSize))
        emit("add", ARM64_FP, "sp", toStr(argSize));
    else {
        load_imm(tmp_reg_no, argSize);
        emit("add", ARM64_FP, "sp", xregs(tmp_reg_no));
    }
}

/// @brief 调用函数fun
/// @param fun
void ILocArm64::call_fun(cstr name)
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
void ILocArm64::jump(cstr label)
{
    emit("b", label);
}

void ILocArm64::branch(cstr cond, cstr label)
{
    emit("b" + cond, label);
}
