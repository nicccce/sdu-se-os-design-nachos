#!/bin/bash
# Nachos调度器测试说明

echo "NACHOS 调度器测试说明"
echo "======================="

echo ""
echo "1. 基本线程测试 (默认):"
echo "   ./nachos"
echo "   - 这将运行默认的ThreadTest()函数"
echo "   - 创建两个线程并交替执行"

echo ""
echo "2. 优先级调度测试 (新添加):"
echo "   ./nachos -P"
echo "   - 运行PriorityTest()函数"
echo "   - 创建三个不同优先级的线程 (10, 50, 25)"
echo "   - 预期按优先级顺序执行 (数值小的优先级高)"

echo ""
echo "3. 复杂优先级测试 (新添加):"
echo "   ./nachos -C"
echo "   - 运行ComplexPriorityTest()函数"
echo "   - 测试运行时优先级变化"

echo ""
echo "4. 饥饿测试 (新添加):"
echo "   ./nachos -S"
echo "   - 运行StarvationTest()函数"
echo "   - 测试低优先级线程是否有机会执行"

echo ""
echo "注意：如果编译失败，可使用预先编译的版本进行测试"
echo "当前系统中已有编译好的nachos二进制文件"
echo "位置: ./nachos (链接到 arch/unknown-i386-linux/bin/nachos)"