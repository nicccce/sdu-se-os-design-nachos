// utility.h 
//	各种有用的定义，包括调试例程。
//
//	调试例程允许用户打开选定的
//	调试消息，可通过命令行参数控制
//	传递给Nachos (-d)。我们鼓励您添加自己的
//	调试标志。预定义的调试标志为：
//
//	'+' -- 打开所有调试消息
//   	't' -- 线程系统
//   	's' -- 信号量、锁和条件变量
//   	'i' -- 中断模拟
//   	'm' -- 机器模拟 (USER_PROGRAM)
//   	'd' -- 磁盘模拟 (FILESYS)
//   	'f' -- 文件系统 (FILESYS)
//   	'a' -- 地址空间 (USER_PROGRAM)
//   	'n' -- 网络模拟 (NETWORK)
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef UTILITY_H
#define UTILITY_H

#include "copyright.h"

#ifdef HOST_ALPHA		// 需要这个，因为gcc在DEC ALPHA架构上使用64位指针和
#define _int long		// 32位整数。
#else
#define _int int
#endif

// 各种有用的例程

#include <bool.h>
					 	// 布尔值。
						// 这与g++库中的定义
						// 相同。
/*
#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif

#define FALSE 0
#define TRUE  1

#define bool int		// 需要这个以避免bool类型已经定义时
				// 以及当布尔值分配给整数变量时
				// 出现的问题。
*/

#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define max(a,b)  (((a) > (b)) ? (a) : (b))

// 除法并向上或向下舍入
#define divRoundDown(n,s)  ((n) / (s))
#define divRoundUp(n,s)    (((n) / (s)) + ((((n) % (s)) > 0) ? 1 : 0))

// 这将类型"VoidFunctionPtr"声明为"指向
// 一个接受整数参数并返回空值的函数"的指针。使用
// 这样的函数指针（假设它是"func"），我们可以这样调用它：
//
//	(*func) (17);
//
// 这被Thread::Fork和中断处理程序使用，以及
// 其他几个地方。

typedef void (*VoidFunctionPtr)(_int arg); 
typedef void (*VoidNoArgFunctionPtr)(); 


// 包含隔离我们与主机机器系统库的接口。
// 需要bool和VoidFunctionPtr的定义
#include "sysdep.h"				

// 调试例程的接口。

extern void DebugInit(char* flags);	// 启用打印调试消息

extern bool DebugIsEnabled(char flag); 	// 此调试标志是否已启用？

extern void DEBUG (char flag, const char* format, ...);  	// 如果标志已启用则打印调试消息

//----------------------------------------------------------------------
// ASSERT
//      如果条件为假，打印消息并转储核心。
//	用于记录代码中的假设。
//
//	注意：需要是一个#define，以便能够打印
//	错误发生的位置。
//----------------------------------------------------------------------
#define ASSERT(condition)                                                     \
    if (!(condition)) {                                                       \
        fprintf(stderr, "断言失败: 第 %d 行, 文件 \"%s\"\n",           \
                __LINE__, __FILE__);                                          \
	fflush(stderr);							      \
        Abort();                                                              \
    }


#endif // UTILITY_H