// addrspace.h 
//	跟踪执行中的用户程序的数据结构
//	(地址空间)。
//
//	目前，我们不保存任何关于地址空间的信息。
//	用户级CPU状态在执行用户程序的线程中
//	保存和恢复(参见thread.h)。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// 根据需要增加这个值！

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// 创建一个地址空间，
					// 用存储在"executable"文件中的程序
					// 初始化它
    ~AddrSpace();			// 释放一个地址空间

    void InitRegisters();		// 初始化用户级CPU寄存器，
					// 在跳转到用户代码之前

    void SaveState();			// 在上下文切换时保存/恢复
    void RestoreState();		// 地址空间特定信息

  private:
    TranslationEntry *pageTable;	// 假设线性页表转换
					// 现在！
    unsigned int numPages;		// 虚拟地址空间中的页数
					// 地址空间
};

#endif // ADDRSPACE_H