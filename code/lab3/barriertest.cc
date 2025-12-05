#include "copyright.h"
#include "barrier.h"
#include "system.h"

#define NUM_THREADS 10  // 测试线程数量

Barrier* barrier;  // 全局屏障对象

//----------------------------------------------------------------------
// BarrierThread
// 	测试线程函数，模拟线程到达屏障点的过程
//
//	"which" 是线程编号
//----------------------------------------------------------------------
void BarrierThread(int which)
{
    printf("Thread %d: Starting work\n", which);
    
    // 模拟一些工作，使用更大的随机循环增加不确定性
    int work = 5000 + (which * 500) + (Random() % 5000);  // 保持原有基数，增加随机性
    for (int i = 0; i < work; i++) {
        // 空循环模拟工作
        if (i % 50 == 0) {  // 保持原有的让出CPU频率
            currentThread->Yield();
        }
    }
    
    // 在到达rendezvous点之前添加随机延迟
    int randomDelay = Random() % 2000;
    for (int i = 0; i < randomDelay; i++) {
        if (i % 30 == 0) {  // 在延迟期间也频繁让出CPU
            currentThread->Yield();
        }
    }
    
    printf("Thread %d: Reached rendezvous point\n", which);
    
    // 等待所有线程到达屏障
    barrier->Wait();
    
    printf("Thread %d: Passed barrier, continuing work\n", which);
    
    // 继续执行后续工作
    for (int i = 0; i < 500; i++) {
        // 空循环模拟后续工作
    }
    
    printf("Thread %d: Finished\n", which);
}

//----------------------------------------------------------------------
// ThreadsBarrier
// 	初始化并启动N线程屏障测试
//----------------------------------------------------------------------
void ThreadsBarrier()
{
    barrier = new Barrier("test barrier", NUM_THREADS + 1);  // 10个工作线程 + 1个主线程
    
    printf("Starting N-thread barrier test with %d threads\n", NUM_THREADS);
    printf("Each thread will do some work, then wait at the barrier\n");
    printf("Only when all threads reach the barrier will they continue\n\n");
    
    // 创建并启动测试线程
    for (int i = 0; i < NUM_THREADS; i++) {
        char* threadName = new char[16];
        sprintf(threadName, "Thread %d", i);
        Thread* t = new Thread(threadName);
        t->Fork(BarrierThread, i);
    }
    
    // 主线程也参与测试
    printf("Main thread: Starting work\n");
    
    // 模拟主线程的工作，增加随机性
    int mainWork = 800 + (Random() % 3000);
    for (int i = 0; i < mainWork; i++) {
        if (i % 50 == 0) {
            currentThread->Yield();
        }
    }
    
    // 在到达rendezvous点之前添加随机延迟
    int mainRandomDelay = Random() % 2000;
    for (int i = 0; i < mainRandomDelay; i++) {
        if (i % 30 == 0) {
            currentThread->Yield();
        }
    }
    
    printf("Main thread: Reached rendezvous point\n");
    barrier->Wait();
    
    printf("Main thread: Passed barrier, continuing work\n");
    
    // 完成后续工作
    for (int i = 0; i < 300; i++) {
        // 空循环
    }
    
    printf("Main thread: Finished\n\n");
    printf("Barrier test completed\n");
}