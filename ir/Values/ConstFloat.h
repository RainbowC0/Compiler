///
/// @file ConstFloat.h
/// @brief 浮点常量值类
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

#include "Value.h"
#include "FloatType.h"

///
/// @brief 浮点常量类
///
class ConstFloat : public Constant {
public:
    ///
    /// @brief 构造函数
    /// @param _val 浮点值
    ///
    explicit ConstFloat(float _val) : Constant(FloatType::getTypeFloat()), val(_val)
    {}

    ///
    /// @brief 获取浮点值
    /// @return 浮点值
    ///
    [[nodiscard]] float getVal() const
    {
        return val;
    }

    ///
    /// @brief 获取浮点值（别名方法）
    /// @return 浮点值
    ///
    [[nodiscard]] float getValue() const
    {
        return getVal();
    }

    ///
    /// @brief 获取IR变量名
    /// @return IR变量名
    ///
    [[nodiscard]] std::string getIRName() const override
    {
        return std::to_string(val);
    }

    void setVal(float f)
    {
        val = f;
    }

    float val = 0.f;

private:
    ///
    /// @brief 常量值
    ///
};
