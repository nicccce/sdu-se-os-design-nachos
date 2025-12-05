// synchdisk.h 
// 	用于向原始磁盘设备导出同步接口的数据结构。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef SYNCHDISK_H
#define SYNCHDISK_H

#include "disk.h"
#include "synch.h"

// 下面的类定义了一个"同步"磁盘抽象。
// 与其他 I/O 设备一样，原始物理磁盘是异步设备 --
// 读取或写入磁盘部分的请求立即返回，
// 稍后发生中断以表示操作完成。
//（此外，磁盘设备的物理特性假设一次只能请求一个操作）。
//
// 这个类提供了这样的抽象：对于任何发出请求的单个线程，
// 它会等待直到操作完成才返回。
//
// SynchDisk同步磁盘设计说明：
// 1. 同步封装：将异步的物理磁盘操作转换为同步操作
// 2. 线程安全：使用锁确保同一时间只有一个磁盘操作
// 3. 完成通知：使用信号量等待操作完成
// 4. 中断处理：提供回调接口给物理磁盘中断使用
//
// 工作原理：
// - 线程调用ReadSector/WriteSector
// - 获取锁确保独占访问
// - 向物理磁盘发送请求
// - 在信号量上等待(P操作)
// - 物理磁盘完成操作并触发中断
// - 中断处理程序调用RequestDone
// - RequestDone执行V操作释放信号量
// - 等待线程被唤醒并返回
class SynchDisk {
  public:
    // 构造函数：初始化同步磁盘
    // 参数：name - 磁盘文件名(通常是"DISK")
    // 操作：创建信号量、锁和物理磁盘对象
    SynchDisk(const char* name);    

    // 析构函数：释放同步磁盘资源
    // 操作：删除所有动态分配的对象
    ~SynchDisk();			
    
    // 从指定扇区读取数据
    // 参数：sectorNumber - 扇区号，data - 存储数据的缓冲区
    // 功能：同步读取操作，仅在读取完成后返回
    // 实现：调用Disk::ReadRequest，然后等待完成信号
    void ReadSector(int sectorNumber, char* data);
    				

    // 向指定扇区写入数据
    // 参数：sectorNumber - 扇区号，data - 要写入的数据
    // 功能：同步写入操作，仅在写入完成后返回
    // 实现：调用Disk::WriteRequest，然后等待完成信号
    void WriteSector(int sectorNumber, char* data);
    
    // 磁盘操作完成回调函数
    // 由物理磁盘中断处理程序调用
    // 功能：释放等待的线程
    void RequestDone();		

  private:
    Disk *disk;		  		// 物理磁盘设备指针
    Semaphore *semaphore; 		// 同步信号量，用于等待操作完成
    Lock *lock;		  		// 互斥锁，确保一次只有一个磁盘操作
    
    // 核心同步机制：
    // - lock: 确保磁盘访问的互斥性
    // - semaphore: 实现操作完成的同步通知
    // - 中断驱动的完成机制
};

#endif // SYNCHDISK_H