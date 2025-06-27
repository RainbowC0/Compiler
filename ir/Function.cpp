///
/// @file Function.cpp
/// @brief 函数实现
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

#include <cstdlib>
#include <string>
#include <set>
#include <map>
#include <algorithm>

#include "IRConstant.h"
#include "Function.h"
#include "MoveInstruction.h"
#include "BinaryInstruction.h"
#include "ConstInt.h"
#include "ConstFloat.h"

// 修复：添加必要的头文件以支持dynamic_cast
#include "Instructions/FuncCallInstruction.h"
#include "Instructions/GotoInstruction.h"

/// @brief 指定函数名字、函数类型的构造函数
/// @param _name 函数名称
/// @param _type 函数类型
/// @param _builtin 是否是内置函数
Function::Function(std::string _name, FunctionType * _type, bool _builtin)
    : GlobalValue(_type, _name), builtIn(_builtin)
{
    returnType = _type->getReturnType();

    // 设置对齐大小
    setAlignment(1);
}

///
/// @brief 析构函数
/// @brief 释放函数占用的内存和IR指令代码
/// @brief 注意：IR指令代码并未释放，需要手动释放
Function::~Function()
{
    Delete();
}

/// @brief 获取函数返回类型
/// @return 返回类型
Type * Function::getReturnType()
{
    return returnType;
}

/// @brief 获取函数的形参列表
/// @return 形参列表
std::vector<FormalParam *> & Function::getParams()
{
    return params;
}

/// @brief 获取函数内的IR指令代码
/// @return IR指令代码
InterCode & Function::getInterCode()
{
    return code;
}

/// @brief 判断该函数是否是内置函数
/// @return true: 内置函数，false：用户自定义
bool Function::isBuiltin()
{
    return builtIn;
}

/// @brief 函数指令信息输出
/// @param str 函数指令
void Function::toString(std::string & str)
{
    if (builtIn) {
        // 内置函数则什么都不输出
        return;
    }

    // 输出函数头
    str = "define " + getReturnType()->toString() + " " + getIRName() + "(";

    int i = 0, l = params.size();
    if (i < l)
        goto ENT;
    for (; i < l; i++) {
        str += ", ";
    ENT:
        FormalParam * param = params[i];
        str += param->getType()->toString() + " " + param->getIRName();
    }

    str += ") {\n";

    // 输出局部变量的名字与IR名字
    for (auto & var: this->varsVector) {

        // 局部变量和临时变量需要输出declare语句
        str += "\tdeclare " + var->getType()->toString() + " " + var->getIRName();

        // std::string extraStr;
        std::string realName = var->getName();
        if (!realName.empty()) {
            str += " ; " + std::to_string(var->getScopeLevel()) + ":" + realName;
        }

        str += "\n";
    }

    // 输出临时变量的declare形式
    // 遍历所有的线性IR指令，文本输出
    for (auto & inst: code.getInsts()) {

        if (inst->hasResultValue()) {

            // 局部变量和临时变量需要输出declare语句
            str += "\tdeclare " + inst->getType()->toString() + " " + inst->getIRName() + "\n";
        }
    }

    // 遍历所有的线性IR指令，文本输出
    for (auto & inst: code.getInsts()) {

        std::string instStr;
        inst->toString(instStr);

        if (!instStr.empty()) {

            // Label指令不加Tab键
            if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
                str += instStr + "\n";
            } else {
                str += "\t" + instStr + "\n";
            }
        }
    }

    // 输出函数尾部
    str += "}\n";
}

/// @brief 设置函数出口指令
/// @param inst 出口Label指令
void Function::setExitLabel(Instruction * inst)
{
    exitLabel = inst;
}

/// @brief 获取函数出口指令
/// @return 出口Label指令
Instruction * Function::getExitLabel()
{
    return exitLabel;
}

/// @brief 设置函数返回值变量
/// @param val 返回值变量，要求必须是局部变量，不能是临时变量
void Function::setReturnValue(LocalVariable * val)
{
    returnValue = val;
}

/// @brief 获取函数返回值变量
/// @return 返回值变量
LocalVariable * Function::getReturnValue()
{
    return returnValue;
}

/// @brief 获取最大栈帧深度
/// @return 栈帧深度
int Function::getMaxDep()
{
    return maxDepth;
}

/// @brief 设置最大栈帧深度
/// @param dep 栈帧深度
void Function::setMaxDep(int dep)
{
    maxDepth = dep;

    // 设置函数栈帧被重定位标记，用于生成不同的栈帧保护代码
    relocated = true;
}

