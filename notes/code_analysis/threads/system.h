// system.h 
//	Nachos中使用的所有全局变量都在这里定义。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

// 初始化和清理例程
extern void Initialize(int argc, char **argv); 	// 初始化，
						// 在其他任何操作之前调用
extern void Cleanup();				// 清理，在
						// Nachos完成时调用。

extern Thread *currentThread;			// 占用CPU的线程
extern Thread *threadToBeDestroyed;  		// 刚完成的线程
extern Scheduler *scheduler;			// 就绪列表
extern Interrupt *interrupt;			// 中断状态
extern Statistics *stats;			// 性能指标
extern Timer *timer;				// 硬件闹钟

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// 用户程序内存和寄存器
#endif

#ifdef FILESYS_NEEDED 		// FILESYS 或 FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H