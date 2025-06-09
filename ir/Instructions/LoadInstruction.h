#pragma once

#include <string>

#include "Value.h"
#include "Instruction.h"

class Function;

///
/// @brief 载入指令
///
class LoadInstruction : public Instruction {

public:
    /**
     * @brief 构造函数
     * @param _func 所属的函数
     * @param addr 地址
     * @param type 基本类型
     */
    LoadInstruction(Function * _func, Value * addr, Type * type);

    /// @brief 转为字符串
    void toString(std::string & str) override;
};
