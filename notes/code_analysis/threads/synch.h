// synch.h 
//	线程同步的数据结构。
//
//	这里定义了三种同步：信号量、
//	锁和条件变量。信号量的实现
//	已经给出；对于后两种，只给出了过程
//	接口 -- 它们需要作为第一项作业
//	的一部分来实现。
//
//	注意所有同步对象都接受一个"名称"作为
//	初始化的一部分。这只是为了调试目的。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// synch.h -- 同步原语。

#ifndef SYNCH_H
#define SYNCH_H

#include "copyright.h"
#include "thread.h"
#include "list.h"


// 以下类定义了一个值为非负整数的"信号量"。
// 信号量只有两个操作 P() 和 V()：
//
//	P() -- 等待直到值 > 0，然后递减
//
//	V() -- 递增，如果必要则唤醒在 P() 中等待的线程
// 
// 注意接口*不*允许线程直接读取
// 信号量的值 -- 即使你读取了值，
// 你只知道的是值曾经是多少。你不知道
// 现在的值是多少，因为当你将值
// 读入寄存器时，可能会发生上下文切换，
// 其他线程可能调用了 P 或 V，所以真实值
// 现在可能不同。

class Semaphore {
  public:
    Semaphore(const char* debugName, int initialValue);	// 设置初始值
    ~Semaphore();   					// 释放信号量
    char* getName() { return name;}			// 调试辅助
    
    void P();	 // 这些是信号量的唯一操作
    void V();	 // 它们都是*原子的*
    
  private:
    char* name;  // 用于调试
    int value;         // 信号量值，始终 >= 0
    List *queue;       // 在 P() 中等待值 > 0 的线程
};

// 以下类定义了一个"锁"。锁可以是 BUSY（忙）或 FREE（空闲）。
// 锁上只允许两种操作：
//
//	Acquire -- 等待直到锁是 FREE，然后设置为 BUSY
//
//	Release -- 设置锁为 FREE，如果必要则唤醒
//		在 Acquire 中等待的线程
//
// 另外，按约定，只有获取锁的线程
// 可以释放它。与信号量一样，你不能读取锁的值
// （因为值在你读取后可能立即改变）。

class Lock {
  public:
    Lock(const char* debugName);  		// 初始化锁为 FREE
    ~Lock();				// 释放锁
    char* getName() { return name; }	// 调试辅助

    void Acquire(); // 这些是锁的唯一操作
    void Release(); // 它们都是*原子的*

    bool isHeldByCurrentThread();	// 如果当前线程
					// 持有此锁则返回 true。用于
					// 在 Release 中检查，以及在
					// 下面的条件变量操作中。

  private:
    char* name;				// 用于调试
    Thread *owner;                      // 记住谁获取了锁
    Semaphore *lock;                    // 使用信号量实现实际的锁
};

// 以下类定义了一个"条件变量"。条件
// 变量没有值，但线程可以排队，等待
// 该变量。条件变量上只有这些操作：
//
//	Wait() -- 释放锁，放弃 CPU 直到被信号唤醒，
//		然后重新获取锁
//
//	Signal() -- 唤醒一个线程，如果有的话在
//		该条件上等待
//
//	Broadcast() -- 唤醒在条件上等待的所有线程
//
// 对条件变量的所有操作必须在
// 当前线程获取了锁时进行。实际上，所有访问
// 给定条件变量的操作必须由同一锁保护。
// 换句话说，必须在调用条件变量操作的线程之间
// 强制执行互斥。
//
// 在Nachos中，条件变量假设遵循*Mesa*风格
// 语义。当 Signal 或 Broadcast 唤醒另一个线程时，
// 它只是将线程放到就绪列表上，而被唤醒的线程
// 负责重新获取锁（这个重新获取
// 在 Wait() 中处理）。相比之下，一些定义条件
// 变量根据 *Hoare* 风格语义 -- 其中发信号的
// 线程放弃对锁和 CPU 的控制权给被唤醒的线程，
// 被唤醒的线程立即运行并在离开临界区时
// 将锁的控制权交还给发信号者。
//
// 使用 Mesa 风格语义的后果是其他线程
// 可以在被唤醒的线程有机会运行之前
// 获取锁并改变数据结构。

class Condition {
  public:
    Condition(const char* debugName);		// 初始化条件为
					// "没有人等待"
    ~Condition();			// 释放条件变量
    char* getName() { return name; }
    
    void Wait(Lock *conditionLock); 	// 这些是条件变量的 3 个操作
					// ；在 Wait() 中释放
					// 锁和进入睡眠是*原子的*
    void Signal(Lock *conditionLock);   // conditionLock 必须由
    void Broadcast(Lock *conditionLock);// currentThread 持有才能进行所有这些操作

  private:
    char* name;
    List* queue;  // 在条件变量上等待的线程
    Lock* lock;   // 调试辅助：用于检查
                  // Wait、Signal 和 Broadcast 参数的正确性
};
#endif // SYNCH_H