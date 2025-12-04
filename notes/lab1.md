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
   - GCC编译器: 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04)
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
    ThreadTest();  // 只有定义了THREADS宏才编译此行
#endif

#ifdef USER_PROGRAM
    StartProcess(*(argv + 1));  // 只有定义了USER_PROGRAM才编译
#endif

#ifdef FILESYS
    fileSystem->List();  // 只有定义了FILESYS才编译
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
```

- `name` 存储线程的调试名称
- `stackTop` 和 `stack` 初始化为 NULL，将在 `Fork` 时分配
- `status` 设置为 `JUST_CREATED`，表示线程刚创建
- 只有当系统需要支持用户级程序时，线程才会有一个关联的地址空间 `AddrSpace *space`

#### 1.2 线程创建过程 (Fork)

`Fork` 方法的实现是 Nachos 线程机制的核心：

```cpp
void Thread::Fork(VoidFunctionPtr func, _int arg)
{
    StackAllocate(func, arg);  // 分配栈并设置初始状态

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void) interrupt->SetLevel(oldLevel);
}
```

在 `StackAllocate` 方法中，关键的寄存器状态设置如下：

```cpp
machineState[PCState] = (_int) ThreadRoot;
machineState[StartupPCState] = (_int) InterruptEnable;
machineState[InitialPCState] = (_int) func;
machineState[InitialArgState] = arg;
machineState[WhenDonePCState] = (_int) ThreadFinish;
```

这里设置寄存器状态，使得线程启动时：
- 执行 `ThreadRoot` 函数作为入口点
- 将用户函数 `func` 设置为要执行的函数
- 将 `arg` 作为参数
- 函数结束后调用 `ThreadFinish`

#### 1.3 上下文切换机制

Nachos 的上下文切换在 `scheduler.cc` 的 `Run` 方法中实现：

```cpp
void Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {
        currentThread->SaveUserState();
        currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();
    currentThread = nextThread;
    currentThread->setStatus(RUNNING);
    
    SWITCH(oldThread, nextThread);  // 使用汇编代码进行上下文切换
    
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {
        currentThread->RestoreUserState();
        currentThread->space->RestoreState();
    }
#endif
}
```

- 保存当前线程状态
- 设置新线程为运行状态
- 通过 `SWITCH` 函数进行底层的寄存器切换
- 清理待销毁的线程

#### 1.4 Scheduler 类

调度器负责管理就绪队列并选择下一个运行的线程：

- `ReadyToRun(Thread* thread)` - 将线程放入就绪队列
- `FindNextToRun()` - 从就绪队列中找出下一个要运行的线程
- `Run(Thread* nextThread)` - 切换到指定线程执行

#### 1.5 中断和同步机制

Nachos 通过禁用中断来保证关键区域的原子性操作：

```cpp
IntStatus oldLevel = interrupt->SetLevel(IntOff);  // 禁用中断
// ... 执行关键操作 ...
(void) interrupt->SetLevel(oldLevel);              // 恢复中断
```

所有线程操作都在中断禁用状态下进行，确保了线程调度的安全性。

#### 1.6 主程序 (main.cc)

主程序负责初始化系统并处理命令行参数：

- 初始化系统资源
- 根据命令行参数执行不同的功能
- 调用线程测试函数

### 2. 系统模块 (system.h/cc)

系统模块定义了 Nachos 中的全局变量和初始化函数：

- `currentThread` - 当前运行的线程
- `scheduler` - 调度器实例
- `interrupt` - 中断管理器
- `stats` - 系统统计信息
- `Initialize()` - 系统初始化函数

### 3. 核心功能详解

#### 3.1 线程创建与执行

线程的创建和执行过程如下：

1. 通过 `new Thread` 创建线程对象
2. 调用 `thread->Fork(func, arg)` 启动线程
3. 线程在分配的栈上执行指定函数
4. 函数执行完毕后调用 `ThreadFinish`

#### 3.2 上下文切换

上下文切换是操作系统的核心功能，Nachos 中通过 `SWITCH` 函数实现：

- 保存当前线程的寄存器状态
- 恢复下一个线程的寄存器状态
- 切换到新线程的执行栈

#### 3.3 线程同步

Nachos 提供了基本的线程同步机制：

- `Yield()` - 主动让出 CPU
- `Sleep()` - 进入阻塞状态
- 通过调度器管理线程状态转换

### 4. 系统初始化流程

Nachos 系统的初始化流程如下：

1. 调用 `Initialize()` 函数进行系统初始化
2. 创建初始线程 (main thread)
3. 启动线程调度机制
4. 根据命令行参数执行相应功能
5. 系统运行并处理用户程序

### 5. 代码特点

- 模块化设计，各功能模块职责明确
- 使用面向对象编程，代码结构清晰
- 通过宏定义实现不同功能模块的开关
- 提供了完整的线程管理机制
- 代码注释详细，便于学习和理解

### 6. 关键代码分析总结

Nachos 的线程机制通过 Thread 类和 Scheduler 类的协作实现，其核心是上下文切换机制。在 `StackAllocate` 中设置的寄存器状态是关键，它决定了线程启动时的行为：执行用户函数，然后调用 `ThreadFinish`。上下文切换通过汇编语言实现的 `SWITCH` 函数完成，确保了线程间的平滑切换。

## Nachos 编译系统详解

### 1. 项目结构总览
```
code/
├── bin/          # 二进制工具（汇编器、链接器等）
├── demo0/        # 线程演示
├── demo1/        # 生产者消费者演示
├── filesys/      # 文件系统
├── lab1-7/       # 实验目录
├── machine/      # 硬件模拟（MIPS处理器、内存等）
├── monitor/      # 管程实现
├── network/      # 网络功能
├── test/         # 测试程序
├── threads/      # 线程系统（包含main函数）
└── userprog/     # 用户程序支持
```

### 2. 编译系统核心组件

#### 2.1 主要配置文件
- `Makefile.dep`: 系统依赖配置，检测平台（Linux、MIPS、SPARC等）
- `Makefile.common`: 通用编译规则，定义编译流程和依赖关系
- `各模块/Makefile.local`: 模块特定的源文件列表和宏定义

#### 2.2 平台检测与配置
```makefile
uname = $(shell uname)
ifeq ($(uname),Linux)
    HOST = -DHOST_i386 -DHOST_LINUX
    arch = unknown-i386-linux
