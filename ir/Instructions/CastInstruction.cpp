///
/// @file CastInstruction.cpp
/// @brief 类型转换指令实现
///
/// @author [用户]
/// @version 1.0
/// @date [当前日期]
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>[当前日期] <td>1.0     <td>[用户]  <td>新建
/// </table>
///
#include <string>
#include "../Function.h"
#include "../Instruction.h"
#include "CastInstruction.h"

///
/// @brief 构造函数
/// @param _func 所在的函数
/// @param _srcVal 源操作数
/// @param _targetType 目标类型
/// @param _castType 转换类型
///
CastInstruction::CastInstruction(Function * _func, Value * _srcVal, Type * _targetType, CastType _castType)
    : Instruction(_func, IRINST_OP_CAST, _targetType), castType(_castType)
{
    addOperand(_srcVal);
}

///
/// @brief 转换成字符串
/// @param str 转换后的字符串
///
void CastInstruction::toString(std::string & str)
{
    Value * src = getOperand(0);
    const char * castOp;

    switch (castType) {
        case INT_TO_FLOAT:
            castOp = " = sitofp ";
            break;
        case FLOAT_TO_INT:
            castOp = " = fptosi ";
            break;
        case BOOL_TO_INT:
            castOp = " = zext ";
            break;
        case INT_TO_BOOL:
            castOp = " = trunc ";
            break;
        default:
            castOp = " = cast ";
            break;
    }

    str = getIRName() + castOp + src->getIRName() + " to " + getType()->toString();
}