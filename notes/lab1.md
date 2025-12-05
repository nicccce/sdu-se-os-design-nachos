# 实验一



## 環境搭建與Nachos安裝

开发环境：

  硬件信息
   - CPU: AMD EPYC 7543 32-Core Processor (4核心, 4线程)
   - 架构: x86_64
   - 内存: 未显示，但系统运行正常
   - 虚拟化: VMware虚拟机环境
   - 缓存: L1d/L1i缓存各128 KiB, L2缓存2 MiB, L3缓存1 GiB

  系统版本
   - 操作系统: Ubuntu Linux 22.04 (基于Linux 6.2.0-32-generic)
   - 内核版本: 6.2.0-32-generic #32~22.04.1-Ubuntu SMP 
     PREEMPT_DYNAMIC
   - 架构: x86_64 GNU/Linux

  开发工具版本
   - GCC编译器: 11.4.0 (Ubuntu 11.1.0-1ubuntu1~22.04)
   - Make工具: GNU Make 4.3
   - Git版本控制: 2.52.0
   - Vim编辑器: 8.2 (2019 Dec 12, with patches 1-3995)
   - VSCode编辑器: 1.106.3 (Commit: 
     bf9252a2fb45be6893dd8870c0bf37e2e1766d61)

运行环境：Ubuntu 20.04.3 LTS amd64



### 运行环境配置

本实验使用Docker容器进行搭建Ubuntu 20.04 LTS amd64的运行环境。

步骤 1: 拉取 Ubuntu 20.04 镜像

```bash
sudo docker pull ubuntu:20.04
```

![image-20251203213141919](lab1.assets/image-20251203213141919.png)



步骤 2: 创建容器并挂载当前目录

```bash
sudo docker run -d -v $(pwd):/root/sdu-se-os-design-nachos --name nachos-container ubuntu:20.04 tail -f /dev/null
```

创建后可以看到目录正确挂载：

![image-20251203213608537](lab1.assets/image-20251203213608537.png)

查看内核版本并退出

![image-20251203213734940](lab1.assets/image-20251203213734940.png)

步骤 3: 进入容器的 bash

```bash
sudo docker exec -it nachos-container bash
```

更新Ubuntu的源

```bash
sudo apt update
```

安装gcc，g++，make，及为编译并运行32位应用需要的一些gcc库

```bash
sudo apt install -y gcc
sudo apt install -y g++
sudo apt install -y make
sudo apt install -y gcc-multilib g++-multilib
```



这是项目目录的截图

![image-20251203220355762](lab1.assets/image-20251203220355762.png)



### 安装MIPS的交叉编译器

将mips工具的压缩包拷贝到mips-tools(实现环境内外文件传输)

```
sudo tar -xzvf ./gcc-2.8.1-mips.tar.gz
cp -r ~/sdu-se-os-design-nachos/mips-tools/mips /usr/local/
```



### 测试Nachos

#### 测试Nachos threads

编译：

```
cd ~/sdu-se-os-design-nachos/code/threads/
make clean
make
```

![image-20251203221131125](lab1.assets/image-20251203221131125.png)

运行：

```
./nachos
```

显示如下页：

![image-20251203221214973](lab1.assets/image-20251203221214973.png)

确认转换工具有x权限

![image-20251203221416395](lab1.assets/image-20251203221416395.png)



#### 生成能加在用户进程的nachos

```
cd ../userprog
make clean
make
```



#### 生成用户MIPS程序halt,matmult,shell

```
cd ../test
make clean
make
```

运行

```
../userprog/nachos -x halt.noff
```

![image-20251204000146258](lab1.assets/image-20251204000146258.png)





## Nachos代码基本分析

### 工程代码规范

这是一个相对标准的工程代码实现了

#### 1. 头文件(.h)和源文件(.cc)分离

.h头文件负责声明变量、函数、类和宏定义

**头文件(.h)的作用：**
- **声明接口**：定义类、函数原型、变量声明（使用extern关键字）
- **类型定义**：定义结构体、枚举、typedef等
- **宏定义**：定义常量、条件编译宏等
- **防止重复包含**：使用头文件保护宏（#ifndef #define #endif）

**示例：thread.h**
```cpp
#ifndef THREAD_H
#define THREAD_H

#include "utility.h"  // 包含其他头文件

// 类声明
class Thread {
private:
    char* name;                    // 线程名称
    void* stack;                   // 线程栈
    int status;                    // 线程状态
    
public:
    Thread(const char* debugName); // 构造函数声明
    ~Thread();                     // 析构函数声明
    void Fork(VoidFunctionPtr func, _int arg); // 线程创建函数声明
    void Yield();                  // 线程让步函数声明
};

// 外部变量声明（使用extern）
extern Thread* currentThread;      // 声明当前运行的线程

#endif // THREAD_H
```

**extern语法和作用：**

extern关键字用于声明一个变量或函数在其他文件中定义，告诉编译器在链接时查找这些符号。

**示例：**
- 在system.h中：`extern Thread* currentThread;` - 声明变量在其他地方定义
- 在system.cc中：`Thread* currentThread;` - 定义实际的变量
- 在其他需要使用currentThread的文件中：`#include "system.h"` - 使用extern声明

**源文件(.cc)的作用：**
- **定义实现**：实现头文件中声明的函数和方法
- **定义变量**：定义全局变量、静态变量
- **包含依赖**：通过#include包含必要的头文件

**示例：thread.cc**
```cpp
#include "thread.h"    // 必须包含对应的头文件
#include "scheduler.h" // 包含需要使用的其他模块
#include "interrupt.h"

// 实现Thread类的构造函数
Thread::Thread(const char* debugName) {
    name = (char*)debugName;
    stack = NULL;
    status = JUST_CREATED;
}

// 实现Thread类的Fork方法
void Thread::Fork(VoidFunctionPtr func, _int arg) {
    StackAllocate(func, arg);
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void) interrupt->SetLevel(oldLevel);
}
```

**优点：**
- **分离接口和实现**：头文件提供接口，源文件提供实现
- **降低耦合**：模块间只通过接口交互
- **提高编译效率**：只在头文件改变时重新编译相关文件
- **便于维护**：接口不变时可以修改实现

#### 2. Makefile和#ifdef实现模块化测试

**Makefile的模块化设计：**
- **模块化配置**：每个模块有独立的Makefile.local文件
- **条件编译**：通过DEFINES宏控制编译哪个功能模块
- **文件组织**：使用vpath指令管理不同目录下的源文件

**示例：threads/Makefile.local**
```makefile
# 定义源文件列表
CCFILES = main.cc \
          thread.cc \
          scheduler.cc \
          synch.cc \
          system.cc

# 定义编译宏
DEFINES += -DTHREADS

# 定义头文件搜索路径
INCPATH += -I../threads -I../machine
```

**#ifdef条件编译实现模块化：**
- **功能开关**：通过宏定义控制功能是否编译
- **代码隔离**：不同功能的代码被#ifdef-#endif块分隔
- **灵活配置**：根据编译时宏定义决定包含哪些功能

**示例：main.cc中的条件编译**
```cpp
#ifdef THREADS
    ThreadTest();
#endif
#ifdef USER_PROGRAM
    StartProcess(*(argv + 1));
#endif
```

**模块化测试的好处：**
- **分阶段开发**：可以先开发线程模块，再添加文件系统，最后添加网络
- **独立测试**：每个模块可以独立编译和测试
- **代码组织**：功能相关代码集中管理
- **可扩展性**：容易添加新功能模块

### 1. 线程模块 (threads)

thread文件夹下定义了线程管理和调度相关的核心代码

其中各个.cc文件分别做了：