/// @brief 获取本函数需要保护的寄存器
/// @return 要保护的寄存器
std::vector<int32_t> & Function::getProtectedReg()
{
    return protectedRegs;
}

/// @brief 获取本函数需要保护的寄存器字符串
/// @return 要保护的寄存器
std::string & Function::getProtectedRegStr()
{
    return protectedRegStr;
}

/// @brief 获取函数调用参数个数的最大值
/// @return 函数调用参数个数的最大值
int Function::getMaxFuncCallArgCnt()
{
    return maxFuncCallArgCnt;
}

/// @brief 设置函数调用参数个数的最大值
/// @param count 函数调用参数个数的最大值
void Function::setMaxFuncCallArgCnt(int count)
{
    maxFuncCallArgCnt = count;
}

/// @brief 函数内是否存在函数调用
/// @return 是否存在函调用
bool Function::getExistFuncCall()
{
    return funcCallExist;
}

/// @brief 设置函数是否存在函数调用
/// @param exist true: 存在 false: 不存在
void Function::setExistFuncCall(bool exist)
{
    funcCallExist = exist;
}

/// @brief 新建变量型Value。先检查是否存在，不存在则创建，否则失败
/// @param name 变量ID
/// @param type 变量类型
/// @param scope_level 局部变量的作用域层级
LocalVariable * Function::newLocalVarValue(Type * type, std::string name, int32_t scope_level)
{
    // 创建变量并加入符号表
    LocalVariable * varValue = new LocalVariable(type, name, scope_level);

    // varsVector表中可能存在变量重名的信息
    varsVector.push_back(varValue);

    return varValue;
}

/// @brief 新建一个内存型的Value，并加入到符号表，用于后续释放空间
/// \param type 变量类型
/// \return 临时变量Value
MemVariable * Function::newMemVariable(Type * type)
{
    // 肯定唯一存在，直接插入即可
    MemVariable * memValue = new MemVariable(type);

    memVector.push_back(memValue);

    return memValue;
}

/// @brief 清理函数内申请的资源
void Function::Delete()
{
    // 清理IR指令
    code.Delete();

    // 清理Value
    for (auto & var: varsVector) {
        delete var;
    }

    varsVector.clear();
}

///
/// @brief 函数内的Value重命名
///
void Function::renameIR()
{
    // 内置函数忽略
    if (isBuiltin()) {
        return;
    }

    int32_t nameIndex = 0;

    // 形式参数重命名
    for (auto & param: this->params) {
        param->setIRName(IR_TEMP_VARNAME_PREFIX + std::to_string(nameIndex++));
    }

    // 局部变量重命名
    for (auto & var: this->varsVector) {

        var->setIRName(IR_LOCAL_VARNAME_PREFIX + std::to_string(nameIndex++));
    }

    // 遍历所有的指令进行命名
    for (auto inst: this->getInterCode().getInsts()) {
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            inst->setIRName(IR_LABEL_PREFIX + std::to_string(nameIndex++));
        } else if (inst->hasResultValue()) {
            inst->setIRName(IR_TEMP_VARNAME_PREFIX + std::to_string(nameIndex++));
        }
    }
}

///
/// @brief 获取统计的ARG指令的个数
/// @return int32_t 个数
///
int32_t Function::getRealArgcount()
{
    return this->realArgCount;
}

///
/// @brief 用于统计ARG指令个数的自增函数，个数加1
///
void Function::realArgCountInc()
{
    this->realArgCount++;
}

///
/// @brief 用于统计ARG指令个数的清零
///
void Function::realArgCountReset()
{
    this->realArgCount = 0;
}

///
/// @brief 执行SSA转换
///
void Function::convertToSSA()
{
    // 内置函数跳过优化
    if (isBuiltin()) {
        return;
    }

    // 构建CFG
    CFG cfg;
    cfg.buildCFG(this);
    
    // 计算支配关系
    std::vector<std::set<int>> dom = computeDominance(cfg);
    
    // 计算支配边界
    std::vector<std::set<int>> df = computeDominanceFrontier(cfg, dom);
    
    // 插入phi函数
    insertPhiFunctions(cfg, df);
    
    // 重命名变量
    renameVariables(cfg);
}

