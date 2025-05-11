///
/// @file BinaryInstruction.cpp
/// @brief 二元操作指令
///
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-09-29
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// </table>
///
#include "BinaryInstruction.h"

/// @brief 构造函数
/// @param _op 操作符
/// @param _result 结果操作数
/// @param _srcVal1 源操作数1
/// @param _srcVal2 源操作数2
BinaryInstruction::BinaryInstruction(Function * _func,
                                     IRInstOperator _op,
                                     Value * _srcVal1,
                                     Value * _srcVal2,
                                     Type * _type)
    : Instruction(_func, _op, _type)
{
    addOperand(_srcVal1);
    addOperand(_srcVal2);
}

/// @brief 转换成字符串
/// @param str 转换后的字符串
void BinaryInstruction::toString(std::string & str)
{

    Value *src1 = getOperand(0), *src2 = getOperand(1);
    const char *opstr;

    switch (op) {
        case IROP(IADD): opstr = " = add "; break;
        case IROP(ISUB): opstr = " = sub "; break;
        case IROP(IMUL): opstr = " = mul "; break;
        case IROP(IDIV): opstr = " = div "; break;
        case IROP(IMOD): opstr = " = urem "; break;
        case IROP(IEQ): opstr = " = icmp eq "; break;
        case IROP(INE): opstr = " = icmp ne "; break;
        case IROP(IGT): opstr = " = icmp sgt "; break;
        case IROP(IGE): opstr = " = icmp sge "; break;
        case IROP(ILT): opstr = " = icmp slt "; break;
        case IROP(ILE): opstr = " = icmp sle "; break;
        case IROP(XOR): opstr = " = xor "; break;
        default: opstr = NULL;
    }
    if (opstr)
        str = getIRName() + opstr + src1->getIRName() + "," + src2->getIRName();
    else
        Instruction::toString(str);
}
