// synch.cc 
//	线程同步的例程。这里定义了三种
//	同步例程：信号量、锁
//   	和条件变量（后两种的实现
//	留给读者）。
//
// 任何同步例程的实现都需要一些
// 原始的原子操作。我们假设Nachos运行在
// 单处理器上，因此原子性可以通过
// 禁用中断来提供。当中断被禁用时，没有
// 上下文切换可以发生，因此当前线程被保证
// 在整个过程中持有CPU，直到中断被重新启用。
//
// 因为其中一些例程可能在中断
// 已经被禁用的情况下被调用（如Semaphore::V），我们不在原子操作
// 结束时开启中断，而是总是简单地
// 将中断状态重设为其原始值（无论
// 那是禁用还是启用）。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	初始化一个信号量，以便它可以用于同步。
//
//	"debugName" 是一个任意名称，用于调试。
//	"initialValue" 是信号量的初始值。
//----------------------------------------------------------------------

Semaphore::Semaphore(const char* debugName, int initialValue)
{
    name = (char*)debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::~Semaphore
// 	释放信号量，当不再需要时。假设没有
//	线程仍在等待信号量！
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	等待直到信号量值 > 0，然后递减。检查
//	值和递减必须是原子的，所以我们
//	需要在检查值之前禁用中断。
//
//	注意 Thread::Sleep 假设中断在
//	调用时是被禁用的。
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// 禁用中断
    
    while (value == 0) { 			// 信号量不可用
	queue->Append((void *)currentThread);	// 所以去睡眠
	currentThread->Sleep();
    } 
    value--; 					// 信号量可用，
						// 消费其值
    
    (void) interrupt->SetLevel(oldLevel);	// 重新启用中断
}

//----------------------------------------------------------------------
// Semaphore::V
// 	递增信号量值，如果必要则唤醒等待者。
//	与 P() 一样，此操作必须是原子的，所以我们需要禁用
//	中断。Scheduler::ReadyToRun() 假设线程
//	在调用时被禁用。
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // 使线程就绪，立即消费 V
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}


//----------------------------------------------------------------------
// Lock::Lock
// 	初始化一个锁，以便它可以用于同步。
//
//	"debugName" 是一个任意名称，用于调试。
//----------------------------------------------------------------------


Lock::Lock(const char* debugName) 
{
    name = (char*)debugName;
    owner = NULL;
    lock = new Semaphore(name,1);
}


//----------------------------------------------------------------------
// Lock::~Lock
// 	释放锁，当不再需要时。与信号量一样，
//	假设没有线程仍在等待锁。
//----------------------------------------------------------------------
Lock::~Lock() 
{
    delete lock;
}

//----------------------------------------------------------------------
// Lock::Acquire
//      使用二进制信号量来实现锁。记录哪个
//      线程获取了锁以便确保只有
//      同一线程释放它。
//----------------------------------------------------------------------
void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);  // 禁用中断

    lock->P();                            // 获取信号量
    owner = currentThread;                // 记录锁的新所有者
    (void) interrupt->SetLevel(oldLevel); // 重新启用中断
}

//----------------------------------------------------------------------
// Lock::Release
//      将锁设置为自由（即释放信号量）。检查
//      当前线程是否被允许释放此锁。
//----------------------------------------------------------------------
void Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);  // 禁用中断

    // 确保：a) 锁是 BUSY  b) 此线程与获取它的线程相同。
    ASSERT(currentThread == owner);        
    owner = NULL;                          // 清除所有者
    lock->V();                             // 释放信号量
    (void) interrupt->SetLevel(oldLevel);
}


//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread()
{
    bool result;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    result = currentThread == owner;
    (void) interrupt->SetLevel(oldLevel);
    return(result);
}

//----------------------------------------------------------------------
// Condition::Condition
// 	初始化一个条件变量，以便它可以用于
//      同步。
//
//	"debugName" 是一个任意名称，用于调试。
//----------------------------------------------------------------------
Condition::Condition(const char* debugName) 
{ 
    name = (char*)debugName;
    queue = new List;
    lock = NULL;
}

//----------------------------------------------------------------------
// Condition::~Condition
// 	释放一个条件变量，当不再需要时。与
//      信号量一样，假设没有线程仍在等待条件。
//----------------------------------------------------------------------

Condition::~Condition() 
{ 
    delete queue;
}

//----------------------------------------------------------------------
// Condition::Wait
//
//      释放锁，放弃 CPU 直到被信号唤醒，然后
//      重新获取锁。
//
//      前提条件：currentThread 持有锁；队列中的
//      线程在等待同一锁。
//----------------------------------------------------------------------
void Condition::Wait(Lock* conditionLock) 
{ 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());  // 检查前提条件
    if(queue->IsEmpty()) {
	lock = conditionLock;  // 有助于强制执行前提条件
    } 
    ASSERT(lock == conditionLock); // 另一个前提条件
    queue->Append(currentThread);  // 将此线程添加到等待列表
    conditionLock->Release();      // 释放锁
    currentThread->Sleep();        // 进入睡眠
    conditionLock->Acquire();      // 唤醒：重新获取锁
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Signal
//      唤醒一个线程，如果条件上有等待的任何线程。
//   
//      前提条件：currentThread 持有锁；队列中的
//      线程在等待同一锁。
//----------------------------------------------------------------------
void Condition::Signal(Lock* conditionLock) 
{ 
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!queue->IsEmpty()) {
	ASSERT(lock == conditionLock);
   	nextThread = (Thread *)queue->Remove();
	scheduler->ReadyToRun(nextThread);      // 唤醒线程
    } 
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Broadcast
//      唤醒所有在条件上等待的线程。
//
//      前提条件：currentThread 持有锁；队列中的
//      线程在等待同一锁。
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLock) 
{ 
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!queue->IsEmpty()) {
	ASSERT(lock == conditionLock);
	while( (nextThread = (Thread *)queue->Remove()) ) {
	    scheduler->ReadyToRun(nextThread);  // 唤醒线程
	}
    } 
    (void) interrupt->SetLevel(oldLevel);
}