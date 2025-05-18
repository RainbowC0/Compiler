///
/// @file BitMap.h
/// @brief 位图类
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-09-19
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-19 <td>1.0     <td>zenglj  <td>新建
/// </table>
///
#pragma once
#include <vector>

///
/// @brief 非类型模板参数，容量N个字节
/// @tparam N 容量N个字节
///
class BitMap {
public:
    ///
    /// @brief Construct a new BitMap object
    ///
    BitMap(size_t N);

    void set(size_t x);
    void reset(size_t x);
    bool test(size_t x);

private:
    ///
    /// @brief 保存位图的数据
    ///
    std::vector<char> _bits;
};
