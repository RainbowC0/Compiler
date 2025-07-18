///
/// @file IRGenerator.cpp
/// @brief AST遍历产生线性IR的源文件
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
// #include <algorithm>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "AST.h"
#include "AttrType.h"
#include "Common.h"
#include "ConstInt.h"
#include "Function.h"
#include "IRCode.h"
#include "IRGenerator.h"
#include "Module.h"
#include "EntryInstruction.h"
#include "LabelInstruction.h"
#include "ExitInstruction.h"
#include "FuncCallInstruction.h"
#include "BinaryInstruction.h"
#include "MoveInstruction.h"
#include "StoreInstruction.h"
#include "LoadInstruction.h"
#include "GotoInstruction.h"
#include "Type.h"
#include "TypeSystem.h"
#include "CastInstruction.h"
#include "ConstFloat.h"
#include "FloatType.h"
#include "ArrayType.h"

#define CHECK_NODE(name, son)                                                                                          \
    name = ir_visit_ast_node(son);                                                                                     \
    if (!name) {                                                                                                       \
        fprintf(stdout, "IR解析错误:%s:%d\n", __FUNCTION__, __LINE__);                                                 \
        return (false);                                                                                                \
    }

extern "C" {
static inline IRInstOperator irtype(ast_operator_type type);
static inline void backPatch(std::vector<LabelInstruction **> * a, LabelInstruction * b)
{
    for (auto i: *a) {
        *i = b;
    }
}
static inline std::vector<LabelInstruction **> * merge(std::vector<LabelInstruction **> *,
                                                       std::vector<LabelInstruction **> *);
}

/// @brief 构造函数
/// @param _root AST的根
/// @param _module 符号表
IRGenerator::IRGenerator(ast_node * _root, Module * _module) : root(_root), module(_module)
{
    /* 叶子节点 */
    ast2ir_handlers[ASTOP(LEAF_LITERAL_INT)] = &IRGenerator::ir_leaf_node_uint;
    ast2ir_handlers[ASTOP(LEAF_LITERAL_FLOAT)] = &IRGenerator::ir_leaf_node_float;
    ast2ir_handlers[ASTOP(VAR_ID)] = &IRGenerator::ir_node_var_id;
    ast2ir_handlers[ASTOP(LEAF_TYPE)] = &IRGenerator::ir_leaf_node_type;

    /* 表达式运算， 加减 */
    ast2ir_handlers[ast_operator_type::AST_OP_SUB] = &IRGenerator::ir_binary;
    ast2ir_handlers[ast_operator_type::AST_OP_ADD] = &IRGenerator::ir_binary;
    ast2ir_handlers[ast_operator_type::AST_OP_MUL] = &IRGenerator::ir_binary;
    ast2ir_handlers[ast_operator_type::AST_OP_DIV] = &IRGenerator::ir_binary;
    ast2ir_handlers[ast_operator_type::AST_OP_MOD] = &IRGenerator::ir_binary;
    ast2ir_handlers[ASTOP(EQ)] = &IRGenerator::ir_relop;
    ast2ir_handlers[ASTOP(NE)] = &IRGenerator::ir_relop;
    ast2ir_handlers[ASTOP(GT)] = &IRGenerator::ir_relop;
    ast2ir_handlers[ASTOP(GE)] = &IRGenerator::ir_relop;
    ast2ir_handlers[ASTOP(LT)] = &IRGenerator::ir_relop;
    ast2ir_handlers[ASTOP(LE)] = &IRGenerator::ir_relop;

    ast2ir_handlers[ASTOP(LOR)] = &IRGenerator::ir_or;
    ast2ir_handlers[ASTOP(LAND)] = &IRGenerator::ir_and;
    ast2ir_handlers[ASTOP(NOT)] = &IRGenerator::ir_not;

    /* 数组访问 */
    ast2ir_handlers[ASTOP(ARRAY_ACCESS)] = &IRGenerator::ir_array_access;

    /* 多维数组相关 */
    ast2ir_handlers[ASTOP(ARRAY_INIT)] = &IRGenerator::ir_array_init;

    ast2ir_handlers[ASTOP(L2R)] = &IRGenerator::ir_lval_to_r;

    ast2ir_handlers[ASTOP(BREAK)] = &IRGenerator::ir_jump;
    ast2ir_handlers[ASTOP(CONTINUE)] = &IRGenerator::ir_jump;

    /* 语句 */
    ast2ir_handlers[ast_operator_type::AST_OP_ASSIGN] = &IRGenerator::ir_assign;
    ast2ir_handlers[ast_operator_type::AST_OP_RETURN] = &IRGenerator::ir_return;

    /* 函数调用 */
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_CALL] = &IRGenerator::ir_function_call;

    /* 函数定义 */
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_DEF] = &IRGenerator::ir_function_define;
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS] = &IRGenerator::ir_function_formal_params;

    /* 变量定义语句 */
    ast2ir_handlers[ast_operator_type::AST_OP_VAR_DECL] = &IRGenerator::ir_variable_declare;

    /* 语句块 */
    ast2ir_handlers[ast_operator_type::AST_OP_BLOCK] = &IRGenerator::ir_block;

    /* 编译单元 */
    ast2ir_handlers[ast_operator_type::AST_OP_COMPILE_UNIT] = &IRGenerator::ir_compile_unit;

    ast2ir_handlers[ast_operator_type::AST_OP_IF] = &IRGenerator::ir_branch;
    ast2ir_handlers[ast_operator_type::AST_OP_WHILE] = &IRGenerator::ir_loop;
    ast2ir_handlers[ASTOP(DOWHILE)] = &IRGenerator::ir_loop;

    ast2ir_handlers[ASTOP(NULL_STMT)] = &IRGenerator::ir_default;

    SLIST_INIT(&labs);
}

/// @brief 遍历抽象语法树产生线性IR，保存到IRCode中
/// @param root 抽象语法树
/// @param IRCode 线性IR
/// @return true: 成功 false: 失败
bool IRGenerator::run()
{
    ast_node * node;

    // 从根节点进行遍历
    node = ir_visit_ast_node(root);

    return node != nullptr;
}

/// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
/// @param node AST节点
/// @return 成功返回node节点，否则返回nullptr
ast_node * IRGenerator::ir_visit_ast_node(ast_node * node)
{
    // 空节点
    if (nullptr == node) {
        return nullptr;
    }

    bool result;

    std::unordered_map<ast_operator_type, ast2ir_handler_t>::const_iterator pIter;
    pIter = ast2ir_handlers.find(node->node_type);
    if (pIter == ast2ir_handlers.end()) {
        // 没有找到，则说明当前不支持
        result = (this->ir_default)(node);
    } else {
        result = (this->*(pIter->second))(node);
    }

    if (!result) {
        // 语义解析错误，则出错返回
        node = nullptr;
    }

    return node;
}

/// @brief 未知节点类型的节点处理
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_default(ast_node * node)
{
    // 未知的节点
    if (node->node_type != ASTOP(NULL_STMT)) {
        printf("Unkown node(%d) at line %lld\n", (int) node->node_type, (long long) node->line_no);
        if (!node->name.empty()) {
            printf("Node name: %s\n", node->name.c_str());
        }
        printf("Node has %zu children\n", node->sons.size());
    }
    return true;
}

