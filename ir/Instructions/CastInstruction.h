///
/// @file CastInstruction.h
/// @brief 类型转换指令
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
#pragma once

#include "../Instruction.h"

///
/// @brief 类型转换指令类
///
class CastInstruction : public Instruction {

public:
    ///
    /// @brief 转换类型枚举
    ///
    enum CastType {
        INT_TO_FLOAT, ///< 整数转浮点数
        FLOAT_TO_INT, ///< 浮点数转整数
        BOOL_TO_INT,  ///< 布尔值转整数
        INT_TO_BOOL   ///< 整数转布尔值
    };

    ///
    /// @brief 构造函数
    /// @param _func 所在的函数
    /// @param _srcVal 源操作数
    /// @param _targetType 目标类型
    /// @param _castType 转换类型
    ///
    CastInstruction(Function * _func, Value * _srcVal, Type * _targetType, CastType _castType);

    ///
    /// @brief 转换成字符串
    /// @param str 转换后的字符串
    ///
    void toString(std::string & str) override;

    ///
    /// @brief 获取转换类型
    /// @return 转换类型
    ///
    CastType getCastType() const
    {
        return castType;
    }

private:
    ///
    /// @brief 转换类型
    ///
    CastType castType;
};