| 文件名 | .h文件功能 | .cc文件功能 |
|--------|------------|-------------|
| main.cc | 定义程序入口点和命令行参数处理接口 | 实现main函数，引导操作系统内核，根据命令行参数执行不同功能，调用测试过程 |
| thread.cc | 声明Thread类，定义线程的基本属性和方法 | 实现Thread类的具体方法，包括线程创建、栈分配、执行、清理等核心功能 |
| scheduler.cc | 声明Scheduler类，定义线程调度器接口 | 实现线程调度算法，管理就绪队列，选择下一个要运行的线程 |
| synch.cc | 声明Lock、Semaphore、Condition等同步原语 | 实现锁、信号量、条件变量等同步机制，提供线程同步功能 |
| synchlist.cc | 声明SynchList类，提供同步链表接口 | 实现线程安全的链表，解决多线程访问共享链表的竞争条件 |
| system.cc | 声明系统全局变量和初始化函数接口 | 实现系统初始化函数，定义全局变量如currentThread、scheduler等 |
| utility.cc | 声明调试、断言等工具函数 | 实现调试输出、断言检查等辅助功能 |
| threadtest.cc | 无.h文件，仅用于测试 | 实现线程测试函数，创建多个线程演示线程调度和同步 |
| synchtest.cc | 无.h文件，仅用于测试 | 实现同步原语测试函数，测试锁和条件变量的使用 |
| interrupt.cc | 声明中断系统接口 | 实现中断管理功能，处理中断请求和调度 |
| sysdep.cc | 声明系统依赖函数接口 | 实现与具体系统相关的底层功能 |
| stats.cc | 声明统计信息类 | 实现系统运行统计功能，收集各种性能数据 |
| timer.cc | 声明定时器接口 | 实现定时器功能，提供时间片中断和时间管理 |

#### 1.1 Thread 类 - 进程/线程的实现

Thread 类是 Nachos 中线程的核心实现，虽然名为 Thread，但可以类比于操作系统中的进程概念。它定义了线程控制块，包含线程的基本信息和操作方法：

**Thread 类的构造函数**：
```cpp
Thread::Thread(const char* debugName)
{
    name = (char*)debugName;      // 设置线程名称，用于调试
    stackTop = NULL;              // 线程栈顶指针，初始化为 NULL
    stack = NULL;                 // 线程栈指针，初始化为 NULL
    status = JUST_CREATED;        // 线程状态，设置为"刚创建"
#ifdef USER_PROGRAM
    space = NULL;                 // 地址空间指针，只在支持用户程序时存在
#endif
}
```

**线程状态**：
- `JUST_CREATED`：刚创建，还未开始运行
- `RUNNING`：正在运行中
- `READY`：准备就绪，等待被调度
- `BLOCKED`：被阻塞，等待某些条件

**线程创建过程 (Fork)**：
Fork 是线程创建的核心方法，它不仅创建线程，还安排线程在未来某个时间点运行：
```cpp
void Thread::Fork(VoidFunctionPtr func, _int arg)
{
    StackAllocate(func, arg);  // 分配栈空间并设置初始状态
    
    IntStatus oldLevel = interrupt->SetLevel(IntOff);  // 关闭中断，防止竞争条件
    scheduler->ReadyToRun(this);  // 将线程放入就绪队列
    (void) interrupt->SetLevel(oldLevel);  // 恢复中断状态
}
```

**栈分配 (StackAllocate)**：
这是线程创建的关键部分，设置线程开始执行时的寄存器状态：
```cpp
machineState[PCState] = (_int) ThreadRoot;      // 程序计数器，指向线程入口函数
machineState[StartupPCState] = (_int) InterruptEnable;  // 启动时的程序计数器
machineState[InitialPCState] = (_int) func;     // 用户指定的函数地址
machineState[InitialArgState] = arg;            // 传递给函数的参数
machineState[WhenDonePCState] = (_int) ThreadFinish;  // 函数执行完毕后的地址
```

#### 1.2 Scheduler 类 - 调度器的实现

Scheduler（调度器）负责管理所有就绪线程，并选择下一个要运行的线程。它就像操作系统的进程调度器一样工作。

**就绪队列管理**：
```cpp
void Scheduler::ReadyToRun(Thread *thread)
{
    DEBUG('t', "ReadyToRun %s", thread->getName());
    
    thread->setStatus(READY);  // 设置线程状态为就绪
    readyList->Append((void *)thread);  // 将线程添加到就绪队列末尾
}
```

**选择下一个线程**：
```cpp
Thread *Scheduler::FindNextToRun()
{
    return (Thread *)readyList->Remove();  // 从就绪队列取出下一个线程
}
```
注意：Nachos 使用简单的 FIFO（先进先出）调度算法，这与操作系统考研中的先来先服务算法类似。

**线程切换 (Run)**：
这是调度器的核心功能，执行线程切换：
```cpp
void Scheduler::Run(Thread *nextThread)
{
    Thread *oldThread = currentThread;  // 保存当前线程
    
    // 如果当前线程还有用户程序空间，需要保存其状态
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {
        currentThread->SaveUserState();
        currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();  // 检查栈溢出
    currentThread = nextThread;  // 设置新线程为当前线程
    nextThread->setStatus(RUNNING);  // 设置新线程状态为运行
    
    // 执行底层的上下文切换，这是关键部分
    SWITCH(oldThread, nextThread);
    
    // 如果有线程需要销毁，现在销毁它
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }
    
    // 如果新线程有用户程序空间，恢复其状态
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {
        currentThread->RestoreUserState();
        currentThread->space->RestoreState();
    }
#endif
}
```

#### 1.3 中断机制的实现

中断是操作系统实现线程调度的重要机制，Nachos 通过 Interrupt 类模拟中断系统。

**中断类的主要功能**：
- 管理中断状态（开启/关闭）
- 处理定时中断
- 实现中断驱动的线程切换

**中断状态管理**：
```cpp
IntStatus Interrupt::SetLevel(IntStatus level)
{
    IntStatus old = level;  // 保存当前状态
    
    level = level;          // 设置新状态
    if (level == IntOff && pending) {  // 如果关闭中断且有待处理的中断
        Scheduler *oldScheduler = scheduler;  // 保存调度器
        currentThread->Yield();  // 让出 CPU，强制线程切换
    }
    return old;  // 返回旧状态
}
```

**定时中断处理**：
```cpp
static void TimerInterruptHandler(_int dummy)
{
    if (interrupt->getStatus() != IdleMode)  // 如果不是空闲模式
        interrupt->YieldOnReturn();  // 标记返回时需要线程切换
}
```
定时中断模拟了真实操作系统中的时钟中断，用于实现时间片轮转调度。

#### 1.4 进程同步机制

Nachos 提供了多种同步原语来解决多进程并发问题：

**锁 (Lock)**：
```cpp
class Lock {
private:
    char* name;          // 锁的名称
    bool value;          // 锁的状态（是否被占用）
    List *queue;         // 等待该锁的线程队列
    
public:
    Lock(const char* debugName);  // 构造函数
    ~Lock();                      // 析构函数
    void Acquire();               // 获取锁（P操作）
    void Release();               // 释放锁（V操作）
    bool isHeldByCurrentThread(); // 检查当前线程是否持有锁
};
```

**信号量 (Semaphore)**：
```cpp
class Semaphore {
private:
    char* name;          // 信号量名称
    int value;           // 信号量值
    List *queue;         // 等待队列
    
public:
    Semaphore(const char* debugName, int initialValue);
    ~Semaphore();
    void P();            // P操作（等待）
    void V();            // V操作（信号）
};
```

**条件变量 (Condition)**：
```cpp
class Condition {
private:
    char* name;          // 条件变量名称
    List *queue;         // 等待队列
    
public:
    Condition(const char* debugName);
    ~Condition();
    void Wait(Lock *conditionLock);      // 等待条件
    void Signal(Lock *conditionLock);    // 唤醒一个等待线程
    void Broadcast(Lock *conditionLock); // 唤醒所有等待线程
};
```

#### 1.5 线程切换的底层实现

线程切换通过汇编代码实现，这是系统级编程的关键部分：
- 当 `SWITCH` 函数被调用时，它会保存当前线程的寄存器状态
- 恢复下一个线程的寄存器状态
- 跳转到下一个线程的执行位置

这个过程模拟了真实操作系统中的上下文切换，是实现多线程并发执行的基础。

**线程运行机制**：
在Nachos中，每个线程有自己的执行栈和寄存器状态。线程的运行和切换通过底层的汇编代码和调度器共同实现：

```cpp
Thread *t = new Thread("thread_name");  // 创建线程对象
t->Fork(func, arg);  // 准备执行 func(arg)
```

当调用`Fork`时，主要做了以下几件事：
1.准备线程栈：在`StackAllocate(func, arg)`中分配栈空间并初始化
2.设置寄存器状态：
```cpp
machineState[PCState] = (_int) ThreadRoot;      // 程序计数器指向 ThreadRoot
machineState[StartupPCState] = (_int) InterruptEnable;  // 启动时执行的函数
machineState[InitialPCState] = (_int) func;     // 用户指定的函数
machineState[InitialArgState] = arg;            // 用户参数
machineState[WhenDonePCState] = (_int) ThreadFinish;  // 结束时执行的函数
```
3.放入就绪队列：调用`scheduler->ReadyToRun(this)`将线程放入就绪队列

