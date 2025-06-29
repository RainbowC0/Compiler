///
/// @file Common.cpp
/// @brief 共通函数
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

#include <ctype.h>
#include <stdio.h>
#include "Common.h"

extern "C" {
/// @brief 检查字符是否为大小写字符或数字(0-9)或下划线
/// @param ch 字符
/// @return true：是，false: 不是
bool isLetterDigitalUnderLine(char ch)
{
    return isalnum(ch) || (ch == '_');
}

/// @brief 检查字符是否为大小写字符或下划线
/// @param ch 字符
/// @return true：是，false：不是
bool isLetterUnderLine(char ch)
{
    return isalpha(ch) || (ch == '_');
}

void minic_log_common(int level, const char * content)
{
    fputs(content, stderr);
    fputc('\n', stderr);
}
}