///
/// @brief 执行死代码删除优化
///
void Function::deadCodeElimination()
{
    // 内置函数跳过优化
    if (isBuiltin()) {
        return;
    }

    auto& insts = code.getInsts();
    std::vector<bool> isDead(insts.size(), false);
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (size_t i = 0; i < insts.size(); i++) {
            Instruction* inst = insts[i];
            
            // 跳过已标记为死代码的指令
            if (isDead[i]) continue;
            
            // 跳过控制流指令
            if (inst->getOp() == IRINST_OP_LABEL || 
                inst->getOp() == IRINST_OP_GOTO ||
                inst->getOp() == IRINST_OP_ENTRY ||
                inst->getOp() == IRINST_OP_EXIT) {
                continue;
            }
            
            // 检查指令是否有副作用
            if (hasSideEffects(inst)) {
                continue;
            }
            
            // 检查结果是否被使用
            if (!isResultUsed(inst, i, insts)) {
                isDead[i] = true;
                changed = true;
            }
        }
    }
    
    // 删除死代码
    removeDeadCode(isDead);
}

///
/// @brief 执行常量传播优化
///
void Function::constantPropagation()
{
    // 内置函数跳过优化
    if (isBuiltin()) {
        return;
    }

    auto& insts = code.getInsts();
    std::map<Value*, Value*> constants;
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (auto inst : insts) {
            if (inst->getOp() == IRINST_OP_ASSIGN) {
                // 处理赋值指令
                auto assignInst = static_cast<MoveInstruction*>(inst);
                Value* src = assignInst->getSrcValue();
                Value* dest = assignInst->getResultValue();
                
                if (isConstant(src)) {
                    constants[dest] = src;
                    changed = true;
                }
            } else if (isBinaryOp(inst)) {
                // 处理二元运算指令
                auto binInst = static_cast<BinaryInstruction*>(inst);
                Value* src1 = binInst->getSrcValue1();
                Value* src2 = binInst->getSrcValue2();
                Value* dest = binInst->getResultValue();
                
                if (isConstant(src1) && isConstant(src2)) {
                    Value* result = evaluateConstant(src1, src2, inst->getOp());
                    if (result) {
                        constants[dest] = result;
                        changed = true;
                    }
                }
            }
        }
    }
    
    // 替换常量
    replaceConstants(constants);
}

///
/// @brief 执行强度消减优化
///
void Function::strengthReduction()
{
    // 内置函数跳过优化
    if (isBuiltin()) {
        return;
    }

    auto& insts = code.getInsts();
    
    for (auto inst : insts) {
        if (isBinaryOp(inst)) {
            auto binInst = static_cast<BinaryInstruction*>(inst);
            
            // 乘法转换为移位
            if (inst->getOp() == IRINST_OP_IMUL) {
                Value* src1 = binInst->getSrcValue1();
                Value* src2 = binInst->getSrcValue2();
                
                if (isPowerOfTwo(src2)) {
                    replaceWithShift(inst, src1, src2, true);
                }
            }
            
            // 除法转换为移位
            if (inst->getOp() == IRINST_OP_IDIV) {
                Value* src1 = binInst->getSrcValue1();
                Value* src2 = binInst->getSrcValue2();
                
                if (isPowerOfTwo(src2)) {
                    replaceWithShift(inst, src1, src2, false);
                }
            }
        }
    }
}

///
/// @brief 执行函数内联优化
///
void Function::inlineExpansion() {
    // 函数内联优化：遍历IR，找到可内联的FuncCallInstruction，
    // 将被调函数的IR插入到调用点，重命名变量，替换形参为实参。
    // 这里只提供基础框架，详细逻辑可后续完善。
    auto &insts = code.getInsts();
    for (size_t i = 0; i < insts.size(); ++i) {
        auto callInst = dynamic_cast<FuncCallInstruction*>(insts[i]);
        if (!callInst) continue;
        Function *callee = callInst->calledFunction;
        // 简单判断：只内联非递归、非内置、小体积函数
        if (!callee || callee == this || callee->isBuiltin()) continue;
        if (callee->getInterCode().getInsts().size() > 32) continue; // 体积阈值
        // TODO: 检查递归、全局副作用、内联深度等
        // TODO: 复制callee的IR，重命名变量，替换形参为实参，插入到当前IR
        // 这里只做提示，具体实现需变量映射、作用域处理
    }
}