**线程切换的核心：SWITCH汇编函数**
`SWITCH`函数是线程切换的关键，它用汇编语言实现，直接操作CPU寄存器：

保存旧线程状态：
```assembly
movl    %eax,_eax_save          # 临时保存 eax 寄存器
movl    4(%esp),%eax            # 从栈中取出 oldThread 指针
movl    %ebx,_EBX(%eax)         # 保存当前线程的所有寄存器
movl    %ecx,_ECX(%eax)         # ...
movl    %edx,_EDX(%eax)         # ...
movl    %esi,_ESI(%eax)         # ...
movl    %edi,_EDI(%eax)         # ...
movl    %ebp,_EBP(%eax)         # ...
movl    %esp,_ESP(%eax)         # 保存栈指针（关键！）
movl    _eax_save,%ebx          # 恢复先前保存的 eax 值
movl    %ebx,_EAX(%eax)         # 保存 eax 寄存器
movl    0(%esp),%ebx            # 从栈顶取出返回地址
movl    %ebx,_PC(%eax)          # 保存程序计数器（关键！）
```

恢复新线程状态：
```assembly
movl    8(%esp),%eax            # 从栈中取出 newThread 指针
movl    _EAX(%eax),%ebx         # 获取新线程的 eax 寄存器值
movl    %ebx,_eax_save          # 临时保存
movl    _EBX(%eax),%ebx         # 恢复新线程的 ebx 寄存器
movl    _ECX(%eax),%ecx         # 恢复新线程的 ecx 寄存器
movl    _EDX(%eax),%edx         # 恢复新线程的 edx 寄存器
movl    _ESI(%eax),%esi         # 恢复新线程的 esi 寄存器
movl    _EDI(%eax),%edi         # 恢复新线程的 edi 寄存器
movl    _EBP(%eax),%ebp         # 恢复新线程的 ebp 寄存器
movl    _ESP(%eax),%esp         # 恢复新线程的栈指针（关键！）
movl    _PC(%eax),%eax          # 恢复新线程的程序计数器（关键！）
movl    %eax,0(%esp)            # 将返回地址放回栈顶
movl    _eax_save,%eax          # 恢复新线程的 eax 寄存器
ret                             # 返回，跳转到新线程的程序计数器位置
```

关键机制：
- 栈指针切换（_ESP）：每个线程有自己的栈空间，通过切换栈指针实现栈的切换
- 程序计数器切换（_PC）：决定CPU下一条执行的指令，通过切换PC实现代码执行位置的切换

**线程执行流程**：
当线程被调度运行时，流程如下：
1.调度器调用`scheduler->Run(新线程)`
2.执行`SWITCH(旧线程, 新线程)`进行上下文切换
3.CPU现在执行新线程，从`ThreadRoot`函数开始执行
4.`ThreadRoot`按顺序执行：启用中断 → 调用用户函数 → 结束线程

```assembly
ThreadRoot:
        pushl   %ebp              # 设置栈帧
        movl    %esp,%ebp         # 
        pushl   InitialArg        # 压入参数
        call    *StartupPC        # 调用 InterruptEnable
        call    *InitialPC        # 调用用户函数（如 Producer）
        call    *WhenDonePC       # 调用 ThreadFinish
```

#### 1.6 **Timer系统实现**

Timer系统是Nachos中实现时间片轮转调度的关键组件，它模拟了真实操作系统中的硬件定时器功能。

Timer类的主要功能是模拟硬件定时器，定期产生中断来实现线程的时间片轮转。

```cpp
class Timer {
private:
    bool randomize;               // 是否使用随机时间间隔
    VoidFunctionPtr handler;      // 中断处理函数指针
    _int arg;                    // 传递给中断处理函数的参数
    
public:
    Timer(VoidFunctionPtr timerHandler, _int callArg, bool doRandom);
    ~Timer() {}
    void TimerExpired();         // 定时器过期时调用
    int TimeOfNextInterrupt();   // 计算下次中断时间
};
```

Timer构造函数：
```cpp
Timer::Timer(VoidFunctionPtr timerHandler, _int callArg, bool doRandom)
{
    randomize = doRandom;
    handler = timerHandler;  // 存储中断处理函数（如 TimerInterruptHandler）
    arg = callArg;          // 存储参数

    // 安排第一次定时器中断
    interrupt->Schedule(TimerHandler, (_int) this, TimeOfNextInterrupt(), TimerInt); 
}
```

TimerExpired方法：
```cpp
void Timer::TimerExpired() 
{
    // 安排下一次定时器中断
    interrupt->Schedule(TimerHandler, (_int) this, TimeOfNextInterrupt(), TimerInt);

    // 调用实际的中断处理函数
    (*handler)(arg);
}
```

TimeOfNextInterrupt方法：
```cpp
int Timer::TimeOfNextInterrupt() 
{
    if (randomize)
        return 1 + (Random() % (TimerTicks * 2));  // 随机间隔
    else
        return TimerTicks;  // 固定间隔（默认100个时钟周期）
}
```

在stats.h中定义：
```cpp
#define TimerTicks 	100    	// (平均) 定时器中断间隔时间
```

这意味着默认情况下，每100个时钟周期产生一次定时器中断，这对应于真实操作系统中的时间片概念。

**SystemTick 和时间推进机制**：
SystemTick是Nachos中时间管理的核心概念，它定义了系统操作的时间单位。

在stats.h中：
```cpp
#define UserTick 	1	// 用户级指令执行时间：1个单位
#define SystemTick 	10 	// 中断使能时推进时间：10个单位
```

设计理念：
- `UserTick = 1`：模拟用户代码执行相对较快
- `SystemTick = 10`：模拟系统调用/中断处理相对耗时更多

时间推进主要在interrupt.cc的OneTick()方法中实现：
```cpp
void Interrupt::OneTick()
{
    // 推进总时钟和系统时钟
    stats->totalTicks += SystemTick;   // 总时间增加SystemTick（10）
    stats->systemTicks += SystemTick;  // 系统时间增加SystemTick（10）
    
    // 检查是否有定时中断到达
    CheckIfDue(true);
}
```

时间推进不是自动的，而是在系统的关键操作中主动进行：
- 中断状态改变时：当调用`SetLevel`时会推进时间
- 线程操作时：`Yield()`等操作会推进时间

与真实操作系统不同，Nachos的时间推进是主动的：
- 真实系统：硬件定时器自动产生中断
- Nachos：通过在系统调用中插入时间推进代码来模拟

这种设计是因为Nachos运行在用户空间，无法直接访问硬件定时器，必须通过软件方式模拟。



#### 1.7 **中断队列管理机制**

Nachos使用统一的中断队列管理所有类型的模拟硬件中断。

中断系统维护一个待处理中断的优先队列，中断类型包括：
- TimerInt：定时器中断
- DiskInt：磁盘中断
- ConsoleReadInt：控制台读中断
- ConsoleWriteInt：控制台写中断
- NetworkRecvInt：网络接收中断
- NetworkSendInt：网络发送中断
- SoftwareInt：软件中断

```cpp
struct PendingInterrupt {
    VoidFunctionPtr handler;  // 中断处理函数
    _int arg;                 // 传递给处理函数的参数
    int when;                 // 中断触发时间
    IntType type;             // 中断类型
};
```

Schedule方法：
```cpp
void Interrupt::Schedule(VoidFunctionPtr handler, _int arg, int when, IntType type)
{
    // 计算中断发生的时间点
    int timeToInterrupt = stats->totalTicks + when;
    
    // 创建待处理中断对象
    PendingInterrupt *toSchedule = new PendingInterrupt(handler, arg, timeToInterrupt, type);
    
    // 按时间排序插入队列
    pending->SortedInsert(toSchedule, timeToInterrupt);
}
```

所有类型的中断都在同一个`CheckIfDue`方法中处理：
```cpp
void Interrupt::CheckIfDue(bool advanceClock)
{
    if (!pending->Empty()) {
        PendingInterrupt *next = (PendingInterrupt*)pending->Front();
        if (next->when <= stats->totalTicks) {  // 如果到了中断时间
            // 执行对应的处理函数（不管是什么类型的中断）
            (*next->handler)(next->arg);
            // 从队列中移除
            (void) pending->Remove();
        }
    }
}
```