/// @brief 编译单元AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_compile_unit(ast_node * node)
{
    module->setCurrentFunction(nullptr);

    for (auto son: node->sons) {

        // 遍历编译单元，要么是函数定义，要么是语句
        ast_node * CHECK_NODE(son_node, son);
    }

    return true;
}

/// @brief 函数定义AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_define(ast_node * node)
{
    bool result;

    // 创建一个函数，用于当前函数处理
    if (module->getCurrentFunction()) {
        // 函数中嵌套定义函数，这是不允许的，错误退出
        // TODO 自行追加语义错误处理
        return false;
    }

    // 函数定义的AST包含四个孩子
    // 第一个孩子：函数返回类型
    // 第二个孩子：函数名字
    // 第三个孩子：形参列表
    // 第四个孩子：函数体即block
    ast_node * type_node = node->sons[0];
    ast_node * name_node = node->sons[1];
    ast_node * param_node = node->sons[2];
    ast_node * block_node = node->sons[3];

    // 创建一个新的函数定义，函数的返回类型设置为VOID，待定，必须等return时才能确定，目前可以是VOID或者INT类型
    // 请注意这个与C语言的函数定义不同。在实现MiniC编译器时必须调整
    Function * newFunc = module->newFunction(name_node->name, type_node->type);
    if (!newFunc) {
        // 新定义的函数已经存在，则失败返回。
        // TODO 自行追加语义错误处理
        minic_log(LOG_ERROR, "函数重复定义：%s", name_node->name.c_str());
        return false;
    }

    // 当前函数设置有效，变更为当前的函数
    module->setCurrentFunction(newFunc);

    // 进入函数的作用域
    module->enterScope();

    // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
    InterCode & irCode = newFunc->getInterCode();

    // 这里也可增加一个函数入口Label指令，便于后续基本块划分

    // 创建并加入Entry入口指令
    irCode.addInst(new EntryInstruction(newFunc));

    // 创建出口指令并不加入出口指令，等函数内的指令处理完毕后加入出口指令
    LabelInstruction * exitLabelInst = new LabelInstruction(newFunc);

    // 函数出口指令保存到函数信息中，因为在语义分析函数体时return语句需要跳转到函数尾部，需要这个label指令
    newFunc->setExitLabel(exitLabelInst);

    // 遍历形参，没有IR指令，不需要追加
    result = ir_function_formal_params(param_node);
    if (!result) {
        // 形参解析失败
        // TODO 自行追加语义错误处理
        fputs("形参解析失败\n", stderr);
        return false;
    }
    node->blockInsts.addInst(param_node->blockInsts);

    // 新建一个Value，用于保存函数的返回值，如果没有返回值可不用申请
    LocalVariable * retValue = nullptr;
    if (!type_node->type->isVoidType()) {

        // 保存函数返回值变量到函数信息中，在return语句翻译时需要设置值到这个变量中
        retValue = static_cast<LocalVariable *>(module->newVarValue(type_node->type));
    }
    newFunc->setReturnValue(retValue);

    // 函数内已经进入作用域，内部不再需要做变量的作用域管理
    block_node->needScope = false;

    // 遍历block
    result = ir_block(block_node);
    if (!result) {
        // block解析失败
        // TODO 自行追加语义错误处理
        return false;
    }

    // IR指令追加到当前的节点中
    node->blockInsts.addInst(block_node->blockInsts);

    // 此时，所有指令都加入到当前函数中，也就是node->blockInsts

    // node节点的指令移动到函数的IR指令列表中
    irCode.addInst(node->blockInsts);

    // 添加函数出口Label指令，主要用于return语句跳转到这里进行函数的退出
    irCode.addInst(exitLabelInst);

    // 函数出口指令
    irCode.addInst(new ExitInstruction(newFunc, retValue));

    // 恢复成外部函数
    module->setCurrentFunction(nullptr);

    // 退出函数的作用域
    module->leaveScope();

    return true;
}

/// @brief 形式参数AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_formal_params(ast_node * node)
{
    // TODO 目前形参还不支持，直接返回true

    // 每个形参变量都创建对应的临时变量，用于表达实参转递的值
    // 而真实的形参则创建函数内的局部变量。
    // 然后产生赋值指令，用于把表达实参值的临时变量拷贝到形参局部变量上。
    // 请注意这些指令要放在Entry指令后面，因此处理的先后上要注意。
    for (auto i: node->sons) {
        calcDims(i);
        module->newFuncParam(i->type, i->name);
    }

    return true;
}

/// @brief 函数调用AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_call(ast_node * node)
{
    std::vector<Value *> realParams;

    // 获取当前正在处理的函数
    Function * currentFunc = module->getCurrentFunction();

    // 函数调用的节点包含两个节点：
    // 第一个节点：函数名节点
    // 第二个节点：实参列表节点

    const std::string & funcName = node->sons[0]->name;
    int64_t lineno = node->sons[0]->line_no;

    // 根据函数名查找函数，看是否存在。若不存在则出错
    // 这里约定函数必须先定义后使用
    auto calledFunction = module->findFunction(funcName);
    if (nullptr == calledFunction) {
        minic_log(LOG_ERROR, "函数(%s)未定义或声明", funcName.c_str());
        return false;
    }

    // 当前函数存在函数调用
    currentFunc->setExistFuncCall(true);

    // 如果没有孩子，也认为是没有参数
    if (node->sons.size() > 1) {
        ast_node * paramsNode = node->sons[1];
        if (!paramsNode->sons.empty()) {
            int32_t argsCount = (int32_t) paramsNode->sons.size();

            // 当前函数中调用函数实参个数最大值统计，实际上是统计实参传参需在栈中分配的大小
            // 因为目前的语言支持的int和float都是四字节的，只统计个数即可
            if (argsCount > currentFunc->getMaxFuncCallArgCnt()) {
                currentFunc->setMaxFuncCallArgCnt(argsCount);
            }

            // 遍历参数列表，孩子是表达式
            // 这里自左往右计算表达式
            auto pIter = calledFunction->getParams().begin();
            for (auto son: paramsNode->sons) {

                // 遍历Block的每个语句，进行显示或者运算
                ast_node * CHECK_NODE(temp, son);
                Value * v = temp->val;
				//函数调用不会隐式转换，而是直接传值
                Type * rtype = v->getType();
                Type * ftype = (*pIter)->getType();

                if (rtype != ftype) {
                    if (rtype->isIntegerType() && ftype->isFloatType()) {
                        v = new CastInstruction(currentFunc, temp->val, ftype, CastInstruction::INT_TO_FLOAT);
                        temp->blockInsts.addInst((Instruction *) v);
                    } else if (rtype->isFloatType() && ftype->isIntegerType()) {
                        v = new CastInstruction(currentFunc, temp->val, ftype, CastInstruction::FLOAT_TO_INT);
                        temp->blockInsts.addInst((Instruction *) v);
                    }
                }

                realParams.push_back(v);
                node->blockInsts.addInst(temp->blockInsts);
                pIter++;
            }
        }
    }

    // TODO 这里请追加函数调用的语义错误检查，这里只进行了函数参数的个数检查等，其它请自行追加。
    if (realParams.size() != calledFunction->getParams().size()) {
        // 函数参数的个数不一致，语义错误
        minic_log(LOG_ERROR, "第%lld的函数%s参数个数有误", (long long) lineno, funcName.c_str());
        return false;
    }

    // 返回调用有返回值，则需要分配临时变量，用于保存函数调用的返回值
    Type * type = calledFunction->getReturnType();

    FuncCallInstruction * funcCallInst = new FuncCallInstruction(currentFunc, calledFunction, realParams, type);

    // 创建函数调用指令
    node->blockInsts.addInst(funcCallInst);

    // 函数调用结果Value保存到node中，可能为空，上层节点可利用这个值
    node->val = funcCallInst;

    return true;
}

