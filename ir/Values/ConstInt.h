///
/// @file ConstInt.h
/// @brief int类型的常量
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

#pragma once

#include "Constant.h"
#include "IRConstant.h"
#include "IntegerType.h"

///
/// @brief 整型常量类
///

//static int32_t zeroReg = -1;

class ConstInt : public Constant {

public:
    static int32_t zeroReg;
    ///
    /// @brief 指定值的常量
    /// \param val
    explicit ConstInt(int32_t val) : Constant(IntegerType::getTypeInt())
    {
        name = std::to_string(val);
        intVal = val;
    }

    /// @brief 获取名字
    /// @return 变量名
    [[nodiscard]] std::string getIRName() const override
    {
        return name;
    }

    ///
    /// @brief 获取值
    /// @return int32_t
    ///
    int32_t getVal();

    ///
    /// @brief 获取值（别名方法）
    /// @return int32_t
    ///
    int32_t getValue() { return getVal(); }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    int32_t getLoadRegId() override
    {
        return this->loadRegNo;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    void setLoadRegId(int32_t regId) override
    {
        this->loadRegNo = regId;
    }

    int32_t getRegId() override {
        return intVal==0?zeroReg:-1;
    }

    void setVal(int32_t i);

    static void setZeroReg(int32_t reg);

private:
    ///
    /// @brief 整数值
    ///
    int32_t intVal = 0;

    ///
    /// @brief 变量加载到寄存器中时对应的寄存器编号
    ///
    int32_t loadRegNo = -1;

};