这种设计使得定时器、磁盘、控制台、网络等各种设备都使用相同的中断队列和处理机制，形成统一的中断管理系统。所有中断都存储在同一个`pending`队列中，按时间排序并统一处理，这提供了一个统一的中断管理系统，使得Nachos能够模拟真实操作系统中的中断驱动调度，实现线程的时间片轮转和抢占式调度。

### 2. 机器模拟模块 (machine)

machine文件夹下定义了硬件模拟和底层系统接口

其中各个.cc文件分别做了：

| 文件名 | .h文件功能 | .cc文件功能 |
|--------|------------|-------------|
| console.cc | 声明控制台输入输出接口 | 实现控制台模拟，处理输入输出中断 |
| disk.cc | 声明磁盘接口 | 实现磁盘模拟，处理磁盘读写操作 |
| interrupt.cc | 声明中断系统接口 | 实现中断管理功能，处理中断请求和调度 |
| machine.cc | 声明MIPS机器模拟接口 | 实现MIPS处理器模拟，执行MIPS指令 |
| mipssim.cc | 声明MIPS处理器模拟接口 | 实现MIPS指令的具体执行逻辑 |
| network.cc | 声明网络接口 | 实现网络模拟，处理网络通信 |
| stats.cc | 声明统计信息类 | 实现系统运行统计功能，收集各种性能数据 |
| sysdep.cc | 声明系统依赖函数接口 | 实现与具体系统相关的底层功能 |
| timer.cc | 声明定时器接口 | 实现定时器功能，提供时间片中断和时间管理 |
| translate.cc | 声明地址转换接口 | 实现虚拟地址到物理地址的转换 |

#### 2.1 Machine 类 - MIPS处理器模拟实现

Machine 类模拟了MIPS处理器的功能，这是Nachos的核心硬件抽象层：

**Machine 类的主要功能**：
- 模拟MIPS处理器的寄存器和内存
- 执行MIPS指令
- 处理内存访问异常
- 实现地址翻译机制

**寄存器管理**：
```cpp
class Machine {
public:
    // MIPS处理器寄存器
    int registers[NumTotalRegs];  // 32个通用寄存器 + PC, NextPC, PrevPC等
    
    // 内存管理
    char *mainMemory;             // 模拟的物理内存
    TranslationEntry *pageTable;  // 页表，用于虚拟地址翻译
    
    // 处理器状态
    bool singleStep;              // 单步执行模式
};
```

**内存访问**：
```cpp
bool Machine::ReadMem(int addr, int size, int* value) {
    // 验证地址是否有效
    if (!isValidAddr(addr, size)) {
        return FALSE;  // 地址无效，产生异常
    }
    
    // 根据页表翻译虚拟地址到物理地址
    int physAddr = translate(addr);
    if (physAddr == -1) {
        return FALSE;  // 地址翻译失败，产生异常
    }
    
    // 根据大小读取数据（1、2、4字节）
    switch(size) {
        case 1: *value = *(unsigned char*)&mainMemory[physAddr]; break;
        case 2: *value = *(unsigned short*)&mainMemory[physAddr]; break;
        case 4: *value = *(unsigned int*)&mainMemory[physAddr]; break;
    }
    
    return TRUE;
}
```
这模拟了真实操作系统中的内存管理单元（MMU）功能，将虚拟地址翻译为物理地址。

**MIPS指令执行**：
mipssim.cc 文件实现了MIPS指令的解释执行：
```cpp
void Machine::Run() {
    // 主循环：取指 -> 译码 -> 执行
    while (!interrupt->CheckIfHalt()) {
        // 从PC指向的地址获取指令
        int pc = registers[PCReg];
        int instr = ReadInstruction(pc);
        
        // 更新PC寄存器
        registers[PrevPCReg] = registers[PCReg];
        registers[PCReg] = registers[NextPCReg];
        registers[NextPCReg] += 4;  // MIPS指令长度为4字节
        
        // 译码并执行指令
        ExecuteInstruction(instr, FALSE);
        
        // 检查是否有中断需要处理
        interrupt->CheckIfDue(TRUE);
    }
}
```
这模拟了真实处理器的取指、译码、执行周期。

#### 2.2 控制台模拟 (console.cc)

控制台模拟实现了输入输出设备的模拟：

**控制台类**：
```cpp
class Console {
private:
    char *readFile;      // 输入文件
    char *writeFile;     // 输出文件
    ConsoleInput *in;    // 输入设备
    ConsoleOutput *out;  // 输出设备
    
public:
    Console(char *readFile, char *writeFile, VoidFunctionPtr readAvail, 
            VoidFunctionPtr writeDone, _int callArg);
    void PutChar(char ch);  // 输出字符
    char GetChar();         // 输入字符
};
```

**中断驱动**：
```cpp
void Console::PutChar(char ch) {
    // 将字符写入输出设备
    write(to, &ch, sizeof(char));  // 真实系统的系统调用
    
    // 模拟完成时间
    interrupt->Schedule(ConsoleWriteDone, console, 
                        ConsoleTime, ConsoleWriteInt);
}

static void ConsoleWriteDone(_int arg) {
    Console *console = (Console*)arg;
    // 调用回调函数，通知写操作完成
    if (console->writeHandler != NULL)
        console->writeHandler(console->handlerArg);
}
```
这模拟了真实系统中I/O操作的异步特性，当I/O设备完成操作时会触发中断。

#### 2.3 定时器实现 (timer.cc)

定时器是操作系统实现时间片轮转调度的关键组件：

**定时器类**：
```cpp
class Timer {
private:
    int wakeUpTime;   // 下一次唤醒时间
    bool randomYield; // 是否随机触发线程切换
    
public:
    Timer(int timeToWakeUp, bool doRandomYield);
};
```

**定时器调度**：
```cpp
Timer::Timer(int timeToWakeUp, bool doRandomYield) {
    randomize = doRandomYield;
    // 安排第一次定时中断
    interrupt->Schedule(&TimerHandler, 0, timeToWakeUp, TimerInt);
}

static void TimerHandler(int dummy) {
    if (doRandomYield) {
        if (stats->Random() > TimerTicks) {
            interrupt->YieldOnReturn();  // 标记返回时需要线程切换
        }
    }
    // 安排下一次中断
    interrupt->Schedule(&TimerHandler, 0, TimerTicks, TimerInt);
}
```
定时器定期触发中断，这对应于真实操作系统中的时钟中断，是实现抢占式调度的基础。

### 3. 用户程序模块 (userprog)

userprog文件夹下定义了用户程序加载和执行相关功能

其中各个.cc文件分别做了：

| 文件名 | .h文件功能 | .cc文件功能 |
|--------|------------|-------------|
| addrspace.cc | 声明地址空间管理接口 | 实现用户程序的虚拟内存管理 |
| exception.cc | 声明异常处理接口 | 实现系统调用和异常处理机制 |
| progtest.cc | 声明程序测试接口 | 实现用户程序加载和运行测试 |
| syscall.h | 定义系统调用接口 | 定义用户程序可用的系统调用 |

#### 3.1 地址空间管理 (addrspace.cc)

地址空间是操作系统中重要的抽象，实现进程间的内存隔离：

**AddressSpace 类**：
```cpp
class Addrspace {
private:
    TranslationEntry *pageTable;     // 页表
    unsigned int numPages;           // 页面数量
    int spaceId;                     // 地址空间ID
    
public:
    Addrspace(OpenFile *executable);  // 构造函数，从可执行文件创建地址空间
    ~Addrspace();                     // 析构函数，释放地址空间
    void InitRegisters();             // 初始化处理器寄存器
    void SaveState();                 // 保存地址空间状态
    void RestoreState();              // 恢复地址空间状态
};
```

