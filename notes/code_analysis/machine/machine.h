// machine.h 
//	模拟用户程序执行的数据结构
//	运行在Nachos之上。
//
//	用户程序被加载到"mainMemory"中；对Nachos来说，
//	这看起来就像一个字节数组。当然，Nachos
//	内核也在内存中 -- 但和当今大多数机器一样，
//	内核被加载到与用户程序分离的内存区域，
//	对内核内存的访问不会被转换或分页。
//
//	在Nachos中，用户程序一次执行一条指令，
//	由模拟器执行。每条内存引用都会被转换，
//	检查错误等。
//
//  不要更改 -- 机器模拟的一部分
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef MACHINE_H
#define MACHINE_H

#include "copyright.h"
#include "utility.h"
#include "translate.h"
#include "disk.h"

// 与用户内存大小和格式相关的定义

#define PageSize 	SectorSize 	// 将页面大小设置为等于
					// 磁盘扇区大小，为了
					// 简化

#define NumPhysPages    32
#define MemorySize 	(NumPhysPages * PageSize)
#define TLBSize		4		// 如果有TLB，使其小一些

enum ExceptionType { NoException,           // 一切正常！
		     SyscallException,      // 程序执行了系统调用。
		     PageFaultException,    // 没有找到有效的转换
		     ReadOnlyException,     // 尝试写入标记为
					    // "只读"的页面
		     BusErrorException,     // 转换导致了
					    // 无效的物理地址
		     AddressErrorException, // 未对齐的引用或一个
					    // 超出地址空间
					    // 结尾的引用
		     OverflowException,     // 加法或减法中的整数溢出。
		     IllegalInstrException, // 未实现或保留的指令。
		     
		     NumExceptionTypes
};

// 用户程序CPU状态。完整的MIPS寄存器集，再加上几个
// 更多，因为我们需要能够在
// 任何两条指令之间启动/停止用户程序
// （因此我们需要跟踪像加载
// 延迟槽等信息）

#define StackReg	29	// 用户的栈指针
#define RetAddrReg	31	// 保存过程调用的返回地址
#define NumGPRegs	32	// MIPS上有32个通用寄存器
#define HiReg		32	// 双寄存器，用于保存乘法结果
#define LoReg		33
#define PCReg		34	// 当前程序计数器
#define NextPCReg	35	// 下一个程序计数器（用于分支延迟）
#define PrevPCReg	36	// 上一个程序计数器（用于调试）
#define LoadReg		37	// 延迟加载的目标寄存器。
#define LoadValueReg 	38	// 延迟加载要加载的值。
#define BadVAddrReg	39	// 异常时失败的虚拟地址

#define NumTotalRegs 	40

// 以下类定义一个指令，用两种方式表示
// 	未解码的二进制形式
//      解码以识别
//	    要执行的操作
//	    要操作的寄存器
//	    任何立即数操作数值

class Instruction {
  public:
    void Decode();	// 解码指令的二进制表示

    unsigned int value; // 指令的二进制表示

    unsigned char opCode;     // 指令类型。这与指令的
    		     // 操作码字段不同：参见mips.h中的定义
    unsigned char rs, rt, rd; // 指令中的三个寄存器。
    int extra;       // 立即数或目标或shamt字段或偏移。
                     // 立即数是符号扩展的。
};

// 以下类定义了模拟的主机工作站硬件，如
// 用户程序所见 -- CPU寄存器、主内存等。
// 用户程序不应该能够分辨它们是在我们的
// 模拟器上运行还是在真实硬件上运行，除了
//	我们不支持浮点指令
//	Nachos的系统调用接口与UNIX不同
//	  (Nachos中有10个系统调用 vs. UNIX中的200个！)
// 如果我们实现更多的UNIX系统调用，我们应当能够
// 在Nachos之上运行Nachos！
//
// 此类中的过程在machine.cc、mipssim.cc和
// translate.cc中定义。

class Machine {
  public:
    Machine(bool debug);	// 初始化运行用户程序的硬件
				// 模拟
    ~Machine();			// 释放数据结构

// Nachos内核可调用的例程
    void Run();	 		// 运行一个用户程序

    int ReadRegister(int num);	// 读取CPU寄存器的内容

    void WriteRegister(int num, int value);
				// 将一个值存储到CPU寄存器中


// 机器模拟内部的例程 -- 不要调用这些

    void OneInstruction(Instruction *instr); 	
    				// 运行用户程序的一条指令。
    void DelayedLoad(int nextReg, int nextVal);  	
				// 执行一个待处理的延迟加载（修改一个寄存器）
    
    bool ReadMem(int addr, int size, int* value);
    bool WriteMem(int addr, int size, int value);
    				// 读取或写入1、2或4字节的虚拟
				// 内存（在addr处）。如果找不到
				// 正确的转换则返回FALSE。
    
    ExceptionType Translate(int virtAddr, int* physAddr, int size,bool writing);
    				// 转换一个地址，并检查
				// 对齐。在转换条目中设置
				// 使用和脏位适当，
    				// 如果转换无法完成
				// 则返回异常码。

    void RaiseException(ExceptionType which, int badVAddr);
				// 陷入Nachos内核，因为
				// 系统调用或其他异常。

    void Debugger();		// 调用用户程序调试器
    void DumpState();		// 打印用户CPU和内存状态


// 数据结构 -- 所有这些都对Nachos内核代码可访问。
// "public"是为了方便。
//
// 注意，用户程序和内核之间的*所有*通信
// 都是使用这些数据结构。

    char *mainMemory;		// 物理内存，用于存储用户程序，
				// 代码和数据，在执行时
    int registers[NumTotalRegs]; // CPU寄存器，用于执行用户程序


// 注意：用户程序中虚拟地址的硬件转换
// 到物理地址（相对于"mainMemory"的开头）
// 可以通过以下方式控制：
//	传统的线性页表
//  	软件加载的转换旁路缓冲器（tlb） -- 一个
//	  虚拟页面号到物理页面号映射的缓存
//
// 如果"tlb"为NULL，则使用线性页表
// 如果"tlb"非NULL，则由Nachos内核负责管理
//	TLB的内容。但内核可以使用任何数据结构
//	它想要的（例如，分段分页）来处理TLB缓存未命中。
// 
// 为了简化，页表指针和TLB指针都是
// 公共的。然而，虽然可以有多个页表（每个地址
// 空间一个，存储在内存中），但只有一个TLB（在硬件中实现）。
// 因此TLB指针应被视为*只读*，尽管
// TLB的内容可以由内核软件自由修改。

    TranslationEntry *tlb;		// 这个指针应被视为
					// "只读"对Nachos内核代码

    TranslationEntry *pageTable;
    unsigned int pageTableSize;

  private:
    bool singleStep;		// 在每个模拟指令后
				// 退回调试器
    int runUntilTime;		// 当模拟时间达到此值时
				// 退回调试器
};

extern void ExceptionHandler(ExceptionType which);
				// 进入Nachos的入口点，用于处理
				// 用户系统调用和异常
				// 在exception.cc中定义


// 用于将字和短字转换为和从
// 模拟机器的小端格式。如果主机机器
// 是小端（DEC和Intel），这些最终是NOPs。
//
// 每种格式存储的内容：
//	主机字节序：
//	   内核数据结构
//	   用户寄存器
//	模拟机器字节序：
//	   主内存的内容

unsigned int WordToHost(unsigned int word);
unsigned short ShortToHost(unsigned short shortword);
unsigned int WordToMachine(unsigned int word);
unsigned short ShortToMachine(unsigned short shortword);

#endif // MACHINE_H