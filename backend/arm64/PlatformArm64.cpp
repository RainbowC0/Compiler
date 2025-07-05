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

#include "FloatType.h"
#include "IntegerType.h"
#include "RegVariable.h"
#include <algorithm>

const std::string PlatformArm64::regName[] = {
    "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8", "w9", "w10", 
    "w11", "w12", "w13", "w14", "w15", "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23", "w24",
    "w25", "w26", "w27", "w28", "w29", "w30", "wzr", "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",
    "s7",  "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20",
    "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
};

const std::string PlatformArm64::xregName[] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", 
    "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23", "x24",
    "x25", "x26", "x27", "x28", "x29", "x30", "xzr", "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",
    "d7",  "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20",
    "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
};

RegVariable * PlatformArm64::regVal[] = {
#define _int(i) new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[i], i)
#define _float(i) new RegVariable(FloatType::getTypeFloat(), PlatformArm64::regName[i], i)
    _int(0),    _int(1),    _int(2),    _int(3),    _int(4),    _int(5),    _int(6),    _int(7),
    _int(8),    _int(9),    _int(10),   _int(11),   _int(12),   _int(13),   _int(14),   _int(15),
    _int(16),   _int(17),   _int(18),   _int(19),   _int(20),   _int(21),   _int(22),   _int(23),
    _int(24),   _int(25),   _int(26),   _int(27),   _int(28),   _int(29),   _int(30),   _int(31),
    _float(32), _float(33), _float(34), _float(35), _float(36), _float(37), _float(38), _float(39),
    _float(40), _float(41), _float(42), _float(43), _float(44), _float(45), _float(46), _float(47),
    _float(48), _float(49), _float(50), _float(51), _float(52), _float(53), _float(54), _float(55),
    _float(56), _float(57), _float(58), _float(59), _float(60), _float(61), _float(62), _float(63),
#undef _int
#undef _float
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
    return num >= 0 && (num < 4096 || ((num & 4095) == 0 && num < 4095 * 4096));
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
bool PlatformArm64::isReg(const std::string & name)
{
    return std::find(regName, regName + (sizeof(regName) / sizeof(regName[0])), name);
}
