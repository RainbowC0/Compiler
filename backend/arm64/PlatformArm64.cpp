///
/// @file PlatformArm64.cpp
/// @brief  ARM32平台相关实现
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-11-21
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-11-21 <td>1.0     <td>zenglj  <td>新做
/// </table>
///
#include "PlatformArm64.h"

#include "IntegerType.h"
#include <algorithm>

const std::string PlatformArm64::regName[] = {
    "w0",  // 用于传参或返回值等，不需要栈保护
    "w1",  // 用于传参或返回值（64位结果时后32位）等，不需要栈保护
    "w2",  // 用于传参等，不需要栈保护
    "w3",  // 用于传参等，不需要栈保护
    "w4",  // 需要栈保护
    "w5",  // 需要栈保护
    "w6",  // 需要栈保护
    "w7",  // 需要栈保护
    "w8",  // 用于加载操作数1,保存表达式结果
    "w9",  // 用于加载操作数2,写回表达式结果,立即数，标签地址
    "w10", // 用于保存乘法结果，虽然mul
    "w11",
    "w12",
    "w13",
    "w14",
    "w15",
    "w16",
    "w17",
    "w18",
    "w19",
    "w20",
    "w21",
    "w22",
    "w23",
    "w24",
    "w25",
    "w26",
    "w27",
    "w28",
    "x29",
    "x30",
    "sp",
    //
    "wzr"
};

RegVariable * PlatformArm64::intRegVal[PlatformArm64::maxRegNum] = {
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[0], 0),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[1], 1),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[2], 2),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[3], 3),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[4], 4),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[5], 5),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[6], 6),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[7], 7),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[8], 8),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[9], 9),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[10], 10),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[11], 11),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[12], 12),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[13], 13),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[14], 14),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[15], 15),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[16], 16),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[17], 17),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[18], 18),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[19], 19),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[20], 20),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[21], 21),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[22], 22),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[23], 23),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[24], 24),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[25], 25),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[26], 26),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[27], 27),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[28], 28),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[29], 29),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[30], 30),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[31], 31),
};

/// @brief 循环左移两位
/// @param num
void PlatformArm64::roundLeftShiftTwoBit(unsigned int & num)
{
    // 取左移即将溢出的两位
    const unsigned int overFlow = num & 0xc0000000;

    // 将溢出部分追加到尾部
    num = (num << 2) | (overFlow >> 30);
}

/// @brief 判断num是否是常数表达式，8位数字循环右移偶数位得到
/// @param num
/// @return
bool PlatformArm64::__constExpr(int num)
{
    unsigned int new_num = (unsigned int) num;

    for (int i = 0; i < 16; i++) {

        if (new_num <= 0xff) {
            // 有效表达式
            return true;
        }

        // 循环左移2位
        roundLeftShiftTwoBit(new_num);
    }

    return false;
}

/// @brief 同时处理正数和负数
/// @param num
/// @return
bool PlatformArm64::constExpr(int num)
{
    return imm12sh(num) || imm12sh(-num);
}

bool PlatformArm64::test(int num)
{
    return false;
}

/// @brief add/sub等imm12判断
bool PlatformArm64::imm12sh(int num)
{
    return num >= 0 && (num < 4096 || ((num & 4095) == 0 && num < 4095*4096));
}

/// @brief 判定是否是合法的偏移
/// @param num
/// @return
bool PlatformArm64::isDisp(int num)
{
    return num < 4096 && num > -4096;
}

/// @brief 判断是否是合法的寄存器名
/// @param s 寄存器名字
/// @return 是否是
bool PlatformArm64::isReg(const std::string &name)
{
    return std::find(regName, regName+(sizeof(regName)/sizeof(regName[0])), name);
}
