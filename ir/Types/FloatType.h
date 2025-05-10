///
/// @file FloatType.h
/// @brief 浮点型类型类，用于描述float类型
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

#pragma once

#include <cstdint>
#include "Type.h"

class FloatType final : public Type {
public:
    ///
    /// @brief 获取类型，全局只有一份
    /// @return FloatType*
    ///
    static FloatType * getTypeFloat();

    ///
    /// @brief 获取类型的IR标识符
    /// @return std::string IR标识符float
    ///
    [[nodiscard]] std::string toString() const override
    {
        return "float";
    }

    ///
    /// @brief 获得类型所占内存空间大小
    /// @return int32_t
    ///
    [[nodiscard]] int32_t getSize() const override
    {
        return 4; // 通常float类型在内存中占4字节
    }

private:
    ///
    /// @brief 构造函数
    ///
    explicit FloatType() : Type(Type::FloatTyID)
    {}

    ///
    /// @brief 唯一的FLOAT类型实例
    ///
    static FloatType * oneInstanceFloat;
};