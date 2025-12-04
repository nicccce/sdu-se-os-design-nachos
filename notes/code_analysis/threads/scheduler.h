// scheduler.h 
//	线程调度器和分发器的数据结构。
//	主要是准备运行的线程列表。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// 以下类定义了调度器/分发器抽象 --
// 跟踪哪个线程正在运行以及哪些线程已准备就绪但未运行所需的
// 数据结构和操作。

class Scheduler {
  public:
    Scheduler();			// 初始化就绪线程列表 
    ~Scheduler();			// 释放就绪列表

    void ReadyToRun(Thread* thread);	// 线程可以被调度。
    Thread* FindNextToRun();		// 从就绪列表中出队第一个线程
					// 如果存在，则返回线程。
    void Run(Thread* nextThread);	// 使 nextThread 开始运行
    void Print();			// 打印就绪列表的内容
    
  private:
    List *readyList;  		// 准备运行但未运行的线程队列
};

#endif // SCHEDULER_H