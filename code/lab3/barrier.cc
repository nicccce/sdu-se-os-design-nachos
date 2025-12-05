#include "barrier.h"
#include "system.h"

//----------------------------------------------------------------------
// Barrier::Barrier
// 	初始化一个屏障对象，设置需要等待的线程数和相关信号量
//
//	"debugName" 是调试用的名称
//	"n" 是需要等待的线程总数
//----------------------------------------------------------------------
Barrier::Barrier(const char* debugName, int n)
{
    name = (char*)debugName;
    threadCount = n;
    arrivedCount = 0;
    mutex = new Semaphore("barrier mutex", 1);  // 初始值为1的互斥信号量
    barrier = new Semaphore("barrier", 0);      // 初始值为0的屏障信号量
}

//----------------------------------------------------------------------
// Barrier::~Barrier
// 	释放屏障对象占用的资源
//----------------------------------------------------------------------
Barrier::~Barrier()
{
    delete mutex;
    delete barrier;
}

//----------------------------------------------------------------------
// Barrier::Wait
// 	等待所有线程到达屏障点，实现N线程屏障
// 	修改后的实现，消除竞争条件
//----------------------------------------------------------------------
void Barrier::Wait()
{
    // rendezvous point
    
    // 进入互斥区，保护共享变量和条件检查
    mutex->P();
    arrivedCount = arrivedCount + 1;
    
    // 在互斥区内检查并处理屏障唤醒，确保原子性
    if (arrivedCount == threadCount) {
        // 最后一个线程：重置计数器并唤醒所有等待的线程
        arrivedCount = 0;  // 重置计数器，为下一次使用做准备
        for (int i = 0; i < threadCount - 1; i++) {
            barrier->V();  // 唤醒其他threadCount-1个线程
        }
        mutex->V();  // 最后一个线程不需要等待，直接通过
    } else {
        mutex->V();  // 非最后一个线程：释放互斥锁
        barrier->P();  // 等待被最后一个线程唤醒
    }
    
    // critical point - 所有线程都已通过屏障
}