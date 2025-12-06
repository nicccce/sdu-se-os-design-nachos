// exception.cc 
//	从用户程序进入Nachos内核的入口点。
//	有两种情况会导致控制从用户代码转移回这里：
//
//	系统调用 -- 用户代码显式请求调用Nachos内核中的过程。
//	目前，我们支持的唯一功能是"Halt"。
//
//	异常 -- 用户代码做了CPU无法处理的事情。
//	例如，访问不存在的内存、算术错误等。
//
//	中断（也可以导致控制从用户代码转移到Nachos内核）在其他地方处理。
//
// 目前，这仅处理Halt()系统调用。
// 其他所有情况都会导致核心转储。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	进入Nachos内核的入口点。当用户程序执行时，无论是执行系统调用，
//	还是生成地址或算术异常，都会调用此函数。
//
// 	对于系统调用，以下是调用约定：
//
// 	系统调用代码 -- r2寄存器
//		参数1 -- r4寄存器
//		参数2 -- r5寄存器
//		参数3 -- r6寄存器
//		参数4 -- r7寄存器
//
//	系统调用的结果（如果有的话）必须放回r2寄存器。
//
// 返回前不要忘记增加pc。否则你会陷入无限循环，反复执行同一个系统调用！
//
//	"which"是异常类型。可能的异常列表在machine.h中。
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    // 从r2寄存器读取系统调用类型
    int type = machine->ReadRegister(2);

    // 检查是否为系统调用异常且调用的是SC_Halt
    if ((which == SyscallException) && (type == SC_Halt)) {
    	// 输出调试信息：用户程序发起的关机
		DEBUG('a', "Shutdown, initiated by user program.\n");
		// 调用中断系统停止系统
    	interrupt->Halt();
    } else {
    	// 输出意外的用户模式异常信息
		printf("Unexpected user mode exception %d %d\n", which, type);
		// 断言失败，程序终止（核心转储）
		ASSERT(FALSE);
    }
}