///
/// @brief 执行分支剪枝优化
///
void Function::branchPruning() {
    // 分支剪枝优化：遍历IR，找到条件为常量的GotoInstruction，
    // 恒真/恒假时直接替换为无条件跳转。
    auto &insts = code.getInsts();
    for (size_t i = 0; i < insts.size(); ++i) {
        auto gotoInst = dynamic_cast<GotoInstruction*>(insts[i]);
        if (!gotoInst) continue;
        Value *cond = gotoInst->getCondiValue();
        auto cint = dynamic_cast<ConstInt*>(cond);
        if (!cint) continue;
        // 恒真/恒假
        if (cint->getVal()) {
            // 恒真，跳转到iftrue
            insts[i] = new GotoInstruction(this, gotoInst->iftrue);
        } else {
            // 恒假，跳转到iffalse
            insts[i] = new GotoInstruction(this, gotoInst->iffalse);
        }
        delete gotoInst;
    }
}

///
/// @brief 执行所有优化
///
void Function::optimize()
{
    // 内置函数跳过优化
    if (isBuiltin()) {
        return;
    }

    // 优化顺序：内联 → 分支剪枝 → 死代码删除 → 常量传播 → 强度消减
    inlineExpansion();
    branchPruning();
    deadCodeElimination();
    constantPropagation();
    strengthReduction();
    // SSA转换放在最后，因为它会改变IR结构
    // convertToSSA();
}

// 私有辅助方法实现

std::vector<std::set<int>> Function::computeDominance(CFG& cfg)
{
    // 简化的支配关系计算
    std::vector<std::set<int>> dom(cfg.inters.size());
    
    // 初始化：每个节点支配自己
    for (size_t i = 0; i < cfg.inters.size(); i++) {
        dom[i].insert(i);
    }
    
    // 迭代计算支配关系
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < cfg.inters.size(); i++) {
            std::set<int> newDom;
            newDom.insert(i);
            
            // 计算前驱节点的支配交集
            for (size_t j = 0; j < cfg.inters.size(); j++) {
                if (j != i && hasEdge(cfg, j, i)) {
                    if (newDom.empty()) {
                        newDom = dom[j];
                    } else {
                        std::set<int> intersection;
                        std::set_intersection(newDom.begin(), newDom.end(),
                                            dom[j].begin(), dom[j].end(),
                                            std::inserter(intersection, intersection.begin()));
                        newDom = intersection;
                    }
                }
            }
            newDom.insert(i);
            
            if (newDom != dom[i]) {
                dom[i] = newDom;
                changed = true;
            }
        }
    }
    
    return dom;
}

std::vector<std::set<int>> Function::computeDominanceFrontier(CFG& cfg, const std::vector<std::set<int>>& dom)
{
    std::vector<std::set<int>> df(cfg.inters.size());
    
    for (size_t i = 0; i < cfg.inters.size(); i++) {
        for (size_t j = 0; j < cfg.inters.size(); j++) {
            if (hasEdge(cfg, i, j) && !isDominatedBy(j, i, dom)) {
                df[i].insert(j);
            }
        }
    }
    
    return df;
}

void Function::insertPhiFunctions(CFG& cfg, const std::vector<std::set<int>>& df)
{
    // 简化的phi函数插入
    // 这里只是框架，实际实现需要更复杂的算法
    
    // 对于每个变量，在支配边界插入phi函数
    // 1. 识别所有变量定义
    // 2. 在支配边界插入phi函数
    // 3. 更新CFG结构
    
    // 由于当前IR没有phi指令，这里只是框架
    // 实际实现需要：
    // 1. 创建PhiInstruction类
    // 2. 在适当位置插入phi指令
    // 3. 更新控制流图
}

void Function::renameVariables(CFG& cfg)
{
    // 简化的变量重命名
    // 这里只是框架，实际实现需要更复杂的算法
    
    // 1. 为每个变量维护版本栈
    // 2. 深度优先遍历CFG
    // 3. 重命名变量定义和使用
    
    // 由于当前IR结构限制，这里只是框架
    // 实际实现需要：
    // 1. 维护变量版本映射
    // 2. 重命名所有变量引用
    // 3. 处理phi指令的特殊情况
}

bool Function::hasSideEffects(Instruction* inst)
{
    // 检查指令是否有副作用
    switch (inst->getOp()) {
        case IRINST_OP_STORE:
        case IRINST_OP_FUNC_CALL:
        case IRINST_OP_ENTRY:
        case IRINST_OP_EXIT:
            return true;
        default:
            return false;
    }
}