**创建地址空间**：
```cpp
Addrspace::Addrspace(OpenFile *executable) {
    // 1. 读取可执行文件头，获取代码段和数据段信息
    NoffHeader noffH;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    
    // 2. 计算需要的页面数
    unsigned int size = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    numPages = divRoundUp(size, PageSize);  // 以页面大小为单位向上取整
    
    // 3. 为页表分配空间
    pageTable = new TranslationEntry[numPages];
    
    // 4. 初始化页表项
    for (unsigned int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;      // 虚拟页号
        pageTable[i].physicalPage = -1;    // 物理页号（还未分配）
        pageTable[i].valid = FALSE;        // 无效页
        pageTable[i].use = FALSE;          // 未使用
        pageTable[i].dirty = FALSE;        // 未修改
        pageTable[i].readOnly = FALSE;     // 可读写
    }
    
    // 5. 加载代码段和数据段到内存
    // 代码段加载
    if (noffH.code.size > 0) {
        // 为代码段分配物理页并加载内容
        executable->ReadAt(
            &(machine->mainMemory[pageTable[0].physicalPage * PageSize]),
            noffH.code.size, noffH.code.inFileAddr);
    }
}
```
这个过程模拟了真实操作系统中的程序加载过程，包括地址空间创建、内存分配和程序代码加载。

#### 3.2 系统调用和异常处理 (exception.cc)

异常处理是用户程序与操作系统内核交互的主要方式：

**异常处理函数**：
```cpp
void ExceptionHandler(ExceptionType which) {
    int type = machine->registers[2];  // 系统调用类型在寄存器2中
    
    switch(which) {
        case SyscallException:  // 系统调用异常
            switch(type) {
                case SC_Halt:  // halt系统调用
                    DEBUG('a', "Shutdown, initiated by user program.\n");
                    interrupt->Halt();
                    break;
                    
                case SC_Exit:  // exit系统调用
                    ExitProcess(machine->registers[4]);  // 退出参数在寄存器4
                    break;
                    
                case SC_Exec:  // exec系统调用
                    ExecProcess((char*)machine->registers[4]);
                    break;
                    
                case SC_Join:  // join系统调用
                    JoinProcess(machine->registers[4]);
                    break;
                    
                // 其他系统调用...
            }
            break;
            
        case PageFaultException:  // 页面错误异常
            // 处理页面错误，可能是缺页中断
            handlePageFault(machine->registers[BadVAddrReg]);
            break;
            
        case ReadOnlyException:  // 只读错误异常
            // 尝试写入只读页面
            printf("Write to read-only page!\n");
            interrupt->Halt();
            break;
    }
}
```

**系统调用实现**：
```cpp
void ExitProcess(_int exitCode) {
    // 释放当前进程的资源
    delete currentThread->space;
    currentThread->Finish();  // 结束当前线程
    
    // 恢复用户程序寄存器
    machine->registers[2] = exitCode;  // 返回值
    return;
}
```
这展示了用户程序如何通过系统调用请求操作系统服务，如程序终止、进程创建等。

### 4. 文件系统模块 (filesys)

filesys文件夹下定义了文件系统相关功能

其中各个.cc文件分别做了：

| 文件名 | .h文件功能 | .cc文件功能 |
|--------|------------|-------------|
| directory.cc | 声明目录管理接口 | 实现目录操作，如创建、删除、查找文件 |
| filehdr.cc | 声明文件头管理接口 | 实现文件头信息管理，包含文件的元数据 |
| filesys.cc | 声明文件系统接口 | 实现整个文件系统的管理功能 |
| fstest.cc | 无.h文件，仅用于测试 | 实现文件系统测试函数 |
| openfile.cc | 声明打开文件接口 | 实现文件的打开、读写、关闭操作 |
| synchdisk.cc | 声明同步磁盘接口 | 实现同步磁盘访问，提供线程安全的磁盘操作 |

#### 4.1 文件系统整体架构

Nachos文件系统采用了类似UNIX的设计但做了简化，包含以下核心组件：

**磁盘布局结构**：
文件系统将磁盘划分为固定大小的扇区，通常每个扇区128字节，具体布局如下：
扇区0存储空闲空间位图的文件头，扇区1存储根目录的文件头，后续扇区分别存储位图数据、根目录数据和普通文件数据。

**文件系统类**：

```cpp
class FileSystem {
private:
    OpenFile *freeMapFile;   // 空闲块位图文件
    OpenFile *directoryFile; // 目录文件
    BitMap *freeMap;         // 空闲块位图
    Directory *directory;    // 目录
    
public:
    FileSystem(bool format);  // 构造函数，可选择格式化磁盘
    bool Create(char *name);  // 创建文件
    OpenFile* Open(char *name);  // 打开文件
    bool Remove(char *name);  // 删除文件
};
```

#### 4.2 文件头(FileHeader)实现

文件头是文件系统的核心数据结构，类似UNIX的i-node：

**文件头结构**：
```cpp
class FileHeader {
private:
    int numSectors;      // 占用的扇区数
    int fileSize;        // 文件大小
    int dataSectors[MaxFileSize/SectorSize];  // 数据扇区号数组
    
public:
    bool Allocate(BitMap *bitMap, int fileSize);  // 分配磁盘块
    void Deallocate(BitMap *bitMap);              // 释放磁盘块
    int WriteAt(char *from, int numBytes, int position);  // 写入数据
    int ReadAt(char *to, int numBytes, int position);     // 读取数据
};
```

**与真实系统的区别**：
Nachos只使用直接指针没有间接指针，这限制了文件最大大小约为4KB，而真实UNIX系统使用直接、单间接、双间接和三间接指针支持大文件。

#### 4.3 目录管理实现

目录在Nachos中实际上是一个特殊的文件：

**目录类结构**：
```cpp
class Directory {
private:
    int tableSize;           // 目录表大小
    DirectoryEntry *table;   // 目录项数组
    
public:
    Directory(int size);     // 构造函数
    ~Directory();            // 析构函数
    void FetchFrom(OpenFile *file);  // 从磁盘加载目录
    void WriteBack(OpenFile *file);  // 将目录写回磁盘
    int Find(char *name);            // 查找文件
    bool Add(char *name, int newSector);  // 添加文件
    bool Remove(char *name);              // 删除文件
};

// 目录项结构
struct DirectoryEntry {
    char name[FileNameMaxLen];  // 文件名(最多9个字符)
    int sector;                 // 对应文件头的扇区号
};
```

**目录特点**：
Nachos只实现了单级目录没有子目录结构，每个目录项包含是否使用标志、扇区号和最多9个字符的文件名。

#### 4.4 磁盘空间管理

磁盘空间管理使用位图(Bitmap)跟踪磁盘空间的使用情况：

**位图工作原理**：
位图的每一位对应一个磁盘扇区，1表示已使用0表示空闲，分配空间时查找连续的0位删除文件时将对应位设为0。

**位图类结构**：
```cpp
class BitMap {
private:
    int numBits;         // 总位数
    unsigned char *bits;  // 位图数据
    
public:
    BitMap(int nbits);           // 构造函数
    ~BitMap();                   // 析构函数
    void Mark(int which);        // 标记位为已使用
    void Clear(int which);       // 清除位为空闲
    bool Test(int which);        // 测试位状态
    int Find();                  // 查找第一个空闲位
    int NumClear();              // 返回空闲位数
};
```

#### 4.5 同步机制实现

由于磁盘是异步设备请求立即返回操作完成后产生中断，Nachos使用信号量实现同步：

**同步磁盘类**：
```cpp
class SynchDisk {
private:
    Disk *disk;           // 原始磁盘设备
    Semaphore *semaphore; // 将请求线程与中断处理程序同步
    Lock *lock;           // 确保一次只能向磁盘发送一个读/写请求
    
public:
    SynchDisk(const char* name);    // 初始化同步磁盘
    ~SynchDisk();                   // 释放同步磁盘数据
    void ReadSector(int sectorNumber, char* data);  // 读取扇区
    void WriteSector(int sectorNumber, char* data); // 写入扇区
    void RequestDone();             // 由磁盘中断处理程序调用
};
```

**同步过程**：
发出磁盘请求后等待信号量，磁盘中断处理程序发出信号量，使用锁确保同一时间只有一个磁盘操作。

#### 4.6 文件操作流程

**创建文件(Create)流程**：
1. 检查文件名是否已存在
2. 在位图中查找空闲扇区用于文件头
3. 为文件数据分配空间
4. 在目录中添加新条目
5. 将文件头写入磁盘
6. 更新位图和目录

**打开文件(Open)流程**：
1. 在目录中查找文件名
2. 读取文件头到内存
3. 创建OpenFile对象，设置读写位置为0

**读写文件流程**：
1. 根据读写位置计算对应的扇区号
2. 读取整个扇区到缓冲区
3. 从缓冲区中复制需要的数据
4. 更新读写位置

#### 4.7 文件系统限制

