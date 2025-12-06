// addrspace.h 
//	跟踪正在执行的用户程序的数据结构（地址空间）。
//
//	目前，我们不保存任何关于地址空间的信息。
//	用户级CPU状态保存和恢复在线程中执行用户程序（见thread.h）。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// 根据需要增加！

class AddrSpace {
  public:
    // 构造函数：创建一个地址空间，使用存储在"executable"文件中的程序进行初始化
    AddrSpace(OpenFile *executable);	
    // 析构函数：释放一个地址空间
    ~AddrSpace();			

    // 初始化用户级CPU寄存器，在跳转到用户代码之前
    void InitRegisters();		

    // 在上下文切换时保存/恢复地址空间特定信息
    void SaveState();			
    void RestoreState();		

  private:
    // 假设线性页表转换（当前实现）
    TranslationEntry *pageTable;	
    // 虚拟地址空间中的页数
    unsigned int numPages;		
};

#endif // ADDRSPACE_H