bool Function::isResultUsed(Instruction* inst, size_t pos, const std::vector<Instruction*>& insts)
{
    if (!inst->hasResultValue()) {
        return false;
    }
    
    Value* result = inst->getResultValue();
    if (!result) {
        return false;
    }
    
    // 检查后续指令是否使用这个结果
    for (size_t i = pos + 1; i < insts.size(); i++) {
        Instruction* laterInst = insts[i];
        
        // 检查操作数
        const auto& operands = laterInst->getOperands();
        for (auto& use : operands) {
            if (use->getUsee() == result) {
                return true;
            }
        }
    }
    
    return false;
}

void Function::removeDeadCode(const std::vector<bool>& isDead)
{
    auto& insts = code.getInsts();
    std::vector<Instruction*> newInsts;
    
    for (size_t i = 0; i < insts.size(); i++) {
        if (!isDead[i]) {
            newInsts.push_back(insts[i]);
        } else {
            delete insts[i];
        }
    }
    
    insts = newInsts;
}

bool Function::isConstant(Value* value)
{
    // 检查是否是常量
    return dynamic_cast<ConstInt*>(value) != nullptr || 
           dynamic_cast<ConstFloat*>(value) != nullptr;
}

bool Function::isBinaryOp(Instruction* inst)
{
    IRInstOperator op = inst->getOp();
    return (op >= IRINST_OP_IADD && op <= IRINST_OP_ILE) ||
           (op >= IRINST_OP_FADD && op <= IRINST_OP_FLE) ||
           op == IRINST_OP_XOR;
}

Value* Function::evaluateConstant(Value* src1, Value* src2, IRInstOperator op)
{
    // 简化的常量求值
    auto constInt1 = dynamic_cast<ConstInt*>(src1);
    auto constInt2 = dynamic_cast<ConstInt*>(src2);
    
    if (constInt1 && constInt2) {
        int val1 = constInt1->getValue();
        int val2 = constInt2->getValue();
        int result = 0;
        
        switch (op) {
            case IRINST_OP_IADD:
                result = val1 + val2;
                break;
            case IRINST_OP_ISUB:
                result = val1 - val2;
                break;
            case IRINST_OP_IMUL:
                result = val1 * val2;
                break;
            case IRINST_OP_IDIV:
                if (val2 != 0) {
                    result = val1 / val2;
                } else {
                    return nullptr; // 除零错误
                }
                break;
            case IRINST_OP_IMOD:
                if (val2 != 0) {
                    result = val1 % val2;
                } else {
                    return nullptr; // 除零错误
                }
                break;
            default:
                return nullptr; // 不支持的操作
        }
        
        return new ConstInt(result);
    }
    
    return nullptr;
}

void Function::replaceConstants(const std::map<Value*, Value*>& constants)
{
    auto& insts = code.getInsts();
    
    for (auto inst : insts) {
        // 替换操作数中的常量
        const auto& operands = inst->getOperands();
        for (auto& use : operands) {
            Value* operand = use->getUsee();
            auto it = constants.find(operand);
            if (it != constants.end()) {
                use->setUsee(it->second);
            }
        }
    }
}

bool Function::isPowerOfTwo(Value* value)
{
    // 检查是否是2的幂次
    if (auto constInt = dynamic_cast<ConstInt*>(value)) {
        int val = constInt->getValue();
        return val > 0 && (val & (val - 1)) == 0;
    }
    return false;
}

void Function::replaceWithShift(Instruction* inst, Value* src1, Value* src2, bool isMultiply)
{
    // 将乘法/除法替换为移位操作
    // 这里只是框架，实际实现需要创建新的移位指令
    
    if (auto constInt = dynamic_cast<ConstInt*>(src2)) {
        int val = constInt->getValue();
        int shift = 0;
        
        // 计算移位次数
        while (val > 1) {
            val >>= 1;
            shift++;
        }
        
        // 创建移位常量
        // ConstInt* shiftConst = new ConstInt(shift);  // 暂时注释掉，避免未使用变量警告
        
        // 这里应该创建新的移位指令来替换原来的指令
        // 由于当前IR没有移位指令，我们暂时保留原指令
        // 在实际实现中，需要：
        // 1. 创建新的移位指令
        // 2. 替换原指令
        // 3. 更新操作数
    }
}

bool Function::hasEdge(CFG& cfg, int from, int to)
{
    // 检查CFG中是否存在从from到to的边
    // 这里只是框架，实际实现需要更复杂的逻辑
    return false;
}

bool Function::isDominatedBy(int node, int dominator, const std::vector<std::set<int>>& dom)
{
    return dom[node].find(dominator) != dom[node].end();
}
