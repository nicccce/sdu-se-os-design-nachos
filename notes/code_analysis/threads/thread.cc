// thread.cc 
//	线程管理例程。有四个主要操作：
//
//	Fork -- 创建一个线程与调用者并发运行一个过程
//		(这分两步完成 -- 首先分配Thread对象，
//		然后在其上调用Fork)
//	Finish -- 当forked过程完成时调用，以清理
//	Yield -- 将CPU控制权让给另一个就绪线程
//	Sleep -- 释放CPU控制权，但线程现在被阻塞。
//		换句话说，它不会再次运行，直到明确
//		放回就绪队列。
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制
// 以及保修声明条款。

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef	// 这被放在执行栈的顶部，
					// 用于检测栈溢出

//----------------------------------------------------------------------
// Thread::Thread
// 	初始化线程控制块，以便我们可以调用
//	Thread::Fork。
//
//	"threadName" 是一个任意字符串，对调试有用。
//----------------------------------------------------------------------

Thread::Thread(const char* threadName)
{
    name = (char*)threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	释放一个线程。
//
// 	注意：当前线程*不能*直接删除自己，
//	因为它仍在我们需删除的栈上运行。
//
//      注意：如果是主线程，我们不能删除栈
//      因为我们没有分配它 -- 我们在启动Nachos时
//      自动获得了它。
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
		DeallocBoundedArray((char *) stack, StackSize * sizeof(_int));
}

//----------------------------------------------------------------------
// Thread::Fork
// 	调用 (*func)(arg)，允许调用者和被调用者并发执行。
//
//	注意：虽然我们的定义只允许传递单个整数参数
//	到过程，但可以通过将它们作为结构的字段来传递多个
//	参数，并将指向结构的指针作为"arg"传递。
//
// 	实现为以下步骤：
//		1. 分配一个栈
//		2. 初始化栈，以便调用SWITCH将
//		导致它运行该过程
//		3. 将线程放在就绪队列上
// 	
//	"func" 是要并发运行的过程。
//	"arg" 是要传递给过程的单个参数。
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, _int arg)
{
#ifdef HOST_ALPHA
    DEBUG('t', "Forking thread \"%s\" with func = 0x%lx, arg = %ld\n",
	  name, (long) func, arg);
#else
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
	  name, (int) func, arg);
#endif
    
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);	// ReadyToRun假设中断
					// 已被禁用！
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	检查线程栈，看它是否超出了为其分配的空间。
//	如果我们有一个更智能的编译器，我们就不需要担心这个，
//	但我们没有。
//
// 	注意：Nachos不会捕获所有栈溢出情况。
//	换句话说，由于溢出，你的程序仍可能崩溃。
//
// 	如果你得到奇怪的结果（比如在没有代码的地方出现段错误）
// 	那么你*可能*需要增加栈大小。你可以通过不在栈上
// 	放置大型数据结构来避免栈溢出。
// 	不要这样做：void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// 在Snake上栈向高地址增长
	ASSERT((unsigned int)stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT((unsigned int)*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	当线程完成执行forked过程时由ThreadRoot调用。
//
// 	注意：我们不会立即释放线程数据结构
//	或执行栈，因为我们仍在该线程中运行
//	并且仍在栈上！相反，我们设置"threadToBeDestroyed"，
//	所以Scheduler::Run()将在我们在不同线程的上下文中
//	运行时调用析构函数。
//
// 	注意：我们禁用中断，这样在设置threadToBeDestroyed
//	和进入睡眠之间就不会获得时间片。
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\"\n", getName());
    
    threadToBeDestroyed = currentThread;
    Sleep();					// 调用SWITCH
    // 无法到达此处
}

//----------------------------------------------------------------------
// Thread::Yield
// 	如果任何其他线程准备运行，则释放CPU。
//	如果是这样，将线程放在就绪列表的末尾，以便
//	它最终会被重新调度。
//
//	注意：如果就绪队列上没有其他线程，则立即返回。
//	否则，当线程最终到达就绪列表的前面并被重新调度时
//	返回。
//
//	注意：我们禁用中断，以便查看就绪列表
//	前面的线程并切换到它，可以原子地完成。
//	在返回时，我们将中断级别重新设置为其
//	原始状态，以防我们以禁用中断的方式被调用。
//
// 	类似于Thread::Sleep()，但有些不同。
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    
    DEBUG('t', "Yielding thread \"%s\"\n", getName());
    
    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL) {
	scheduler->ReadyToRun(this);
	scheduler->Run(nextThread);
    }
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	释放CPU，因为当前线程被阻塞
//	等待同步变量（信号量、锁或条件）。
//	最终，某个线程会唤醒此线程，并将其
//	放回就绪队列，以便它可以被重新调度。
//
//	注意：如果就绪队列上没有线程，这意味着
//	我们没有线程可运行。"Interrupt::Idle"被调用
//	表示我们应该让CPU空闲直到下一个I/O中断
//	发生（这是导致线程变为
//	准备运行的唯一事情）。
//
//	注意：我们假设中断已经禁用，因为它
//	从必须禁用中断以保证原子性的同步例程
//	中调用。我们需要关闭中断
//	这样在从就绪列表中取出第一个线程
//	和切换到它之间就不会有时间片。
//----------------------------------------------------------------------
void
Thread::Sleep ()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
	interrupt->Idle();	// 没有人运行，等待中断
        
    scheduler->Run(nextThread); // 当我们被信号通知时返回
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	哑函数，因为C++不允许指向成员函数的指针。
//	所以为了这样做，我们创建一个哑C函数
//	（我们可以传递指向它的指针），然后简单地调用
//	成员函数。
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(_int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//	分配并初始化一个执行栈。栈使用ThreadRoot的
//	初始栈帧进行初始化，其中：
//		启用中断
//		调用 (*func)(arg)
//		调用 Thread::Finish
//
//	"func" 是要fork的过程
//	"arg" 是要传递给过程的参数
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, _int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(_int));

