// addrspace.cc 
//	管理地址空间的例程（执行用户程序）。
//
//	要运行用户程序，必须：
//
//	1. 使用 -N -T 0 选项链接
//	2. 运行 coff2noff 将目标文件转换为 Nachos 格式
//		(Nachos 对象代码格式本质上是 UNIX 可执行对象代码格式的简化版本)
//	3. 将 NOFF 文件加载到 Nachos 文件系统中
//		(如果尚未实现文件系统，则不需要执行最后一步)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	对目标文件头中的字节执行小端到大端的转换，以防文件是在小端机器上生成的，
//	而我们现在正在大端机器上运行。
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	// 转换魔数
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	// 转换代码段大小
	noffH->code.size = WordToHost(noffH->code.size);
	// 转换代码段虚拟地址
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	// 转换代码段在文件中的地址
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	// 转换已初始化数据段大小
	noffH->initData.size = WordToHost(noffH->initData.size);
	// 转换已初始化数据段虚拟地址
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	// 转换已初始化数据段在文件中的地址
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	// 转换未初始化数据段大小
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	// 转换未初始化数据段虚拟地址
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	// 转换未初始化数据段在文件中的地址
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	创建一个地址空间来运行用户程序。
//	从"executable"文件加载程序，并设置所有内容，以便我们开始执行用户指令。
//
//	假设目标代码文件采用NOFF格式。
//
//	首先，设置从程序内存到物理内存的转换。目前，这非常简单（1:1），
//	因为只运行一个程序，并且我们有一个单一的非分段页表
//
//	"executable"是包含要加载到内存中的目标代码的文件
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    // 从文件偏移0位置读取NOFF头
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    // 检查是否需要字节序转换
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	// 如果需要，进行字节序转换
    	SwapHeader(&noffH);
    // 验证魔数是否正确
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // 地址空间有多大？
    // 计算地址空间总大小 = 代码段大小 + 已初始化数据段大小 + 未初始化数据段大小 + 用户栈大小
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// 需要增加大小以留出栈空间
    // 计算页数，向上取整
    numPages = divRoundUp(size, PageSize);
    // 调整大小为页大小的倍数
    size = numPages * PageSize;

    // 检查我们是否没有尝试运行过大的程序
    // 至少在实现虚拟内存之前
    ASSERT(numPages <= NumPhysPages);		

    // 调试输出：初始化地址空间，页数和大小
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
    // 首先，设置转换表（页表）
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
    	// 当前实现：虚拟页号 = 物理页号（简单映射）
		pageTable[i].virtualPage = i;	
		// 物理页号
		pageTable[i].physicalPage = i;
		// 页有效标志
		pageTable[i].valid = TRUE;
		// 使用标志
		pageTable[i].use = FALSE;
		// 脏页标志
		pageTable[i].dirty = FALSE;
		// 只读标志（当前实现中所有页都是可读写的）
		pageTable[i].readOnly = FALSE;  
    }
    
    // 将整个地址空间清零，以清零未初始化数据段和栈段
    bzero(machine->mainMemory, size);

    // 然后，将代码段和数据段复制到内存中
    if (noffH.code.size > 0) {
        // 调试输出：初始化代码段
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        // 从可执行文件的指定位置读取代码段到内存中
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        // 调试输出：初始化数据段
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        // 从可执行文件的指定位置读取已初始化数据段到内存中
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	释放一个地址空间。目前没有特殊操作！
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   // 释放页表占用的内存
   delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	设置用户级寄存器组的初始值。
//
// 	我们直接将这些写入"machine"寄存器，以便我们可以立即跳转到用户代码。
//	请注意，当此线程上下文切换时，这些将被保存/恢复到currentThread->userRegisters中。
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    // 将所有寄存器清零
    for (i = 0; i < NumTotalRegs; i++)
		machine->WriteRegister(i, 0);

    // 初始程序计数器 -- 必须是"Start"的位置
    machine->WriteRegister(PCReg, 0);	

    // 还需要告诉MIPS下一条指令在哪里，因为
    // 存在分支延迟槽的可能性
    machine->WriteRegister(NextPCReg, 4);

    // 将栈寄存器设置到地址空间的末尾，我们在那里分配了栈；
    // 但减去一点，以确保我们不会意外引用超出末尾！
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    // 调试输出：初始化栈寄存器
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	在上下文切换时，保存任何需要保存的、特定于此地址空间的机器状态。
//
//	目前，不需要保存任何东西！
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	在上下文切换时，恢复机器状态，以便此地址空间可以运行。
//
//      目前，告诉机器在哪里找到页表。
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    // 设置机器的页表指针为当前地址空间的页表
    machine->pageTable = pageTable;
    // 设置页表大小
    machine->pageTableSize = numPages;
}