#ifndef BARRIER_H
#define BARRIER_H

#include "synch.h"

class Barrier {
public:
    Barrier(const char* debugName, int n);  // 构造函数，n为需要等待的线程数
    ~Barrier();                            // 析构函数
    
    void Wait();                           // 等待所有线程到达屏障点

private:
    char* name;                            // 屏障名称，用于调试
    int threadCount;                       // 需要等待的线程总数
    int arrivedCount;                      // 已到达屏障的线程数
    Semaphore* mutex;                      // 互斥信号量，保护共享变量
    Semaphore* barrier;                    // 屏障信号量，控制线程通过
};

#endif // BARRIER_H