///
/// @file IRGenerator.h
/// @brief AST遍历产生线性IR的头文件
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///
#pragma once

#include <unordered_map>

#include "AST.h"
#include "Module.h"
#include <sys/queue.h>

// 前向声明
class ArrayType;
class InterCode;

struct db {
    LabelInstruction *a, *b;
    SLIST_ENTRY(db) entries;
};

SLIST_HEAD(dbhead, db);

/// @brief AST遍历产生线性IR类
class IRGenerator {

public:
    /// @brief 构造函数
    /// @param root
    /// @param _module
    IRGenerator(ast_node * root, Module * _module);

    /// @brief 析构函数
    ~IRGenerator() = default;

    /// @brief 运行产生IR
    bool run();

protected:
    /// @brief 编译单元AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_compile_unit(ast_node * node);

    /// @brief 函数定义AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_define(ast_node * node);

    /// @brief 形式参数AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_formal_params(ast_node * node);

    /// @brief 函数调用AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_call(ast_node * node);

    /// @brief 语句块（含函数体）AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_block(ast_node * node);

    /// @brief 整数加法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_binary(ast_node * node);

    /// @brief 赋值AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_assign(ast_node * node);

    /// @brief return节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_return(ast_node * node);

    /// @brief 数组访问节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_access(ast_node * node);

    /// @brief 数组初始化节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_init(ast_node * node);

    bool ir_lval_to_r(ast_node * node);

    /// @brief 类型叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_type(ast_node * node);

    /// @brief 标识符叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_node_var_id(ast_node * node);

    /// @brief 无符号整数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_uint(ast_node * node);

    /// @brief float数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_float(ast_node * node);

    /// @brief 变量定声明节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_variable_declare(ast_node * node);

    /// @brief 未知节点类型的节点处理
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_default(ast_node * node);

    bool ir_branch(ast_node * node);

    bool ir_relop(ast_node * node);

    bool ir_loop(ast_node * node);

    bool ir_or(ast_node * node);
    bool ir_and(ast_node * node);
    bool ir_not(ast_node * node);

    bool ir_jump(ast_node * node);

    /// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
    /// @param node AST节点
    /// @return 成功返回node节点，否则返回nullptr
    ast_node * ir_visit_ast_node(ast_node * node);

    bool calcConstExpr(ast_node * node, void * ret);

    /// @brief 计算维度
    /// @return 值节点下标，-1无值节点
    int calcDims(ast_node * node);

    /// @brief 展开嵌套的初始化列表，处理缺省值
    /// @param node 初始化列表节点
    void expandNestedInitList(ast_node * node);

    /// @brief 扁平化初始化列表
    /// @param node 初始化列表节点
    /// @param flatList 扁平化后的列表
    void flattenInitList(ast_node * node, std::vector<ast_node*> & flatList);

    /// @brief 填充数组的缺省值
    void fillArrayDefaults(Value * arrayVal, ArrayType * arrayType, Function * func, InterCode & blockInsts);

    /// @brief 将扁平化索引转换为多维索引并生成GEP指令
    /// @param arrayVal 数组变量
    /// @param arrayType 数组类型
    /// @param flatIndex 扁平化索引
    /// @param func 当前函数
    /// @return GEP指令
    Instruction * generateMultiDimGEP(Value * arrayVal, ArrayType * arrayType, 
                                     uint32_t flatIndex, Function * func);

    /// @brief 计算数组总元素个数
    uint32_t getTotalArrayElements(ArrayType * arrayType);

    /// @brief 生成数组初始化指令
    /// @param arrayVal 数组变量
    /// @param arrayType 数组类型
    /// @param initList 初始化列表节点
    /// @param func 当前函数
    /// @param blockInsts 指令块
    /// @param baseOffset 基础偏移量
    void generateArrayInitInstructions(Value * arrayVal, ArrayType * arrayType, 
                                      ast_node * initList, Function * func, 
                                      InterCode & blockInsts, uint32_t baseOffset);

    /// @brief 生成简单的数组初始化指令
    /// @param arrayVal 数组变量
    /// @param arrayType 数组类型
    /// @param initList 初始化列表节点
    /// @param func 当前函数
    /// @param blockInsts 指令块
    void generateSimpleArrayInit(Value * arrayVal, ArrayType * arrayType, 
                                ast_node * initList, Function * func, 
                                InterCode & blockInsts);

    /// @brief 生成简单的数组初始化指令（带偏移量）
    /// @param arrayVal 数组变量
    /// @param arrayType 数组类型
    /// @param initList 初始化列表节点
    /// @param func 当前函数
    /// @param blockInsts 指令块
    /// @param baseOffset 基础偏移量
    void generateSimpleArrayInitWithOffset(Value * arrayVal, ArrayType * arrayType, 
                                          ast_node * initList, Function * func, 
                                          InterCode & blockInsts, uint32_t baseOffset);

    /// @brief AST的节点操作函数
    typedef bool (IRGenerator::*ast2ir_handler_t)(ast_node *);

    /// @brief AST节点运算符与动作函数关联的映射表
    std::unordered_map<ast_operator_type, ast2ir_handler_t> ast2ir_handlers;

private:
    /// @brief 抽象语法树的根
    ast_node * root;

    /// @brief 符号表:模块
    Module * module;

    /// @brief 循环入口出口栈
    dbhead labs;
};
