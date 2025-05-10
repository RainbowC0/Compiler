///
/// @file FloatType.cpp
/// @brief 浮点型类型类的实现，用于描述float类型
///
/// @author [你的名字]
/// @version 1.0
/// @date [当前日期]
///
/// @copyright Copyright (c) [年份]
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>[当前日期] <td>1.0     <td>[你的名字]  <td>新建
/// </table>
///

#include "FloatType.h"

///
/// @brief 唯一的FLOAT类型实例
///
FloatType * FloatType::oneInstanceFloat;

///
/// @brief 获取类型float
/// @return FloatType*
///
FloatType * FloatType::getTypeFloat()
{
    // 只维持一份
    if (!oneInstanceFloat) {
        oneInstanceFloat = new FloatType();
    }
    return oneInstanceFloat;
}