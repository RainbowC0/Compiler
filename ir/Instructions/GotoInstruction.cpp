///
/// @file GotoInstruction.cpp
/// @brief 无条件跳转指令即goto指令
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

#include "VoidType.h"

#include "GotoInstruction.h"

///
/// @brief 无条件跳转指令的构造函数
/// @param target 跳转目标
///
GotoInstruction::GotoInstruction(Function * _func, Instruction * _target)
    : Instruction(_func, IRInstOperator::IRINST_OP_GOTO, VoidType::getType())
{
    // 真假目标一样，则无条件跳转
    iftrue = static_cast<LabelInstruction *>(_target);
}

GotoInstruction::GotoInstruction(Function * func, Value * cond, Instruction * iftrue, Instruction * iffalse)
    : Instruction(func, IRInstOperator::IRINST_OP_GOTO, VoidType::getType())
{
    this->cond = cond;
    this->iftrue = static_cast<LabelInstruction *>(iftrue);
    this->iffalse = static_cast<LabelInstruction *>(iffalse);
}

/// @brief 转换成IR指令文本
void GotoInstruction::toString(std::string & str)
{
    if (cond && iffalse)
        str = "br " + cond->getIRName() + ", label " + iftrue->getIRName() + ", label " + iffalse->getIRName();
    else
        str = "br label " + iftrue->getIRName();
}

///
/// @brief 获取目标Label指令
/// @return LabelInstruction* label指令
///

Value * GotoInstruction::getCondiValue() const
{
    return cond;
}
