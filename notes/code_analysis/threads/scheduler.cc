// scheduler.cc 
//	选择下一个要运行的线程并调度到该线程的例程。
//
// 	这些例程假设中断已经被禁用。
//	如果中断被禁用，我们可以假设互斥
//	（因为我们在单处理器上）。
//
// 	注意：我们不能使用锁来提供互斥，因为
// 	如果我们需要等待一个锁，而锁正忙，我们会
//	最终调用 FindNextToRun()，这会让我们陷入
//	无限循环。
//
// 	非常简单的实现 -- 没有优先级，直接使用先进先出（FIFO）。
//	可能需要在后续作业中改进。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	将就绪但未运行的线程列表初始化为空。
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List; 
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	释放就绪线程列表。
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	标记一个线程为就绪，但未运行。
//	将其放在就绪列表中，以便稍后调度到CPU上。
//
//	"thread" 是要放在就绪列表中的线程。
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);
    readyList->Append((void *)thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	返回下一个要调度到CPU的线程。
//	如果没有就绪线程，返回NULL。
// 副作用：
//	线程从就绪列表中移除。
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    return (Thread *)readyList->Remove();
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	将CPU分配给 nextThread。通过调用机器
//	相关的上下文切换例程 SWITCH 来保存旧线程的状态，
//	并加载新线程的状态。
//
//      注意：我们假设先前运行线程的状态
//	已经被从运行状态改为阻塞或就绪状态（根据情况）。
// 副作用：
//	全局变量 currentThread 变为 nextThread。
//
//	"nextThread" 是要放入CPU的线程。
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// 在运行用户程序之前忽略 
    if (currentThread->space != NULL) {	// 如果这个线程是用户程序，
        currentThread->SaveUserState(); // 保存用户的CPU寄存器
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // 检查旧线程是否
				    // 有未被检测到的栈溢出

    currentThread = nextThread;		    // 切换到下一个线程
    currentThread->setStatus(RUNNING);      // nextThread 现在正在运行
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // 这是一个在 switch.s 中定义的机器相关汇编语言例程。
    // 你可能需要思考一下接下来会发生什么，
    // 从线程的角度和"外部世界"的角度来看。

    SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // 如果旧线程因为完成而放弃处理器，
    // 我们需要删除它的骨架。注意我们不能
    // 在现在之前删除线程（例如，在 Thread::Finish() 中），
    // 因为到这一点为止，我们仍在旧线程的栈上运行！
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// 如果有一个地址空间
        currentThread->RestoreUserState();     // 需要恢复，就这样做。
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	打印调度器状态 -- 换句话说，就绪列表的内容。
//	用于调试。
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}