#ifdef HOST_SNAKE
    // HP栈从低地址到高地址工作
    stackTop = stack + 16;	// HP需要64字节帧标记
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC & ALPHA 栈从高地址到低地址工作
#ifdef HOST_SPARC
    // SPARC栈必须至少包含1个激活记录才能开始。
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386 || HOST_ALPHA
    stackTop = stack + StackSize - 4;	// -4 为了安全起见！
#ifdef HOST_i386
    // 80386在栈上传递返回地址。为了
    // 在切换到此线程时SWITCH()转到ThreadRoot，
    // SWITCH()中使用的返回地址必须是
    // ThreadRoot的起始地址。

    //    *(--stackTop) = (int)ThreadRoot;
    // 在修复i386中SWITCH函数的错误后，此语句可以被注释掉：
    // 当前i386 SWITCH的最后三条指令如下：
    // movl    %eax,4(%esp)            # 将栈上的返回地址复制过去
    // movl    _eax_save,%eax
    // ret
    // 这里"movl    %eax,4(%esp)"应该是"movl   %eax,0(%esp)"。
    // 修复此错误后，下一个语句存储在machineState[PCState]中的
    // ThreadRoot的起始地址
    // 将在SWITCH函数"返回"到ThreadRoot时被放入%esp指向的位置。
    // 看起来这个语句是用来绕过SWITCH中的那个错误的。
    //
    // 然而，如果i386的SWITCH进一步简化，这个语句将是必要的。
    // 实际上，保存和恢复返回地址的代码都是冗余的，因为
    // 返回地址已经在栈中（由%esp指向）。
    // 即，以下四条指令可以被移除：
    // ...
    // movl    0(%esp),%ebx            # 将返回地址从栈获取到ebx
    // movl    %ebx,_PC(%eax)          # 将其保存到pc存储中
    // ...
    // movl    _PC(%eax),%eax          # 将返回地址恢复到eax
    // movl    %eax,0(%esp)            # 将返回地址复制到栈上

    // SWITCH函数可以如下：
//         .comm   _eax_save,4

//         .globl  SWITCH
// SWITCH:
//         movl    %eax,_eax_save          # 保存eax的值
//         movl    4(%esp),%eax            # 将t1的指针移动到eax
//         movl    %ebx,_EBX(%eax)         # 保存寄存器
//         movl    %ecx,_ECX(%eax)
//         movl    %edx,_EDX(%eax)
//         movl    %esi,_ESI(%eax)
//         movl    %edi,_EDI(%eax)
//         movl    %ebp,_EBP(%eax)
//         movl    %esp,_ESP(%eax)         # 保存栈指针
//         movl    _eax_save,%ebx          # 获取eax的保存值
//         movl    %ebx,_EAX(%eax)         # 存储它

//         movl    8(%esp),%eax            # 将t2的指针移动到eax

//         movl    _EAX(%eax),%ebx         # 将eax的新值获取到ebx
//         movl    %ebx,_eax_save          # 保存它
//         movl    _EBX(%eax),%ebx         # 恢复旧寄存器
//         movl    _ECX(%eax),%ecx
//         movl    _EDX(%eax),%edx
//         movl    _ESI(%eax),%esi
//         movl    _EDI(%eax),%edi
//         movl    _EBP(%eax),%ebp
//         movl    _ESP(%eax),%esp         # 恢复栈指针

//         movl    _eax_save,%eax

//         ret

    // 在这种情况下，上述语句
    //    *(--stackTop) = (int)ThreadRoot;
    // 是必要的。但是，以下语句
    //    machineState[PCState] = (_int) ThreadRoot;
    // 变得冗余。

    // Peiyi Tang, ptang@titus.compsci.ualr.edu
    // 计算机科学系
    // 阿肯色大学小石城分校
    // 2003年9月1日

#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState] = (_int) ThreadRoot;
    machineState[StartupPCState] = (_int) InterruptEnable;
    machineState[InitialPCState] = (_int) func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (_int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	在上下文切换时保存用户程序的CPU状态。
//
//	注意，用户程序线程有*两套*CPU寄存器 --
//	一套用于执行用户代码时的状态，一套用于执行
//	内核代码时的状态。此例程保存前者。
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	在上下文切换时恢复用户程序的CPU状态。
//
//	注意，用户程序线程有*两套*CPU寄存器 --
//	一套用于执行用户代码时的状态，一套用于执行
//	内核代码时的状态。此例程恢复前者。
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, userRegisters[i]);
}
#endif