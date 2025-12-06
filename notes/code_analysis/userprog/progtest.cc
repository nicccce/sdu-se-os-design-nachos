// progtest.cc 
//	测试例程，用于演示Nachos可以加载和执行用户程序。
//
//	还包括测试控制台硬件设备的例程。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	运行一个用户程序。打开可执行文件，将其加载到内存中，然后跳转执行。
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    // 打开可执行文件
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    // 检查文件是否打开成功
    if (executable == NULL) {
    	// 输出错误信息
		printf("Unable to open file %s\n", filename);
		return;
    }
    // 创建地址空间对象，加载可执行文件
    space = new AddrSpace(executable);    
    // 将地址空间关联到当前线程
    currentThread->space = space;

    // 删除可执行文件对象（关闭文件）
    delete executable;			

    // 初始化寄存器值
    space->InitRegisters();		
    // 恢复地址空间状态（加载页表寄存器）
    space->RestoreState();		

    // 开始运行用户程序
    machine->Run();			
    // 断言：machine->Run永远不会返回；
    // 地址空间通过执行系统调用"exit"退出
    ASSERT(FALSE);			
}

// 控制台测试所需的数据结构。发出I/O请求的线程等待信号量，
// 直到I/O完成。

// 控制台对象
static Console *console;
// 读操作可用信号量
static Semaphore *readAvail;
// 写操作完成信号量
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	唤醒请求I/O的线程。
//----------------------------------------------------------------------

// 读操作可用中断处理函数
static void ReadAvail(_int arg) { readAvail->V(); }
// 写操作完成中断处理函数
static void WriteDone(_int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	通过将输入的字符回显到输出上来测试控制台。
//	当用户输入'q'时停止。
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    // 创建控制台对象，指定输入输出设备和中断处理函数
    console = new Console(in, out, ReadAvail, WriteDone, 0);
    // 创建读操作可用信号量，初始值为0
    readAvail = new Semaphore("read avail", 0);
    // 创建写操作完成信号量，初始值为0
    writeDone = new Semaphore("write done", 0);
    
    // 无限循环
    for (;;) {
    	// 等待字符到达
		readAvail->P();		
		// 从控制台获取字符
		ch = console->GetChar();
		// 将字符输出到控制台（回显）
		console->PutChar(ch);	
		// 等待写操作完成
		writeDone->P() ;        
		// 如果输入的是'q'，则退出
		if (ch == 'q') return;  
    }
}