/// @brief 语句块（含函数体）AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_block(ast_node * node)
{
    // 进入作用域
    if (node->needScope) {
        module->enterScope();
    }

    std::vector<ast_node *>::iterator pIter;
    for (pIter = node->sons.begin(); pIter != node->sons.end(); ++pIter) {

        // 遍历Block的每个语句，进行显示或者运算
        ast_node * CHECK_NODE(temp, *pIter);

        node->blockInsts.addInst(temp->blockInsts);
    }

    // 离开作用域
    if (node->needScope) {
        module->leaveScope();
    }

    return true;
}

/// @brief 整数加法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_binary(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 加法的左边操作数
    ast_node * CHECK_NODE(left, src1_node);

    // 加法的右边操作数
    ast_node * CHECK_NODE(right, src2_node);

    // 如果需要类型转换
    Value * leftVal = left->val;
    Value * rightVal = right->val;

    // 获取操作数类型，并确定结果类型和适合的操作符
    Type * leftType = leftVal->getType();
    Type * rightType = rightVal->getType();

    // 获取适当的操作符
    IRInstOperator op = TypeSystem::getAppropriateOp(irtype(node->node_type), leftType, rightType);

    // 获取结果类型
    Type * resultType = TypeSystem::getBinaryResultType(op, leftType, rightType);

    Type * opType = TypeSystem::getCommonType(leftType, rightType);

    Function * func = module->getCurrentFunction();

    // 如果操作数类型与结果类型不同，需要进行隐式类型转换
    // 包括float bool int
    if (opType->isFloatType()) {
        if (Instanceof(leftInt, ConstInt *, leftVal)) {
            leftVal = module->newConstFloat((float) leftInt->getVal());
        } else if (Instanceof(rightInt, ConstInt *, rightVal)) {
            rightVal = module->newConstFloat((float) rightInt->getVal());
        } else if (!leftType->isFloatType()) {
            // 整数转浮点 - 使用CastInstruction
            Instruction * castInst = new CastInstruction(func, leftVal, rightType, CastInstruction::INT_TO_FLOAT);
            left->blockInsts.addInst(castInst);
            leftVal = castInst;
        } else if (!rightType->isFloatType()) {
            // 整数转浮点 - 使用CastInstruction
            Instruction * castInst = new CastInstruction(func, rightVal, leftType, CastInstruction::INT_TO_FLOAT);
            right->blockInsts.addInst(castInst);
            rightVal = castInst;
        }
    }

    Instruction * addInst = new BinaryInstruction(func, op, leftVal, rightVal, resultType);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(addInst);

    node->val = addInst;

    // 设置节点的类型
    node->type = resultType;

    return true;
}

bool IRGenerator::ir_relop(ast_node * node)
{
    bool ret = ir_binary(node);
    if (!ret)
        return false;

    ast_node * parent = node->parent;
    ast_operator_type tp;
    Function * func = module->getCurrentFunction();
    if (parent && ((tp = parent->node_type) == ASTOP(LAND) || tp == ASTOP(LOR) || tp == ASTOP(IF) ||
                   tp == ASTOP(WHILE) || tp == ASTOP(DOWHILE))) {
        auto go = new GotoInstruction(func, node->val, nullptr, nullptr);
        node->truelist = new std::vector<LabelInstruction **>{&go->iftrue};
        node->falselist = new std::vector<LabelInstruction **>{&go->iffalse};
        node->blockInsts.addInst(go);
    } else {
        auto cast = new CastInstruction(func, node->val, IntegerType::getTypeInt(), CastInstruction::BOOL_TO_INT);
        node->blockInsts.addInst(cast);
        node->val = cast;
    }

    return true;
}

