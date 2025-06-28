# 短路计算实现文档

## 概述

短路计算（Short Circuit Evaluation）是逻辑运算的重要特性，确保在某些条件下不会计算所有操作数。本项目在多个阶段实现了短路计算：

1. **AST到IR转换阶段**：通过控制流实现短路计算
2. **IR优化阶段**：进一步优化短路计算

## 实现架构

### 1. AST到IR转换阶段

#### 文件位置
- `ir/Generator/IRGenerator.cpp`

#### 核心方法

##### `ir_and()` - 逻辑与短路实现
```cpp
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
```

**工作原理**：
1. 先计算左操作数
2. 如果左操作数为假，直接跳转到假分支（短路）
3. 否则继续计算右操作数

##### `ir_or()` - 逻辑或短路实现
```cpp
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
```

**工作原理**：
1. 先计算左操作数
2. 如果左操作数为真，直接跳转到真分支（短路）
3. 否则继续计算右操作数

### 2. IR优化阶段

#### 文件位置
- `ir/Function.cpp`

#### 核心方法

##### `shortCircuitOptimization()` - 短路计算优化
```cpp
void Function::shortCircuitOptimization()
{
    // 检测常量条件跳转
    // 将条件跳转替换为无条件跳转
}
```

**功能**：
- 检测常量条件跳转指令
- 将条件跳转替换为无条件跳转
- 删除不必要的条件判断

##### `shortCircuitExpressionOptimization()` - 短路表达式优化
```cpp
void Function::shortCircuitExpressionOptimization()
{
    // 优化常量关系表达式
    // 提前计算常量比较结果
}
```

**功能**：
- 优化常量关系表达式
- 提前计算常量比较结果
- 减少运行时计算

### 3. 语法分析阶段

#### 文件位置
- `frontend/flexbison/MiniC.y`

#### 核心函数

##### `adjustCond()` - 条件表达式调整
```cpp
ast_node* adjustCond(ast_node *node) {
    // 转换条件 a -> a!=0，保证条件式内无NOT节点
    auto x = node->node_type;
    if (ASTOP(EQ)<=x && x<=ASTOP(LE)) return node;
    auto v = ast_node::New(digit_int_attr{0,yylineno});
    int tp = (int)ASTOP(NE);
    while (node->node_type==ASTOP(NOT)) {
        tp = (((int)ASTOP(EQ))^((int)ASTOP(NE)))^tp;
        node = node->sons[0];
    }
    return create_contain_node(ast_operator_type(tp), node, v);
}
```

**功能**：
- 将非关系表达式转换为关系表达式
- 处理NOT操作符
- 确保条件表达式的标准化

## 测试用例

### 基本测试
- `test/short_circuit_test.sy`：基本短路计算测试

### 高级测试
- `test/short_circuit_advanced_test.sy`：高级短路计算测试

### 测试场景
1. 基本短路计算：`a && b`，`a || b`
2. 复杂短路表达式：`(a && b) || (c && a)`
3. 嵌套短路：`a && (b || c)`
4. 循环中的短路：`while (i < 5 && (a || b))`
5. 条件表达式中的短路：`(a && b) ? 10 : 20`
6. 数组访问短路：`a && arr[10]`
7. 指针操作短路：`a && *ptr`

## 优化效果

### 1. 性能提升
- 避免不必要的计算
- 减少指令数量
- 提高执行效率

### 2. 安全性
- 防止数组越界访问
- 避免空指针解引用
- 减少运行时错误

### 3. 代码质量
- 编译时优化
- 生成更高效的代码
- 保持语义正确性

## 使用方法

### 编译时启用优化
```bash
# 启用优化级别1（包含短路优化）
./compiler -S -I -O1 -o output.ir input.sy

# 不启用优化
./compiler -S -I -O0 -o output.ir input.sy
```

### 测试短路计算
```bash
# 运行测试脚本
chmod +x test_short_circuit.sh
./test_short_circuit.sh
```

## 技术细节

### 控制流图（CFG）
短路计算通过修改控制流图实现：
- 使用Label指令标记跳转目标
- 使用Goto指令实现条件跳转
- 通过回填（backpatch）技术处理跳转目标

### 回填技术
```cpp
static inline void backPatch(std::vector<LabelInstruction **> * a, LabelInstruction * b)
{
    for (auto i: *a) {
        *i = b;
    }
}
```

### 真值表和假值表
- `truelist`：指向真分支的跳转指令列表
- `falselist`：指向假分支的跳转指令列表
- 通过合并和回填操作管理跳转目标

## 扩展性

### 支持的操作符
- 逻辑与：`&&`
- 逻辑或：`||`
- 关系运算符：`==`, `!=`, `>`, `>=`, `<`, `<=`

### 未来扩展
- 支持更多逻辑运算符
- 支持位运算的短路计算
- 支持函数调用的短路计算
- 支持异常处理的短路计算

## 总结

短路计算是本编译器的重要特性，通过多阶段的实现确保了：
1. **正确性**：保持C语言的语义
2. **效率**：避免不必要的计算
3. **安全性**：防止运行时错误
4. **可扩展性**：支持未来的功能扩展

该实现为编译器提供了强大的优化能力，是编译器优化的重要组成部分。 