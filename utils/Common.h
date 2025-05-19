///
/// @file Common.cpp
/// @brief 共通函数头文件
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
#pragma once

/// @brief 整数变字符串
/// @param num 无符号数
/// @return 字符串
extern "C" {

/// @brief 检查字符是否为大小写字符或数字(0-9)或下划线
/// @param ch 字符
/// @return true：是，false: 不是
bool isLetterDigitalUnderLine(char ch);

/// @brief 检查字符是否为大小写字符或下划线
/// @param ch 字符
/// @return true：是，false：不是
bool isLetterUnderLine(char ch);

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_ERROR 2

void minic_log_common(int level, const char * content);

#define minic_log(level, fmt, args...)                                                                                 \
    do {                                                                                                               \
        char max_buf[1024];                                                                                            \
        snprintf(max_buf, sizeof(max_buf), "%s:%d " fmt "\n", __FILE__, __LINE__, ##args);                             \
        minic_log_common(level, max_buf);                                                                              \
    } while (0)
}
