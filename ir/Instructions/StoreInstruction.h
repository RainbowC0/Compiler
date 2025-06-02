#pragma once

#include <string>

#include "Value.h"
#include "Instruction.h"

class Function;

///
/// @brief 复制指令
///
class StoreInstruction : public Instruction {

public:
    ///
    /// @brief 构造函数
    /// @param _func 所属的函数
    /// @param result 目的地址
    /// @param srcVal1 源操作数
    ///
    StoreInstruction(Function * _func, PointerType * ptr, Value * srcVal1);

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};