Nachos文件系统有以下教学目的限制：

**功能限制**：
文件大小固定创建时确定，最大文件约4KB无间接指针，只支持单级目录，最大文件数量有限目录大小固定。

**并发限制**：
没有文件权限和访问控制，不支持并发访问，没有错误恢复机制。

**设计目的**：
这种设计让学习者能够理解文件系统的核心概念而不必处理真实系统的复杂性，是操作系统教学中的经典简化实现。

### 5. 网络模块 (network)

network文件夹下定义了网络通信相关功能

其中各个.cc文件分别做了：

| 文件名 | .h文件功能 | .cc文件功能 |
|--------|------------|-------------|
| network.cc | 声明网络接口 | 实现网络模拟，处理数据包发送和接收 |
| post.cc | 声明邮局接口 | 实现网络消息传递机制，模拟邮件系统 |

#### 5.1 网络模拟 (network.cc)

网络模块模拟了网络通信的基本功能：

**网络类**：
```cpp
class Network {
private:
    NetworkAddress ident;        // 本机网络地址
    double reliability;          // 网络可靠性（0.0-1.0）
    double orderability;         // 网络顺序性（0.0-1.0）
    int sock;                    // UNIX套接字
    char *noff_file;             // 网络配置文件
    
public:
    Network(NetworkAddress addr, double reliability, double orderability,
            char* reliabilityFile);
    ~Network();                  // 析构函数
    void Send(PacketHeader hdr, char* data);  // 发送数据包
    PacketHeader Receive(char* data);         // 接收数据包
};
```

**数据包结构**：
```cpp
struct PacketHeader {
    int to;      // 目标机器ID
    int from;    // 源机器ID
    int length;  // 数据长度
};
```

**发送过程**：
```cpp
void Network::Send(PacketHeader hdr, char* data) {
    // 将数据包打包
    int totalLen = hdr.length + sizeof(hdr);
    char* packet = new char[totalLen];
    
    // 复制包头
    memcpy(packet, &hdr, sizeof(hdr));
    // 复制数据
    memcpy(packet + sizeof(hdr), data, hdr.length);
    
    // 通过套接字发送
    send(sock, packet, totalLen, 0);
    
    // 调度接收中断
    interrupt->Schedule(NetworkSendDone, (_int)this, 
                        NetworkTime, NetworkSendInt);
    
    delete [] packet;
}
```
这模拟了真实网络通信中的数据包发送过程。

#### 5.2 邮局系统 (post.cc)

邮局系统实现了进程间通信的高级抽象：

**邮局类**：
```cpp
class PostOffice {
private:
    Network *network;            // 底层网络
    NetworkAddress netAddr;      // 网络地址
    
    // 同步机制
    Semaphore *messageAvailable; // 有消息可用信号量
    Semaphore *messageSent;      // 消息已发送信号量
    
    // 邮箱（用于接收消息）
    PacketHeader inHdr;          // 输入包头
    char inPacket[MaxPacketSize]; // 输入数据包
    
public:
    PostOffice(NetworkAddress addr, double reliability, double orderability,
               int nMails);
    ~PostOffice();
    void Send(PacketHeader to, char* data);  // 发送消息
    PacketHeader Receive(char* data);        // 接收消息
};
```

**消息接收**：
```cpp
PacketHeader PostOffice::Receive(char* data) {
    // 等待消息到达
    messageAvailable->P();
    
    // 复制消息数据
    memcpy(data, inPacket, inHdr.length);
    PacketHeader returnHdr = inHdr;
    
    // 通知可以接收下一个消息
    messageSent->V();
    
    return returnHdr;
}
```
这实现了进程间的消息传递机制，是分布式系统的基础。

### 6. 管程模块 (monitor)

monitor文件夹下定义了管程（Monitor）相关的同步机制实现

其中各个.cc文件分别做了：

| 文件名 | .h文件功能 | .cc文件功能 |
|--------|------------|-------------|
| main.cc | 定义程序入口点和命令行参数处理接口 | 实现main函数，引导操作系统内核，根据命令行参数执行不同功能，调用生产者消费者测试过程 |
| prodcons++.cc | 无.h文件，仅用于测试 | 实现生产者-消费者问题的解决方案，使用管程和信号量同步机制 |
| ring.cc/ring.h | 声明环形缓冲区接口 | 实现环形缓冲区数据结构，供生产者消费者使用 |
| synch.cc/synch.h | 声明同步原语接口 | 实现锁、信号量、条件变量等同步机制 |
| 其他文件 | 与threads模块相同 | 与线程模块共享的基础设施代码 |

#### 6.1 管程的概念和实现

管程（Monitor）是操作系统中用于解决并发控制问题的高级同步机制。与信号量相比，管程提供了一种更高级、更安全的同步方式：

**管程的特性**：

- **互斥访问**：管程内的共享变量只能被一个线程访问
- **条件变量**：提供等待/通知机制，允许线程在条件不满足时等待
- **封装性**：将共享数据和操作封装在一起

**在monitor模块中的应用**：
```cpp
// 在prodcons++.cc中，使用信号量实现管程功能
Semaphore *nempty;  // 空槽位数量（管程中的条件变量）
Semaphore *nfull;   // 满槽位数量（管程中的条件变量）
Semaphore *mutex;   // 互斥锁（管程的互斥访问机制）
```

#### 6.2 生产者-消费者问题的实现

monitor模块演示了经典的生产者-消费者问题：

**生产者函数**：
```cpp
void Producer(_int which)
{
    int num;
    slot *message = new slot(0,0);

    for (num = 0; num < N_MESSG ; num++) {
        message->thread_id = which;  // 设置生产者ID
        message->value = num;        // 设置消息值

        // 等待空槽位（条件变量wait操作）
        nempty->P();     // 等待空槽位
        mutex->P();      // 获取互斥锁
        ring->Put(message);  // 放入消息
        mutex->V();      // 释放互斥锁
        nfull->V();      // 通知满槽位
    }
}
```

**消费者函数**：
```cpp
void Consumer(_int which)
{
    char str[MAXLEN];
    char fname[LINELEN];
    int fd;
    
    slot *message = new slot(0, 0);
    sprintf(fname, "tmp_%d", which);  // 生成输出文件名

    // 创建输出文件
    if ( (fd = creat(fname, 0600) ) == -1) {
        perror("creat: file create failed");
        exit(1);
    }
    
    for (; ; ) {
        // 等待满槽位（条件变量wait操作）
        nfull->P();      // 等待满槽位
        mutex->P();      // 获取互斥锁
        ring->Get(message);  // 获取消息
        mutex->V();      // 释放互斥锁
        nempty->V();     // 通知空槽位

        // 将消息写入文件
        sprintf(str,"producer id --> %d; Message number --> %d;\n", 
                message->thread_id, message->value);
        if ( write(fd, str, strlen(str)) == -1 ) {
            perror("write: write failed");
            exit(1);
        }
    }
}
```

**初始化过程**：
```cpp
void ProdCons()
{
    // 创建三个同步信号量
    nempty = new Semaphore("nempty", BUFF_SIZE);  // 空槽位计数
    nfull = new Semaphore("nfull", 0);            // 满槽位计数
    mutex = new Semaphore("mutex", 1);            // 互斥锁

    // 创建环形缓冲区
    ring = new Ring(BUFF_SIZE);

    // 创建并启动生产者线程
    for (int i=0; i < N_PROD; i++) {
        sprintf(prod_names[i], "producer_%d", i);
        producers[i] = new Thread(prod_names[i]);
        producers[i]->Fork(Producer, i);
    }

    // 创建并启动消费者线程
    for (int i=0; i < N_CONS; i++) {
        sprintf(cons_names[i], "consumer_%d", i);
        consumers[i] = new Thread(cons_names[i]);
        consumers[i]->Fork(Consumer, i);
    }
}
```

这个实现展示了如何使用同步原语（信号量）来解决生产者-消费者问题，这与操作系统考研中的经典同步问题完全对应。

## Nachos系统启动和运行流程

### Nachos系统本质说明

Nachos **不是一个真正的操作系统**，而是一个**操作系统教学模拟器**。它运行在现有操作系统（如Linux）之上，而不是直接运行在硬件上。

**与真实操作系统的区别：**
- **真实操作系统（如Linux）**：BIOS/UEFI → Bootloader → Kernel → systemd/init → Shell
- **Nachos模拟器**：宿主系统（Linux）→ `./nachos` 程序 → Nachos内核模拟