/// @brief 赋值AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_assign(ast_node * node)
{
    ast_node * son1_node = node->sons[0];
    ast_node * son2_node = node->sons[1];

    // 赋值节点，自右往左运算

    // 赋值运算符的左侧操作数
    ast_node * left = ir_visit_ast_node(son1_node);
    if (!left) {
        minic_log(LOG_ERROR, "赋值左值变量未定义或作用域隐藏");
        return false;
    }

    // 赋值运算符的右侧操作数
    ast_node * right = ir_visit_ast_node(son2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 支持float类型赋值：若类型不一致且目标为float，自动插入类型转换
    Value * leftVal = left->val;
    Value * rightVal = right->val;
    Type * leftType = leftVal->getType();
    if (leftType->isArrayType())
        leftType = (Type *)((ArrayType *)leftType)->getBaseElementType();
    Type * rightType = rightVal->getType();
    Function * func = module->getCurrentFunction();
    if (leftType != rightType) {
        if (leftType->isFloatType() && rightType->isIntegerType()) {
            // int->float
            auto * castInst =
                new CastInstruction(func, rightVal, FloatType::getTypeFloat(), CastInstruction::INT_TO_FLOAT);
            right->blockInsts.addInst(castInst);
            rightVal = castInst;
        } else if (leftType->isIntegerType() && rightType->isFloatType()) {
            // float->int
            auto cast = new CastInstruction(func, rightVal, IntegerType::getTypeInt(), CastInstruction::FLOAT_TO_INT);
            right->blockInsts.addInst(cast);
            rightVal = cast;
        }
    }
    Instruction * movInst;
    if (left->node_type == ASTOP(ARRAY_ACCESS)) {
        movInst = new StoreInstruction(func, leftVal, rightVal);
    } else {
        movInst = new MoveInstruction(func, leftVal, rightVal);
    }

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(movInst);

    // 这里假定赋值的类型是一致的
    node->val = movInst;

    return true;
}

/// @brief return节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_return(ast_node * node)
{
    ast_node * right = nullptr;

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    Function * currentFunc = module->getCurrentFunction();

    // return语句可能没有没有表达式，也可能有，因此这里必须进行区分判断
    if (node->sons.empty()) {
        node->blockInsts.addInst(new GotoInstruction(currentFunc, currentFunc->getExitLabel()));
        return true;
    }

    ast_node * son_node = node->sons[0];

    // 返回的表达式的指令保存在right节点中
    right = ir_visit_ast_node(son_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);

    Value * val = right->val;
    Type * retType = currentFunc->getReturnType();
    if (val->getType() != retType) {
        val = new CastInstruction(currentFunc, val, retType, retType->isFloatType() ? CastInstruction::INT_TO_FLOAT : CastInstruction::FLOAT_TO_INT);
        node->blockInsts.addInst((Instruction*)val);
    }
    // 返回值赋值到函数返回值变量上，然后跳转到函数的尾部
    node->blockInsts.addInst(new MoveInstruction(currentFunc, currentFunc->getReturnValue(), val));

    // 跳转到函数的尾部出口指令上
    node->blockInsts.addInst(new GotoInstruction(currentFunc, currentFunc->getExitLabel()));

    node->val = right->val;

    // TODO 设置类型

    return true;
}

/// @brief 类型叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_type(ast_node * node)
{
    // 不需要做什么，直接从节点中获取即可。

    return !node->type->isVoidType();
}

/// @brief 标识符叶子节点翻译成线性中间IR，变量声明的不走这个语句
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_node_var_id(ast_node * node)
{
    Value * val;
    val = module->findVarValue(node->name);
    if (!val) {
        minic_log(LOG_ERROR, "变量(%s)未定义或作用域隐藏", node->name.c_str());
        return false;
    }
    node->val = val;
    return true;
}

/// @brief 无符号整数字面量叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_uint(ast_node * node)
{
    ConstInt * val;

    // 新建一个整数常量Value
    val = module->newConstInt(node->integer_val);

    node->val = val;

    return true;
}

/// @brief float数字面量叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_float(ast_node * node)
{
    // 创建浮点常量Value
    ConstFloat * val = module->newConstFloat(node->float_val);

    node->val = val;

    return true;
}

static std::vector<int32_t> * vec;

static void init_global_array(ast_node * node)
{
    vec->push_back(node->integer_val);
}
/// @brief 变量声明语句节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_variable_declare(ast_node * node)
{
    Function * func = module->getCurrentFunction();

    // 函数内定义
    if (func) {
        for (auto child: node->sons) {
            int c = calcDims(child);
            Value * val = module->newVarValue(child->type, child->name);
            if (!val) {
                minic_log(LOG_ERROR, "变量(%s)声明失败，可能被隐藏或重复", child->name.c_str());
                continue;
            }
            child->val = val;
            if (c == -1) {
                continue;
            }
            ast_node * s = child->sons[c];
            if (s->node_type == ASTOP(ARRAY_INIT)) {
                // 数组初始化 - 使用新的一维偏移方式
                ArrayType * arrayType = (ArrayType *) child->type;

                // 处理第一维度缺失的情况
                if (arrayType->getNumElements() == 0) {
                    arrayType->setNumElements(s->sons.size());
                }

                // 使用新的数组初始化处理函数
                uint32_t currentOffset = 0;
                processArrayInitialization(val, arrayType, s, node->blockInsts, currentOffset);
            } else {
                // 非数组初始化
                int v;
                if (s->isConst && calcConstExpr(s, &v)) {
                    Constant * cst = s->node_type == ASTOP(LEAF_LITERAL_INT)
                                    ? (Constant*) module->newConstInt(v)
                                    : (Constant*) module->newConstFloat(v);
                    module->setVal(val, cst);
                    s->val = cst;
                    //fprintf(stderr, "//const %s\n", val->getName().c_str());
                }
                CHECK_NODE(s, s);
                node->blockInsts.addInst(s->blockInsts);
                Type * tv = val->getType();
                Type * ts = s->val->getType();
                Instruction * inst = nullptr;
                if (tv != ts) {
                    if (tv->isFloatType() && ts->isIntegerType()) {
                        inst = new CastInstruction(func, s->val, tv, CastInstruction::INT_TO_FLOAT);
                    } else if (tv->isIntegerType() && ts->isFloatType()) {
                        inst = new CastInstruction(func, s->val, tv, CastInstruction::FLOAT_TO_INT);
                    }
                    node->blockInsts.addInst(inst);
                }
                inst = new MoveInstruction(func, val, inst!=nullptr ? inst : s->val);
                node->blockInsts.addInst(inst);
            }
        }
    } else
        for (auto child: node->sons) {
            int c = calcDims(child);

            child->val = module->newVarValue(child->type, child->name);

            if (c == -1) continue;

            if (child->isConst && !child->type->isArrayType()) {
                // 只支持常量初始化
                ast_node * s = child->sons[c];
                int v = 0;
                if (calcConstExpr(s, &v)) {
                    // 隐式转换
                    if (s->node_type == ASTOP(LEAF_LITERAL_FLOAT) && child->type->isIntegerType()) {
                        s->node_type = ASTOP(LEAF_LITERAL_INT);
                        s->integer_val = s->float_val;
                    } else if (s->node_type == ASTOP(LEAF_LITERAL_INT) && child->type->isFloatType()) {
                        s->node_type = ASTOP(LEAF_LITERAL_FLOAT);
                        s->float_val = s->integer_val;
                    }
                    Constant * cint = s->node_type == ASTOP(LEAF_LITERAL_FLOAT)
                        ? (Constant*)module->newConstFloat(s->float_val)
                        : (Constant*)module->newConstInt(s->integer_val);
                    module->setVal(child->val, cint);
                }
            }

            ast_node * CHECK_NODE(s, child->sons[c]);
            if (Instanceof(cexp, ConstInt *, s->val)) {
                Instanceof(gVal, GlobalVariable *, child->val);
                gVal->intVal = new int(cexp->getVal());
            } else if (Instanceof(cexp, ConstFloat *, s->val)) {
                Instanceof(gVal, GlobalVariable *, child->val);
                gVal->floatVal = new float(cexp->getVal());
            } else if (child->type->isArrayType()) {
                vec = new std::vector<int32_t>();
                vec->push_back(0);
                bool hasVar =
                    array_init((ArrayType *) child->type, s->sons.begin(), s->sons.end(), init_global_array);
                if (!hasVar) {
                    vec->data()[0] = vec->size() - 1;
                    ((GlobalVariable *) child->val)->intVal = vec->data();
                }
            }
        }
    return true;
}

/// @brief 分支语句
bool IRGenerator::ir_branch(ast_node * node)
{
    ast_node * CHECK_NODE(cond, node->sons[0]);
    ast_node * CHECK_NODE(if1, node->sons[1]);
    ast_node * if0 = nullptr;
    if (node->sons.size() == 3) { // 有 else
        CHECK_NODE(if0, node->sons[2]);
    }

    node->blockInsts.addInst(cond->blockInsts);
    auto func = module->getCurrentFunction();
    auto lab1 = new LabelInstruction(func);
    auto lab0 = new LabelInstruction(func);
    backPatch(cond->truelist, lab1);
    backPatch(cond->falselist, lab0);
    cond->truelist->clear();
    cond->falselist->clear();

    // if0:
    if (if0) {
        node->blockInsts.addInst(lab0);
        node->blockInsts.addInst(if0->blockInsts);
        lab0 = new LabelInstruction(func);
        // br exit
        node->blockInsts.addInst(new GotoInstruction(func, lab0));
    }

    node->blockInsts.addInst(lab1);
    node->blockInsts.addInst(if1->blockInsts);
    // if0:
    node->blockInsts.addInst(lab0);

    return true;
}

/**
 * @brief 循环语句
 * 将while的条件语句放在循环体后面，在入口前加入一个goto语句进入到条件语句处，回填处
 * 理更方便，且与do-while语句兼容。
 */
bool IRGenerator::ir_loop(ast_node * node)
{
    int isWhile = node->node_type == ast_operator_type::AST_OP_WHILE;
    ast_node * CHECK_NODE(cond, node->sons[!isWhile]);

    auto func = module->getCurrentFunction();
    auto entry = new LabelInstruction(func);
    backPatch(cond->truelist, entry);
    cond->truelist->clear();

    db mlabs;

    LabelInstruction * expr = nullptr;
    if (isWhile) { // 添加goto跳转到条件式
        expr = new LabelInstruction(func);
        node->blockInsts.addInst(new GotoInstruction(func, expr));
        mlabs.a = expr;
    } else
        mlabs.a = entry;

    auto ext = new LabelInstruction(func);
    mlabs.b = ext;
    SLIST_INSERT_HEAD(&labs, &mlabs, entries);
    ast_node * CHECK_NODE(body, node->sons[isWhile]);
    SLIST_REMOVE_HEAD(&labs, entries);

    node->blockInsts.addInst(entry);
    node->blockInsts.addInst(body->blockInsts);
    if (expr)
        node->blockInsts.addInst(expr);
    backPatch(cond->falselist, ext);
    cond->falselist->clear();
    node->blockInsts.addInst(cond->blockInsts);

    node->blockInsts.addInst(ext);
    return true;
}

bool IRGenerator::ir_or(ast_node * node)
{
    ast_node * CHECK_NODE(left, node->sons[0]);
    ast_node * CHECK_NODE(right, node->sons[1]);

    auto flab = new LabelInstruction(module->getCurrentFunction());
    backPatch(left->falselist, flab);
    node->truelist = merge(left->truelist, right->truelist);
    node->falselist = right->falselist;
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(flab);
    node->blockInsts.addInst(right->blockInsts);
    return true;
}

bool IRGenerator::ir_and(ast_node * node)
{
    ast_node * CHECK_NODE(left, node->sons[0]);
    ast_node * CHECK_NODE(right, node->sons[1]);

    auto tlab = new LabelInstruction(module->getCurrentFunction());
    backPatch(left->truelist, tlab);
    node->falselist = merge(left->falselist, right->falselist);
    node->truelist = right->truelist;
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(tlab);
    node->blockInsts.addInst(right->blockInsts);

    return true;
}

bool IRGenerator::ir_not(ast_node * node)
{
    /* Sys2022的NOT操作只针对算术表达式，不针对关系表达式，
     * 构建AST时已将关系表达式内所有的NOT节点转为==0操作，此处只考虑算术表达式内
     */
    ast_node * CHECK_NODE(kid, node->sons[0]);
    node->blockInsts.addInst(kid->blockInsts);

    // TODO float cmp
    auto func = module->getCurrentFunction();
    Value * v = kid->val;
    if (v->getType() != IntegerType::getTypeBool()) {
        v = new BinaryInstruction(func, IROP(INE), kid->val, module->newConstInt(0), IntegerType::getTypeBool());
        node->blockInsts.addInst((Instruction *) v);
    }
    auto i = new BinaryInstruction(func, IROP(XOR), v, module->newConstInt(1), IntegerType::getTypeBool());
    node->blockInsts.addInst(i);
    node->val = i;
    return true;
}

bool IRGenerator::ir_jump(ast_node * node)
{
    if (SLIST_EMPTY(&labs))
        return false;
    db * d = SLIST_FIRST(&labs);
    node->blockInsts.addInst(
        new GotoInstruction(module->getCurrentFunction(), node->node_type == ASTOP(BREAK) ? d->b : d->a));
    return true;
}

IRInstOperator irtype(ast_operator_type type)
{
    switch (type) {
        case ASTOP(ADD):
            return IROP(IADD);
        case ASTOP(SUB):
            return IROP(ISUB);
        case ASTOP(MUL):
            return IROP(IMUL);
        case ASTOP(DIV):
            return IROP(IDIV);
        case ASTOP(MOD):
            return IROP(IMOD);
        case ASTOP(EQ):
            return IROP(IEQ);
        case ASTOP(NE):
            return IROP(INE);
        case ASTOP(GT):
            return IROP(IGT);
        case ASTOP(GE):
            return IROP(IGE);
        case ASTOP(LT):
            return IROP(ILT);
        case ASTOP(LE):
            return IROP(ILE);
        default:
            return IROP(MAX);
    }
}

std::vector<LabelInstruction **> * merge(std::vector<LabelInstruction **> * a, std::vector<LabelInstruction **> * b)
{
    a->insert(a->end(), b->begin(), b->end());
    b->clear();
    return a;
}

/// @brief 数组访问节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_array_access(ast_node * node)
{
    ast_node * arrayNameNode = node->sons[0];
    ast_node * indicesNode = node->sons[1];

    // 解析数组名
    if (!ir_node_var_id(arrayNameNode)) {
        return false;
    }

    // 获取数组变量
    Value * arrayVal = arrayNameNode->val;
    if (!arrayVal || !arrayVal->getType()->isArrayType()) {
        return false;
    }

    const Type * tp = arrayVal->getType();
    Function * func = module->getCurrentFunction();

    for (auto indexNode: indicesNode->sons) {
        if (!tp->isArrayType()) {
            // 维度过多
            return false;
        }

        ast_node * CHECK_NODE(indexExpr, indexNode);
        // TODO 数组越界检查
        node->blockInsts.addInst(indexExpr->blockInsts);
        Instruction * getptr = new BinaryInstruction(func, IRINST_OP_GEP, arrayVal, indexExpr->val, (Type *) tp);
        arrayVal = getptr;
        node->blockInsts.addInst(getptr);

        tp = ((ArrayType *) tp)->getElementType();
    }
    node->val = arrayVal;
    node->type = (Type *) tp;

    return true;
}

bool IRGenerator::ir_lval_to_r(ast_node * node)
{
    ast_node * CHECK_NODE(s, node->sons[0]);

    node->blockInsts.addInst(s->blockInsts);
    Type * tp = s->type;
    Value * v = s->val;
    if (s->node_type == ASTOP(ARRAY_ACCESS) && !tp->isArrayType()) {
        //fprintf(stderr, "%s \n", tp->toString().c_str());
        auto func = module->getCurrentFunction();
        auto ldr = new LoadInstruction(func, v, tp);
        node->blockInsts.addInst(ldr);
        v = ldr;
    }
    node->type = tp;
    node->val = v;
    return true;
}

/// @brief 数组初始化节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_array_init(ast_node * node)
{
    // 递归处理嵌套的初始化列表
    for (auto child: node->sons) {
        if (child->node_type == ASTOP(ARRAY_INIT)) {
            ir_array_init(child);
        }
    }

    return true;
}

/**
 * @brief 列表展平初始化
 * @param node 处理节点
 * @param type 初始化对应的数组类型
 * @param begin end array_init的子列表
 * @param process 每个解析单元逐个处理，参数为当前解析的节点
 * @return 是否包含变量元素
 */

bool IRGenerator::array_init(ArrayType * type,
                             std::vector<ast_node *>::const_iterator begin,
                             std::vector<ast_node *>::const_iterator end,
                             void (*process)(ast_node *))
{
    const Type * tp = type->getElementType();
    bool hasVariable = false;
    if (tp->isArrayType()) {
        // 多维数组需按维度展开
        Instanceof(arrTp, const ArrayType *, tp);
        uint32_t n = arrTp->getNumElements();
        uint32_t dim = 0;
        for (auto iter = begin; iter < end;) {
            ast_node * child = *iter;
            bool ret;
            if (child->node_type == ASTOP(ARRAY_INIT)) {
                auto & sons = child->sons;
                ret = array_init((ArrayType *) arrTp, sons.begin(), sons.end(), process);
                iter++;
            } else {
                auto iend = std::min(iter + n, end);
                ret = array_init((ArrayType *) arrTp, iter, iend, process);
                iter += n;
            }
            if (ret)
                hasVariable = true;
            dim++;
        }
        if (type->getNumElements() == 0)
            type->setNumElements(dim);
    } else {
        // 一维数组直接元素
        uint32_t n = type->getNumElements(), i = 0;
        if (n == 0) {
            n = end - begin;
            type->setNumElements(n);
        }
        ast_node * cnst = tp->isFloatType() ? ast_node::fzero() : ast_node::izero();
        auto * func = module->getCurrentFunction();
        for (; begin < end && i < n; begin++, i++) {
            // 仅处理第一个元素
            auto x = begin, e = end;
            while (x < e && (*x)->node_type == ASTOP(ARRAY_INIT)) {
                auto & sons = (*x)->sons;
                x = sons.begin();
                e = sons.end();
            }
            ast_node * vl = cnst;
            if (x < e) {
                CHECK_NODE(vl, *x);
                // node->blockInsts.addInst(xx->blockInsts);
                if (vl->node_type != ASTOP(LEAF_LITERAL_INT) && vl->node_type != ASTOP(LEAF_LITERAL_FLOAT))
                    hasVariable = true;
                Type * rtype = vl->type;
                if (rtype != tp) {
                    // 类型转换
                    if (tp->isFloatType() && vl->node_type == ASTOP(LEAF_LITERAL_INT)) {
                        vl->node_type = ASTOP(LEAF_LITERAL_FLOAT);
                    } else if (tp->isIntegerType() && vl->node_type == ASTOP(LEAF_LITERAL_FLOAT)) {
                        vl->node_type = ASTOP(LEAF_LITERAL_INT);
                    } else if (func) {
                        auto * inst = new CastInstruction(func,
                                                          vl->val,
                                                          (Type *) tp,
                                                          tp->isFloatType() ? CastInstruction::INT_TO_FLOAT
                                                                            : CastInstruction::FLOAT_TO_INT);
                        vl->blockInsts.addInst(inst);
                        vl->val = inst;
                        vl->type = (Type *) tp;
                    }
                }
            }
            process(vl);
        }
        for (; i < n; i++)
            process(cnst);
    }
    return hasVariable;
}

/// @brief 计算数组总元素个数
/// @param arrayType 数组类型
/// @return 总元素个数
uint32_t IRGenerator::getTotalArrayElements(ArrayType * arrayType)
{
    uint32_t totalElements = arrayType->getNumElements();
    const Type * elementType = arrayType->getElementType();

    // 递归计算多维数组的总元素个数
    while (elementType && elementType->isArrayType()) {
        ArrayType * subArrayType = (ArrayType *) elementType;
        totalElements *= subArrayType->getNumElements();
        elementType = subArrayType->getElementType();
    }

    return totalElements;
}

typedef union {int i;float f;} uif;
/// @brief 计算常量表达式（只包含常量符号、字面量）
bool IRGenerator::calcConstExpr(ast_node * node, void * ret)
{
    uif a, b;
    ast_node * l, * r;
    switch (node->node_type) {
        case ASTOP(LEAF_LITERAL_INT):
            *(int *) ret = node->integer_val;
            return true;
        case ASTOP(LEAF_LITERAL_FLOAT):
            *(float *) ret = node->float_val;
            return true;
        case ASTOP(VAR_ID): {
            Value * v = module->findVarValue(node->name);
            if (Instanceof(cint, ConstInt *, module->getVal(v))) {
                *(int *) ret = node->integer_val = cint->getVal();
                node->node_type = ASTOP(LEAF_LITERAL_INT);
                return true;
            } else if (Instanceof(cfloat, ConstFloat *, module->getVal(v))) {
                *(float *) ret = node->float_val = cfloat->getVal();
                node->node_type = ASTOP(LEAF_LITERAL_FLOAT);
                return true;
            } else {
                Instanceof(glb, GlobalVariable *, v);
                if (glb && nullptr != glb->intVal) {
                    *(int *) ret = node->integer_val = *glb->intVal;
                    node->node_type = glb->getType()->isFloatType()
                        ? ASTOP(LEAF_LITERAL_FLOAT)
                        : ASTOP(LEAF_LITERAL_INT);
                    return true;
                }
            }
            return false;
        }
        case ASTOP(ADD): {
            l = node->sons[0]; r = node->sons[1];
            if (!(calcConstExpr(l, &a.i)
                && calcConstExpr(r, &b.i)))
                return false;
            bool li = l->node_type == ASTOP(LEAF_LITERAL_INT);
            bool lf = l->node_type == ASTOP(LEAF_LITERAL_FLOAT);
            bool ri = r->node_type == ASTOP(LEAF_LITERAL_INT);
            bool rf = r->node_type == ASTOP(LEAF_LITERAL_FLOAT);
            if (li && ri)
                *(int *) ret = a.i + b.i;
            else if (li && rf)
                *(float *) ret = a.i + b.f;
            else if (lf && ri)
                *(float *) ret = a.f + b.i;
            else
                *(float *) ret = a.f + b.f;
            node->integer_val = *(int *) ret;
            node->node_type = li && ri ? ASTOP(LEAF_LITERAL_INT) : ASTOP(LEAF_LITERAL_FLOAT);
            return true;
        }
        case ASTOP(SUB): {
            l = node->sons[0]; r = node->sons[1];
            if (!(calcConstExpr(l, &a.i)
                && calcConstExpr(r, &b.i)))
                return false;
            bool li = l->node_type == ASTOP(LEAF_LITERAL_INT),
            lf = l->node_type == ASTOP(LEAF_LITERAL_FLOAT),
            ri = r->node_type == ASTOP(LEAF_LITERAL_INT),
            rf = r->node_type == ASTOP(LEAF_LITERAL_FLOAT);
            if (li && ri)
                *(int *) ret = a.i - b.i;
            else if (li && rf)
                *(float *) ret = a.i - b.f;
            else if (lf && ri)
                *(float *) ret = a.f - b.i;
            else
                *(float *) ret = a.f - b.f;
            node->integer_val = *(int *) ret;
            node->node_type = li && ri ? ASTOP(LEAF_LITERAL_INT) : ASTOP(LEAF_LITERAL_FLOAT);
            return true;
        }
        case ASTOP(MUL): {
            l = node->sons[0]; r = node->sons[1];
            if (!(calcConstExpr(l, &a.i)
                && calcConstExpr(r, &b.i)))
                return false;
            bool li = l->node_type == ASTOP(LEAF_LITERAL_INT),
            lf = l->node_type == ASTOP(LEAF_LITERAL_FLOAT),
            ri = r->node_type == ASTOP(LEAF_LITERAL_INT),
            rf = r->node_type == ASTOP(LEAF_LITERAL_FLOAT);
            if (li && ri)
                *(int *) ret = a.i * b.i;
            else if (li && rf)
                *(float *) ret = a.i * b.f;
            else if (lf && ri)
                *(float *) ret = a.f * b.i;
            else
                *(float *) ret = a.f * b.f;
            node->integer_val = *(int *) ret;
            node->node_type = li && ri ? ASTOP(LEAF_LITERAL_INT) : ASTOP(LEAF_LITERAL_FLOAT);
            return true;
        }
        case ASTOP(DIV): {
            l = node->sons[0], r = node->sons[1];
            if (!(calcConstExpr(l, &a.i)
                && calcConstExpr(r, &b.i)))
                return false;
            bool li = l->node_type == ASTOP(LEAF_LITERAL_INT),
            lf = l->node_type == ASTOP(LEAF_LITERAL_FLOAT),
            ri = r->node_type == ASTOP(LEAF_LITERAL_INT),
            rf = r->node_type == ASTOP(LEAF_LITERAL_FLOAT);
            if (li && ri)
                *(int *) ret = a.i / b.i;
            else if (li && rf)
                *(float *) ret = a.i / b.f;
            else if (lf && ri)
                *(float *) ret = a.f / b.i;
            else
                *(float *) ret = a.f / b.f;
            node->integer_val = *(int *) ret;
            node->node_type = li && ri ? ASTOP(LEAF_LITERAL_INT) : ASTOP(LEAF_LITERAL_FLOAT);
            return true;
        }
        case ASTOP(MOD):
            if (!(calcConstExpr(node->sons[0], &a)
                && calcConstExpr(node->sons[1], &b)))
                return false;
            *(int *) ret = a.i % b.i;
            node->integer_val = *(int *) ret;
            node->node_type = ASTOP(LEAF_LITERAL_INT);
            return true;
        case ASTOP(L2R): {
            if (!calcConstExpr(node->sons[0], ret))
                return false;
            node->integer_val = node->sons[0]->integer_val;
            node->node_type = node->sons[0]->node_type;
            return true;
        }
        default:
            return false;
    }
}

/// @brief 计算维度
/// @return 值节点下标，-1无值节点
int IRGenerator::calcDims(ast_node * child)
{
    if (child->sons.empty())
        return -1;
    ast_node * firstNode = child->sons[0];
    if (firstNode->node_type == ASTOP(ARRAY_INDICES)) {
        // 计算数组各维度的值，支持常量表达式
        Type * btype = child->type;
        ArrayType * atype = (ArrayType *) btype;
        auto & x = firstNode->sons;
        for (auto dimExp = x.rbegin(); dimExp != x.rend(); ++dimExp) {
            int dim = 0;
            ir_visit_ast_node(*dimExp); // 支持表达式维度
            calcConstExpr(*dimExp, &dim);
            ArrayType * arr = new ArrayType(atype, dim);
            atype = arr;
        }
        child->type = atype;
        return child->sons.size() > 1 ? 1 : -1;
    }
    return 0;
}

/// @brief 计算多维数组索引的一维偏移
/// @param arrayType 数组类型
/// @param indices 多维索引值列表
/// @return 一维偏移
uint32_t IRGenerator::calculateMultiDimOffset(ArrayType * arrayType, const std::vector<uint32_t> & indices)
{
    std::vector<uint32_t> dimensions;
    ArrayType * currentType = arrayType;

    // 收集所有维度大小
    while (currentType) {
        dimensions.push_back(currentType->getNumElements());
        const Type * elementType = currentType->getElementType();
        if (elementType->isArrayType()) {
            currentType = (ArrayType *) elementType;
        } else {
            break;
        }
    }

    uint32_t offset = 0;
    uint32_t multiplier = 1;

    // 从最后一维开始向前计算 offset = i0×(d1×d2×d3) + i1×(d2×d3) + i2×d3 + i3
    for (int i = dimensions.size() - 1; i >= 0; i--) {
        if (i < (int) indices.size()) {
            offset += indices[i] * multiplier;
        }
        if (i > 0) {
            multiplier *= dimensions[i];
        }
    }

    return offset;
}

/// @brief 获取多维数组的底层数组类型（一维表示）
/// @param arrayType 多维数组类型
/// @return 底层一维数组类型
ArrayType * IRGenerator::getBaseArrayType(ArrayType * arrayType)
{
    // 获取最内层的元素类型
    const Type * baseElementType = arrayType;
    while (baseElementType->isArrayType()) {
        baseElementType = ((ArrayType *) baseElementType)->getElementType();
    }

    // 计算总元素个数
    uint32_t totalElements = getTotalElements(arrayType);

    // 创建一维数组类型，如 [16 x i32]
    return new ArrayType(baseElementType, totalElements);
}

/// @brief 计算数组总元素个数
/// @param arrayType 数组类型
/// @return 总元素个数
uint32_t IRGenerator::getTotalElements(ArrayType * arrayType)
{
    uint32_t totalElements = arrayType->getNumElements();
    const Type * elementType = arrayType->getElementType();

    while (elementType && elementType->isArrayType()) {
        ArrayType * subArrayType = (ArrayType *) elementType;
        totalElements *= subArrayType->getNumElements();
        elementType = subArrayType->getElementType();
    }

    return totalElements;
}

/// @brief 处理数组初始化，计算每个元素的一维偏移
/// @param arrayVal 数组变量
/// @param arrayType 数组类型
/// @param initList 初始化列表节点
/// @param func 当前函数
/// @param blockInsts 指令块
/// @param currentOffset 当前偏移（引用，会被修改）
void IRGenerator::processArrayInitialization(Value * arrayVal,
                                             ArrayType * arrayType,
                                             ast_node * initList,
                                             InterCode & blockInsts,
                                             uint32_t & currentOffset)
{
    if (!initList) {
        return;
    }

    // 1. 计算维度信息
    std::vector<uint32_t> dimensions = getDimensions(arrayType);
    std::vector<uint32_t> dimensionsCnt = calculateDimensionsCnt(dimensions);

    // 2. 计算总元素数和非零元素数
    uint32_t totalElements = getTotalElements(arrayType);
    uint32_t nonZeroCount = countNonZeroElements(initList);

    // 3. 稀疏数组优化：如果非零元素少于50%，使用memset清零
    bool useMemset = ((nonZeroCount << 1) < totalElements) && (totalElements > 4);

    if (useMemset) {
        // 生成memset调用清零整个数组
        uint32_t arraySize = totalElements * IntegerType::getTypeInt()->getSize();
        Instruction * memsetCall =
            new FuncCallInstruction(module->getCurrentFunction(),
                                    module->findFunction("memset"),
                                    {arrayVal, module->newConstInt(0), module->newConstInt(arraySize)},
                                    VoidType::getType());
        blockInsts.addInst(memsetCall);
        module->getCurrentFunction()->setExistFuncCall(true);
    }

    // 4. 处理原始嵌套结构，保持维度边界
    currentOffset = 0;
    processInitListWithOffset(initList, dimensions, dimensionsCnt, currentOffset, arrayVal, blockInsts, useMemset, 0);

    // 5. 扁平化初始化列表（注释掉，我们已经在上面处理过了）
    // ir_array_init(initList);
}

/// @brief 计算数组维度信息
/// @param arrayType 数组类型
/// @return 维度列表
std::vector<uint32_t> IRGenerator::getDimensions(ArrayType * arrayType)
{
    std::vector<uint32_t> dimensions;
    ArrayType * currentType = arrayType;

    while (currentType) {
        dimensions.push_back(currentType->getNumElements());
        const Type * elementType = currentType->getElementType();
        if (elementType->isArrayType()) {
            currentType = (ArrayType *) elementType;
        } else {
            break;
        }
    }

    return dimensions;
}

/// @brief 计算维度累积信息
/// @param dimensions 维度列表
/// @return 累积信息列表
std::vector<uint32_t> IRGenerator::calculateDimensionsCnt(const std::vector<uint32_t> & dimensions)
{
    std::vector<uint32_t> dimensionsCnt(dimensions.size());

    // 计算每个维度对应的步长
    // 对于[2][2]，dimensionsCnt应该是[2, 1]
    // 对于[2][3][4]，dimensionsCnt应该是[12, 4, 1]
    for (int i = dimensions.size() - 1; i >= 0; i--) {
        if (i == dimensions.size() - 1) {
            dimensionsCnt[i] = 1; // 最后一维步长为1
        } else {
            dimensionsCnt[i] = dimensionsCnt[i + 1] * dimensions[i + 1];
        }
    }

    return dimensionsCnt;
}

/// @brief 统计初始化列表中的非零元素数量
/// @param initList 初始化列表
/// @return 非零元素数量
uint32_t IRGenerator::countNonZeroElements(ast_node * initList)
{
    if (!initList || initList->sons.empty()) {
        return 0;
    }

    uint32_t count = 0;
    for (ast_node * item: initList->sons) {
        if (item->node_type == ASTOP(ARRAY_INIT)) {
            // 递归统计嵌套列表
            count += countNonZeroElements(item);
        } else {
            // 检查是否为非零值
            if (!isZeroValue(item)) {
                count++;
            }
        }
    }

    return count;
}

/// @brief 检查是否为零值
/// @param node AST节点
/// @return 是否为零值
bool IRGenerator::isZeroValue(ast_node * node)
{
    if (!node)
        return true;

    switch (node->node_type) {
        case ASTOP(LEAF_LITERAL_INT):
            return node->integer_val == 0;
        case ASTOP(LEAF_LITERAL_FLOAT):
            return node->float_val == 0.0f;
        default:
            // 对于复杂表达式，保守地认为不是零值
            return false;
    }
}

/// @brief 处理初始化列表的一维偏移
/// @param initList 初始化列表
/// @param dimensions 维度信息
/// @param dimensionsCnt 累积维度信息
/// @param currentOffset 当前偏移
/// @param arrayVal 数组变量
/// @param func 当前函数
/// @param blockInsts 指令块
/// @param usedMemset 是否已经使用memset清零
/// @param dimLevel 当前维度级别
void IRGenerator::processInitListWithOffset(ast_node * initList,
                                            const std::vector<uint32_t> & dimensions,
                                            const std::vector<uint32_t> & dimensionsCnt,
                                            uint32_t & currentOffset,
                                            Value * arrayVal,
                                            InterCode & blockInsts,
                                            bool usedMemset,
                                            int dimLevel)
{
    if (!initList) {
        return;
    }

    // 计算当前维度的步长
    uint32_t elementSize = (dimLevel < dimensionsCnt.size()) ? dimensionsCnt[dimLevel] : 1;

    ArrayType * flatType = getBaseArrayType((ArrayType *) arrayVal->getType());
    Type * elemType = (Type*)flatType->getElementType();

    Function * func = module->getCurrentFunction();
    for (size_t i = 0; i < initList->sons.size(); i++) {
        ast_node * item = initList->sons[i];

        if (item->node_type == ASTOP(ARRAY_INIT)) {
            uint32_t startOffset = currentOffset;
            processInitListWithOffset(item,
                                      dimensions,
                                      dimensionsCnt,
                                      currentOffset,
                                      arrayVal,
                                      blockInsts,
                                      usedMemset,
                                      dimLevel + 1);

            // 处理完一个嵌套列表后，需要跳跃到下一个同级位置
            // 计算这个嵌套列表应该占用的总大小
            uint32_t expectedOffset = startOffset + elementSize;

            // 如果实际处理的元素少于预期，需要补零
            if (usedMemset)
                currentOffset = startOffset + elementSize;
            else
                for (; currentOffset < expectedOffset; currentOffset++) {
                    Instruction * ptr = new BinaryInstruction(func,
                                                              IRINST_OP_GEP,
                                                              arrayVal,
                                                              module->newConstInt(currentOffset),
                                                              flatType);
                    blockInsts.addInst(ptr);

                    // 存储值
                    blockInsts.addInst(new StoreInstruction(func, ptr, module->newConstInt(0)));
                }
        } else {
            // 叶子节点，生成存储指令
            if (!ir_visit_ast_node(item)) {
                currentOffset++;
                continue;
            }

            blockInsts.addInst(item->blockInsts);

            if (!(isZeroValue(item) && usedMemset)) {
                // 使用一维偏移，创建GEP指令
                // 这里使用扁平化的数组类型进行GEP
                Instruction * ptr =
                    new BinaryInstruction(func, IRINST_OP_GEP, arrayVal, module->newConstInt(currentOffset), flatType);
                blockInsts.addInst(ptr);
                Value * vl = item->val;
                if (elemType != vl->getType()) {
                    if (elemType->isFloatType() && dynamic_cast<ConstInt*>(vl)) {
                        vl = module->newConstFloat(((ConstInt*)vl)->getVal());
                    } else if (elemType->isIntegerType() && dynamic_cast<ConstFloat*>(vl)) {
                        vl = module->newConstInt(((ConstFloat*)vl)->getVal());
                    } else {
                        auto * inst = new CastInstruction(func,
                                                          vl,
                                                          elemType,
                                                          elemType->isFloatType() ? CastInstruction::INT_TO_FLOAT
                                                                            : CastInstruction::FLOAT_TO_INT);
                        blockInsts.addInst(inst);
                        vl = inst;
                    }
                }
                // 存储值
                blockInsts.addInst(new StoreInstruction(func, ptr, vl));
            }
            currentOffset++;
        }
    }
}