endif
```
这使得Nachos能在不同架构上编译运行。

### 3. 编译流程详解

#### 3.1 目录结构
- `arch/$(arch)/objects`: 编译后的目标文件
- `arch/$(arch)/bin`: 可执行文件
- `arch/$(arch)/depends`: 自动生成的依赖文件

#### 3.2 编译命令
1. **模块级编译**: 在各模块目录下 `make`
2. **系统级编译**: 在 `code/` 目录下 `make`

### 4. 各模块编译详情

#### 4.1 Threads 模块（核心模块）
在 `code/threads` 目录下编译时：
- **源文件**: 编译 `main.cc`, `thread.cc`, `scheduler.cc`, `synch.cc` 等
- **功能**: 实现线程管理、调度、同步等核心功能
- **入口**: `main.cc` 包含整个系统的入口函数
- **宏定义**: `DEFINES += -DTHREADS`

#### 4.2 Machine 模块（硬件模拟）
- **功能**: 模拟MIPS处理器、内存管理、中断系统、定时器等
- **源文件**: `machine.cc`, `mipssim.cc`, `interrupt.cc`, `timer.cc` 等

#### 4.3 UserProg 模块（用户程序支持）
- **编译条件**: 需要定义 `USER_PROGRAM` 宏
- **功能**: 实现用户程序加载、执行、系统调用等功能
- **源文件**: `addrspace.cc`, `exception.cc`, `progtest.cc` 等

#### 4.4 FileSystem 模块（文件系统）
- **编译条件**: 需要定义 `FILESYS` 宏
- **功能**: 实现文件创建、删除、读写等操作
- **源文件**: `filesys.cc`, `directory.cc`, `filehdr.cc`, `openfile.cc` 等

#### 4.5 Network 模块（网络功能）
- **编译条件**: 需要定义 `NETWORK` 宏
- **功能**: 实现网络通信功能
- **源文件**: `network.cc`, `post.cc` 等

### 5. 模块间依赖与链接

#### 5.1 文件查找机制
通过 `Makefile.common` 中的 `vpath` 指令：
```makefile
vpath %.cc  ../network:../filesys:../userprog:../threads:../machine
```
允许编译器在指定目录中查找源文件。

#### 5.2 头文件包含路径
通过 `INCPATH` 指令指定：
```makefile
INCPATH += -I../machine -I../userprog -I../filesys
```

#### 5.3 条件编译控制
在 `main.cc` 中使用 `#ifdef` 控制不同功能：
```c++
#ifdef THREADS
    ThreadTest();
#endif
#ifdef USER_PROGRAM
    StartProcess(*(argv + 1));
#endif
```

### 6. 编译时的文件合并过程

#### 6.1 单模块编译
- 在 `threads/` 目录下编译时，会生成包含 `main` 函数的可执行文件
- 只有核心线程功能可用

#### 6.2 多模块编译
- 在包含用户程序功能的模块下编译时：
  - `main.cc` (来自threads)
  - `thread.cc`, `scheduler.cc` (来自threads)
  - `machine.cc`, `interrupt.cc` (来自machine)
  - `addrspace.cc`, `exception.cc` (来自userprog)
  - 以及其他相关模块的文件
- 最终链接成一个完整的系统

### 7. 编译顺序与依赖关系

从 `Makefile.common` 中可以看到依赖顺序：
1. `THREADS` 必须最先实现（所有系统的基础）
2. `USERPROG` 必须在 `VM` 之前
3. `USERPROG` 和 `FILESYS` 可以任意顺序，但若先做 `USERPROG` 则需 `FILESYS_STUB`

### 8. 总结

Nachos 采用模块化编译设计，虽然 `main` 函数只在 `threads` 模块中，但通过条件编译和链接机制，可以在不同阶段构建出功能不同的系统：
- 仅线程功能（THREADS）
- 加上用户程序支持（USER_PROGRAM）
- 再加上文件系统（FILESYS）
- 最后加上网络功能（NETWORK）

这种设计便于教学和逐步开发，每个阶段都形成一个可运行的完整系统。