Nachos运行在现有操作系统之上，作为一个普通进程执行，模拟操作系统功能而非直接控制硬件。它不是用来替代systemd或bash的，而是用来学习操作系统内部工作原理的教学工具。

### 1. 系统启动阶段

#### 1.1 main函数入口
- **函数位置**: `main.cc`
- **参数处理**: 接收命令行参数 `argc` 和 `argv`
- **调试输出**: `DEBUG('t', "Entering main")` 输出调试信息
- **系统初始化**: 调用 `(void) Initialize(argc, argv)` 进行系统初始化

#### 1.2 Initialize函数执行流程
- **参数解析**: 解析命令行参数（如 `-d`, `-rs`, `-s`, `-f`, `-n` 等）
- **调试系统初始化**: `DebugInit(debugArgs)` 初始化调试系统
- **统计系统创建**: `stats = new Statistics()` 创建统计对象
- **中断系统创建**: `interrupt = new Interrupt` 创建中断对象
- **调度器创建**: `scheduler = new Scheduler()` 创建调度器对象
- **定时器创建（可选）**: `timer = new Timer(TimerInterruptHandler, 0, randomYield)` 创建定时器
- **主线程创建**: `currentThread = new Thread("main")` 创建当前线程对象
- **中断使能**: `interrupt->Enable()` 启用中断
- **用户程序支持（可选）**: `machine = new Machine(debugUserProg)` 创建机器模拟对象
- **文件系统创建（可选）**: `fileSystem = new FileSystem(format)` 创建文件系统对象
- **网络系统创建（可选）**: `postOffice = new PostOffice(netname, rely, order, 10)` 创建邮局对象

#### 1.3 线程测试执行（如果定义了THREADS宏）
- **ThreadTest()函数**: `ThreadTest()` 创建并运行线程测试
- **创建新线程**: `Thread *t = new Thread("forked thread")` 创建新线程
- **启动新线程**: `t->Fork(SimpleThread, 1)` 启动新线程执行SimpleThread函数
- **主线程执行**: `SimpleThread(0)` 主线程也执行SimpleThread函数

### 2. 系统运行阶段

#### 2.1 SimpleThread函数执行流程
- **循环执行**: 循环5次，每次打印线程信息
- **线程让步**: `currentThread->Yield()` 让出CPU控制权
- **上下文切换**: 通过调度器切换到其他就绪线程

#### 2.2 命令行参数处理
- **参数遍历**: `for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount)` 遍历所有参数
- **参数计数**: `argCount = 1` 初始化参数计数
- **版权显示**: `-z` 参数触发 `printf ("%s", copyright)` 显示版权信息
- **用户程序执行（可选）**: `-x` 参数触发 `StartProcess(*(argv + 1))` 启动用户程序
- **控制台测试（可选）**: `-c` 参数触发 `ConsoleTest()` 进行控制台测试
- **文件系统操作（可选）**: 
  - `-cp` 参数触发 `Copy()` 复制文件
  - `-p` 参数触发 `Print()` 打印文件
  - `-r` 参数触发 `fileSystem->Remove()` 删除文件
  - `-l` 参数触发 `fileSystem->List()` 列出目录
  - `-D` 参数触发 `fileSystem->Print()` 打印文件系统
  - `-t` 参数触发 `PerformanceTest()` 性能测试
- **网络测试（可选）**: `-o` 参数触发 `MailTest()` 网络邮件测试

### 3. 系统结束阶段

#### 3.1 主线程结束
- **线程完成**: `currentThread->Finish()` 标记主线程完成
- **防止返回**: 防止main函数直接返回，确保其他线程可以继续运行
- **程序退出**: `return(0)` 但实际不会执行到这一步

#### 3.2 系统清理（如果用户中断）
- **Cleanup函数**: 当用户按下Ctrl+C时调用 `Cleanup()` 函数
- **网络清理**: `delete postOffice` 删除网络对象
- **用户程序清理**: `delete machine` 删除机器对象
- **文件系统清理**: `delete fileSystem` 删除文件系统对象
- **磁盘清理**: `delete synchDisk` 删除同步磁盘对象
- **其他对象清理**: 删除timer、scheduler、interrupt等对象
- **程序退出**: `Exit(0)` 退出程序

### 4. 测试示例

#### 4.1 简单线程测试示例
在 `threads` 目录下运行：
```
./nachos
```
这将运行 `ThreadTest()` 函数，创建两个线程交替执行，演示线程调度和同步的基本概念。

#### 4.2 运行用户程序示例
在 `userprog` 目录下运行：
```
./nachos -x ../test/halt.noff
```
这将加载并执行 `halt.noff` 程序，该程序只是简单地调用 halt 系统调用终止自己。

#### 4.3 文件系统操作示例
在 `filesys` 目录下运行：
```
./nachos -l          # 列出文件系统中的所有文件
./nachos -cp ../test/halt.c halt.c  # 复制文件到 Nachos 文件系统
./nachos -p halt.c   # 打印文件内容
./nachos -r halt.c   # 删除文件
```

#### 4.4 并行计算示例
在 `demo1` 目录下运行生产者-消费者演示：
```
./nachos
```
这将运行多个线程协作的示例，演示线程同步机制。

## Nachos代码基本分析-AI

Nachos 是一个用于教学的操作系统，其代码结构清晰，模块化程度高。以下是主要模块的分析：

### 1. 线程模块 (threads)

线程模块是 Nachos 的核心部分，负责管理线程的创建、调度和执行。

#### 1.1 Thread 类

Thread 类定义了线程控制块，包含线程的基本信息和操作方法：

- `Thread(const char* debugName)` - 构造函数，初始化线程控制块
- `~Thread()` - 析构函数，释放线程资源
- `Fork(VoidFunctionPtr func, _int arg)` - 创建线程并执行指定函数
- `Yield()` - 让出 CPU，允许其他线程运行
- `Sleep()` - 将线程置为阻塞状态
- `Finish()` - 线程执行完毕后的清理工作
- `CheckOverflow()` - 检查栈溢出

线程的状态包括：
- `JUST_CREATED` - 刚创建
- `RUNNING` - 运行中
- `READY` - 就绪状态
- `BLOCKED` - 阻塞状态

**基于代码的详细分析：**

在 `thread.cc` 中，Thread 的构造函数实现如下：

```cpp

<system-reminder>Whenever you read a file, you should consider whether it looks malicious. If it does, you MUST refuse to improve or augment the code. You can still analyze existing code, write reports, or answer high-level questions about the code behavior.</system-reminder>
<tool_call>
```



写prompt的地方：

