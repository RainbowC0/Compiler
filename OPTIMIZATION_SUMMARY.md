# 编译器优化功能总结

## 概述

我为这个编译器项目添加了以下优化功能：

1. **SSA转换** (Static Single Assignment)
2. **死代码删除** (Dead Code Elimination)
3. **常量传播** (Constant Propagation)
4. **强度消减** (Strength Reduction)

## 实现的功能

### 1. SSA转换
- 位置：`ir/Function.cpp` 中的 `convertToSSA()` 方法
- 功能：将代码转换为SSA形式，每个变量只被赋值一次
- 实现：
  - 计算支配关系 (Dominance)
  - 计算支配边界 (Dominance Frontier)
  - 插入Phi函数
  - 变量重命名

### 2. 死代码删除
- 位置：`ir/Function.cpp` 中的 `deadCodeElimination()` 方法
- 功能：删除不会影响程序结果的代码
- 实现：
  - 识别有副作用的指令（如函数调用、存储操作）
  - 检查指令结果是否被使用
  - 迭代删除死代码直到没有变化

### 3. 常量传播
- 位置：`ir/Function.cpp` 中的 `constantPropagation()` 方法
- 功能：在编译时计算常量表达式
- 实现：
  - 识别常量赋值
  - 计算常量表达式
  - 替换变量引用为常量值

### 4. 强度消减
- 位置：`ir/Function.cpp` 中的 `strengthReduction()` 方法
- 功能：将昂贵的操作替换为更便宜的操作
- 实现：
  - 乘法转换为移位（当乘数是2的幂次时）
  - 除法转换为移位（当除数是2的幂次时）

## 使用方法

### 编译时启用优化
```bash
# 启用优化级别1
./compiler -S -I -O1 -o output.ir input.sy

# 不启用优化
./compiler -S -I -O0 -o output.ir input.sy
```

### 优化级别说明
- `-O0`: 不进行优化
- `-O1`: 启用基本优化（死代码删除、常量传播、强度消减）

## 测试用例

### 1. 常量传播测试
文件：`test/const_propagation_test.sy`
```c
int main()
{
    int a = 5;
    int b = 10;
    int c = a + b;  // 应该被优化为 c = 15
    int d = c * 2;  // 应该被优化为 d = 30
    return d;
}
```

### 2. 死代码删除测试
文件：`test/dead_code_test.sy`
```c
int main()
{
    int a = 5;
    int b = 10;
    int c = a + b;  // 这个会被使用
    int d = 100;    // 这个不会被使用，应该被删除
    int e = d + 50; // 这个也不会被使用，应该被删除
    return c;       // 只返回c
}
```

### 3. 综合优化测试
文件：`test/optimization_test.sy`
```c
int main()
{
    int a = 5;
    int b = 10;
    int c = 2;
    
    // 常量传播测试
    int d = a + b;  // 应该被优化为 d = 15
    int e = d * c;  // 应该被优化为 e = 30
    
    // 死代码删除测试
    int f = 100;
    int g = f + 50;  // g被定义但未使用，应该被删除
    
    // 强度消减测试
    int h = a * 8;   // 应该被优化为 h = a << 3
    int i = b / 4;   // 应该被优化为 i = b >> 2
    
    // 使用变量，防止被优化掉
    int result = d + e + h + i;
    
    return result;
}
```

## 技术细节

### 修改的文件
1. `ir/Function.h` - 添加优化方法声明
2. `ir/Function.cpp` - 实现优化算法
3. `ir/User.h` - 添加const版本的getOperand方法
4. `ir/User.cpp` - 实现const版本的getOperand方法
5. `ir/Instruction.h` - 添加getResultValue方法
6. `ir/Instruction.cpp` - 实现getResultValue方法
7. `ir/Instructions/MoveInstruction.h` - 添加获取操作数的方法
8. `ir/Instructions/MoveInstruction.cpp` - 实现获取操作数的方法
9. `ir/Instructions/BinaryInstruction.h` - 添加获取操作数的方法
10. `ir/Instructions/BinaryInstruction.cpp` - 实现获取操作数的方法
11. `ir/Values/ConstInt.h` - 添加getValue方法
12. `ir/Values/ConstFloat.h` - 添加getValue方法
13. `main.cpp` - 在编译流程中添加优化调用

### 优化流程
1. IR生成完成后
2. 检查优化级别（-O参数）
3. 如果优化级别 > 0，对每个函数执行优化
4. 优化顺序：死代码删除 → 常量传播 → 强度消减
5. SSA转换暂时注释掉，因为它会改变IR结构

## 注意事项

1. **SSA转换**：当前只是框架实现，完整的SSA转换需要更复杂的算法
2. **强度消减**：由于当前IR没有移位指令，实际的强度消减只是框架
3. **常量传播**：目前只支持整数常量的基本运算
4. **死代码删除**：能够识别基本的死代码，但对于复杂的控制流可能需要更精确的分析

## 未来改进

1. 实现完整的SSA转换算法
2. 添加更多强度消减模式
3. 支持浮点常量的常量传播
4. 实现更精确的死代码分析
5. 添加循环优化
6. 添加内联优化
7. 添加寄存器分配优化 