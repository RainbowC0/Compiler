#!/bin/bash

echo "Testing Short Circuit Evaluation..."

# 编译测试文件
echo "Compiling test files..."

# 基本短路计算测试
echo "1. Testing basic short circuit..."
./compiler -S -I -O1 -o test1.ir test/short_circuit_test.sy
if [ $? -eq 0 ]; then
    echo "   ✓ Basic short circuit test compiled successfully"
else
    echo "   ✗ Basic short circuit test compilation failed"
fi

# 高级短路计算测试
echo "2. Testing advanced short circuit..."
./compiler -S -I -O1 -o test2.ir test/short_circuit_advanced_test.sy
if [ $? -eq 0 ]; then
    echo "   ✓ Advanced short circuit test compiled successfully"
else
    echo "   ✗ Advanced short circuit test compilation failed"
fi

# 优化测试
echo "3. Testing optimization..."
./compiler -S -I -O0 -o test3_no_opt.ir test/short_circuit_test.sy
./compiler -S -I -O1 -o test3_with_opt.ir test/short_circuit_test.sy

echo "Short circuit evaluation testing completed!"
echo ""
echo "Generated IR files:"
echo "  - test1.ir (basic short circuit)"
echo "  - test2.ir (advanced short circuit)"
echo "  - test3_no_opt.ir (without optimization)"
echo "  - test3_with_opt.ir (with optimization)" 