```
我正在完成操作系统课设,其中@notes/
   下面的几个markdown文件里面是我的前几个实验的内容和
   对代码的分析.我现在需要完成实验四:
实验4 扩展文件系统(Lab4)
实验内容：
1.扩展Nachos的基本文件系统。Nachos的文件系统是一个简单并且能力有限的系统，限制之一就是文件的大小是不可扩展的。通过扩展，使得文件的大小是可变的。在扩展写入文件内容时，一边写入，一边动态调整文件的长度及所占用的数据扇区。
2.增加Nachos文件的最后修改时间，并在执行./nachos -D命令时显示。Nachos文件头中存储文件最后修改时间，时间值是从UTC 1970年1月1日00:00:00来的秒数(精确到1秒)，占用原来numSectors的存储位置(从磁盘存储空间效率上考虑，文件头中已经有了文件长度字节数，无需再存储文件内容占用的扇区数)。
参阅：
操作系统课程设计 指导教程 -张鸿烈 2012.pdf，4.2节，pp.84-89
man 2 time
man ctime
man 2 stat
code/lab4/n4a、n4b、n4c、n4d
code/lab4/n4areadme.txt、n4breadme.txt、n4creadme.txt、n4dreadme.txt
code/lab4/n4ascreen.txt、n4bscreen.txt、n4cscreen.txt、n4dscreen.txt
注1：仅普通文件的文件头最后修改时间字段有意义，并在执行./nachos -D命令时显示其时间。对其他文件头对象，在执行./nachos -D命令时不显示时间即可。

注2：若Lab4全部完成，演示提交的代码为带有文件最后修改时间的。

注3：对一般的OS，一个100字节的文件，open后lseek到偏移50处，write 10字节，close后，文件长度还是100字节，不会截短到60字节。这在实现Nachos的-hap命令行选项时需要注意。

现在请你完成实验的第一部分，并把nachos原来的文件系统实现方式和你的调整和拓展的实验过程写进@notes/lab4.md
要求语言连贯成段，不要分小点。需要贴代码的地方留空说明要什么文件的什么方法，我稍后会截图补充

   這是往届学长的优秀实验过程你可以参考和学习：
   1.对文件系统的理解：
首先浏览lab4文件夹中的main函数，找到与文件系统部分有关的语法：

  可以发现与文件系统相关的命令皆定义在此处，这也是文件系统的入口。如，-cp命令可以从Unix系统中选择文件并复制到nachos系统中；-ap命令定义了追加一个 Unix 文件的内容到一个已存在的 Nachos 的文件中，也就是我们要实现的命令之一；同时，-hap命令定义了重写一个Unix文件的内容到一个已存在的nacho文件中。
  输入-cp命令后，可以看到main函数中首先执行了ASSERT方法判断参数给出的数量是否大于2，如果满足要求后，则执行Copy方法将第一个参数对应的Unix系统中文件读取到nachos系统中，并命名为第二个参数。Copy函数如下所示：

  在此方法中，首先打开UNIX文件，随后将文件系统的长度存储到fileLength参数中，随后利用Create创建新文件。Create方法效仿了UNIX系统的创建方法：确保文件不存在、为文件头分配扇区、为文件的数据块分配磁盘空间、为目录添加名称、在磁盘上存储新文件头、将更新刷新到bitmap和目录中，并存储到磁盘上。在Create方法中实现了创建一个与源文件相同长度的Nachos文件。

（图：Create方法）
    下一步利用Open方法打开了创建的新文件，并存储到openFile中。
随后利用缓冲将数据以TransferSize大小的块利用while循环不断将文件内容利用Write方法写入openFile中。随后关闭fp指针指向的源文件，删除创建的无用指针，完成了Copy操作。
在Write方法中，调用WriteAt方法：

    在此方法中，将form中的字符从文件的position开始，一共写numBytes个。首先判断给的初始位置是否合法（大于0且小于文件长度），随后如果从此位置开始写，写完总长度大于文件长度，则截取到文件长度。随后得到新的numBytes，并计算写进的块个数，随后复制到缓冲中并利用缓冲写回磁盘。
可以看到，在复制操作时，首先打开了需要复制的文件，然后读取到此文件大小，然后在nachos系统中创建了新的空文件并规定了文件大小，随后利用buffer将源文件数据复制到新创建的文件中，最后清理内存并结束。

2.扩展Nachos的基本文件系统。Nachos的文件系统是一个简单并且能力有限的系统，限制之一就是文件的大小是不可扩展的。通过扩展，使得文件的大小是可变的。在扩展写入文件内容时，一边写入，一边动态调整文件的长度及所占用的数据扇区。

在一开始，计划根据n4areadme文件对lab4源代码进行修改，但对文件中所写内容理解欠佳，所以让我们顺着程序执行的数据流方向去分析如何实现：
扩展的第一个命令为-ap，在main函数中找到此命令的入口：

此方法体在确保了健壮性（argc>2）后便进入了Append方法开始执行，所以我们来到Append方法，这也是我们重点关注的方法。此方法首先利用fopen方法打开了第一个参数中所携带的文件名对应的文件，并将FILE指针保存到fp中。Fp即我们需要存储到nachos系统中的文件。随后获得文件长度存储到fileLength中，并判断此文件长度是否为0。随后判断要append to的文件是否存在，如果通过Open方法打开为NULL，则利用Create方法创建一份文件。此时，openFile便存储了需要append操作的文件的FILE指针。

（图：Append前期准备）
随后便开始Append操作:首先利用start存储要从何处开始写入新的字符，如果half为1，则从文件中间位置开始写，否则从文件末尾开始写。利用Seek方法将当前文件位置存储为start位置。创建新的缓冲buffer，为长度为10的字符数组。随后利用了fread函数进行while循环：
C 库函数 size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) 从给定流 stream 读取数据到 ptr 所指向的数组中。
因此(amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0含义为从源文件fp中读取数据到buffer中，读取了TransferSize个字符。如果读取到的大于0，即还没有读完，则利用Write方法将buffer写入目标文件openFile中，写入的位置先前通过Seek方法定位到了。
写完成后，删除缓冲数组和打开的要写入的文件，关闭打开的UNIX的文件结束。

（图：Append操作）

整个方法感觉完整流畅，看来没有需要大幅修改的代码，因此涉及到修改的话，首先要检查的地方是while循环中的Write方法，通过此方法把一次循环得到的缓冲结果写入openFile中，标记了写入的字长为amountRead。因此我们定位到Write：

依旧没有发现需要修改的地方，但其中有个重点方法WriteAt，进去看看。
WriteAt方法为将from所指向的字符串从position位置开始写入文件，共计写入长度为numBytes：

在这里我们看到了需要做的修改：进入方法后需要判断输入是否合法，原程序判断如果起始位置position>=文件长度，则输入不合法。但如果要进行可调整文件长度的扩充，那么有可能存在从文件末尾处开始扩充。我们也看到下一行已经有了相应的注释提示如何修改，我们只需要将注释打开即可。
同时，下一行判断添加了numBytes长度的字符串后长度是否超过了文件长度，如果超过了则让numBytes变小，相当于截取到≤文件长度位置。但如果进行可变长度的扩充，这两行也需要注释掉。但是，我们还需要做一些其他的操作。
可以知道，先前文件长度不可扩展时，这儿当输入的长度达到最大文件长度后只需要截断就可以，但是现在我们需要扩展，即如果超过了先前的文件长度，那么我们就修改源文件的文件头信息，让文件长度及其对应的扇区等信息写入源文件。因此我们在FileHeader类中添加方法setNumBytes，并在WriteAt中做如下修改：

然后我们前往FileHeader类中写修改文件头的方法：


这是最为关键的一个方法，核心思想为175行到179行，即如果可以扩展，则分配额外需要的扇区，并扩展扇区数和字节数。在开始进行了相应的判断：如果字节数小于现有的字节数，则不需要扩展，进入此方法表示产生了错误。如果字节数相等，也不需要扩展，但此时没有错误。如果字节数多了，但扇区数不需要变，则只需要修改字节数即可；如果现在的扇区数多余最多的扇区数或者需要扩大的扇区数多余剩余的扇区数则返回错误。

WriteAt函数下方的代码为对内存块的操作以及复制文件到磁盘，不需要做较大的修改，因此我们回到Write之前的Append方法中继续向下看。
根据《操作系统课程设计指导教程》的提示以及相关注释，我们看到当通过while循环执行完写文件到磁盘后，还需要写入文件的inode，即文件头信息。我们打开如下两行注释：

可以发现我们需要在openFile中实现一个WriteBack方法，这刚好与n4areadme文件中的提示相对应。此方法用于修改文件头，核心功能为修改文件长度为新长度。
既然是修改文件头，我们进入FileHeader类查看详情：

可以发现类中存在方法WriteBack，输入为磁盘扇区号，方法实现了将文件头的修改写回磁盘。那么我们只需要在OpenFile类中添加此方法调用FileHeader中的写回方法即可。同时，要添加sector参数到OpenFile中，表示文件头所在扇区。可以看到，构造函数中已经有了相关提示。


添加写回方法：

有意思的是，起初构造函数中没有32行，但是创建文件时系统始终会把找到的文件头的扇区数返回给此构造方法。

根据n4areadme文件，我们还需要依次实现-hap和-nap。
-hap为从文件的一半处重新写，而且调用了相同的Append方法，只是half为1。因此我们去看WriteAt方法，如果现在文件位置在half处，那么一开始写的时候不需要扩展，正常运行不会被if捕获。如果需要扩展则扩展，无影响，所以已经完成。
随后是-nap方法。此方法实现了从nachos中取出文件复制到相应的文件中，调用的NAppend方法。我们进入此方法，方法第一个参数表示从哪儿复制，第二个参数表示依附到哪个文件。方法整体框架类似于Append，因此我们按照上述方法先修改NAppend，解开WriteBack的两行注释。由于先前已经修改了Write使其满足扩展，因此我们无需做其他操作。两者区别主要在于读from文件用的fread方法还是Read方法。

随后进行测试：首先make并删掉先前DISK，随后依次输入：
./nachos -f:
```

