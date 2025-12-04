// system.cc 
//	Nachos 初始化和清理例程。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"
#include "system.h"

// 这定义了 Nachos 使用的*所有*全局数据结构。
// 这些都是由这个文件初始化和释放的。

Thread *currentThread;			// 我们当前运行的线程
Thread *threadToBeDestroyed;  		// 刚刚完成的线程
Scheduler *scheduler;			// 就绪列表
Interrupt *interrupt;			// 中断状态
Statistics *stats;			// 性能指标
Timer *timer;				// 硬件定时器设备，
					// 用于调用上下文切换

#ifdef FILESYS_NEEDED
FileSystem  *fileSystem;
#endif

#ifdef FILESYS
SynchDisk   *synchDisk;
#endif

#ifdef USER_PROGRAM	// 需要 FILESYS 或 FILESYS_STUB
Machine *machine;	// 用户程序内存和寄存器
#endif

#ifdef NETWORK
PostOffice *postOffice;
#endif


// 外部定义，以便我们可以指向这个函数
extern void Cleanup();


//----------------------------------------------------------------------
// TimerInterruptHandler
// 	定时器设备的中断处理程序。定时器设备
//	被设置为周期性地中断CPU（每 TimerTicks 一次）。
//	每次发生定时器中断时都调用此例程，
//	中断被禁用。
//
//	注意，不是直接调用 Yield()（这会
//	挂起中断处理程序，而不是被中断的线程
//	我们想要的是上下文切换），我们设置了一个标志
//	以便一旦中断处理程序完成，就会出现
//	被中断的线程在它被中断的点调用了 Yield。
//
//	"dummy" 是因为每个中断处理程序都需要一个参数，
//		不管它是否需要。
//----------------------------------------------------------------------
static void
TimerInterruptHandler(_int dummy)
{
    if (interrupt->getStatus() != IdleMode)
	interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
// 	初始化 Nachos 全局数据结构。解释命令
//	行参数以确定初始化的标志。
// 
//	"argc" 是命令行参数的数量（包括命令的名称）
//		例如： "nachos -d +" -> argc = 3 
//	"argv" 是一个字符串数组，每个命令行参数一个
//		例如： "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void
Initialize(int argc, char **argv)
{
    int argCount;
    char* debugArgs = (char*)"";
    bool randomYield = FALSE;

#ifdef USER_PROGRAM
    bool debugUserProg = FALSE;	// 单步执行用户程序
#endif
#ifdef FILESYS_NEEDED
    bool format = FALSE;	// 格式化磁盘
#endif
#ifdef NETWORK
    double rely = 1;		// 网络可靠性
    double order = 1;           // 网络有序性
    int netname = 0;		// UNIX 套接字名称
#endif
    
    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
	if (!strcmp(*argv, "-d")) {
	    if (argc == 1)
		debugArgs = (char*)"+";	// 打开所有调试标志
	    else {
	    	debugArgs = *(argv + 1);
	    	argCount = 2;
	    }
	} else if (!strcmp(*argv, "-rs")) {
	    ASSERT(argc > 1);
	    RandomInit(atoi(*(argv + 1)));	// 初始化伪随机
						// 数字生成器
	    randomYield = TRUE;
	    argCount = 2;
	}
#ifdef USER_PROGRAM
	if (!strcmp(*argv, "-s"))
	    debugUserProg = TRUE;
#endif
#ifdef FILESYS_NEEDED
	if (!strcmp(*argv, "-f"))
	    format = TRUE;
#endif
#ifdef NETWORK
	if (!strcmp(*argv, "-n")) {
	    ASSERT(argc > 1);
	    rely = atof(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-e")) {
	    ASSERT(argc > 1);
	    order = atof(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-m")) {
	    ASSERT(argc > 1);
	    netname = atoi(*(argv + 1));
	    argCount = 2;
	}
#endif
    }

    DebugInit(debugArgs);			// 初始化 DEBUG 消息
    stats = new Statistics();			// 收集统计信息
    interrupt = new Interrupt;			// 开始中断处理
    scheduler = new Scheduler();		// 初始化就绪队列
    if (randomYield)				// 启动定时器（如果需要）
	timer = new Timer(TimerInterruptHandler, 0, randomYield);

    threadToBeDestroyed = NULL;

    // 我们没有显式分配我们正在运行的当前线程。
    // 但如果它尝试放弃 CPU，我们最好有一个 Thread
    // 对象来保存它的状态。
    currentThread = new Thread("main");		
    currentThread->setStatus(RUNNING);

    interrupt->Enable();
    CallOnUserAbort(Cleanup);			// 如果用户按 ctl-C
    
#ifdef USER_PROGRAM
    machine = new Machine(debugUserProg);	// 这必须首先出现
#endif

#ifdef FILESYS
    synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED
    fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
    postOffice = new PostOffice(netname, rely, order, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos 正在停止。释放全局数据结构。
//----------------------------------------------------------------------
void
Cleanup()
{
    printf("\n正在清理...\n");
#ifdef NETWORK
    delete postOffice;
#endif
    
#ifdef USER_PROGRAM
    delete machine;
#endif

#ifdef FILESYS_NEEDED
    delete fileSystem;
#endif

#ifdef FILESYS
    delete synchDisk;
#endif
    
    delete timer;
    delete scheduler;
    delete interrupt;
    
    Exit(0);
}