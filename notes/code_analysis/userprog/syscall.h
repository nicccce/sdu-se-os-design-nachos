/* syscalls.h 
 * 	Nachos系统调用接口。这些是用户程序可以通过"syscall"指令陷入内核
 *	来调用的Nachos内核操作。
 *
 *	此文件被用户程序和Nachos内核包含。
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"

/* 系统调用代码 -- 由存根用来告诉内核正在请求哪个系统调用 */
#define SC_Halt		0   // 系统停止调用
#define SC_Exit		1   // 程序退出调用
#define SC_Exec		2   // 执行程序调用
#define SC_Join		3   // 等待程序结束调用
#define SC_Create	4   // 创建文件调用
#define SC_Open		5   // 打开文件调用
#define SC_Read		6   // 读取文件调用
#define SC_Write	7   // 写入文件调用
#define SC_Close	8   // 关闭文件调用
#define SC_Fork		9   // 创建线程调用
#define SC_Yield	10  // 让出CPU调用

#ifndef IN_ASM

/* 系统调用接口。这些是Nachos内核需要支持的操作，
 * 以便能够运行用户程序。
 *
 * 用户程序只需调用过程即可调用这些操作；
 * 汇编语言存根将系统调用代码放入寄存器，然后陷入内核。
 * 然后在Nachos内核中调用内核过程，在适当的错误检查后，
 * 从exception.cc中的系统调用入口点调用。
 */

/* 停止Nachos，并打印性能统计信息 */
void Halt();		
 

/* 地址空间控制操作：Exit、Exec和Join */

/* 此用户程序已完成（status = 0表示正常退出） */
void Exit(int status);	

/* 正在执行的用户程序（地址空间）的唯一标识符 */
typedef int SpaceId;	
 
/* 运行存储在Nachos文件"name"中的可执行文件，并返回
 * 地址空间标识符
 */
SpaceId Exec(char *name);
 
/* 仅在用户程序"id"完成后返回。
 * 返回退出状态。
 */
int Join(SpaceId id); 	
 

/* 文件系统操作：Create、Open、Read、Write、Close
 * 这些函数以UNIX为蓝本 -- 文件代表
 * 文件和硬件I/O设备。
 *
 * 如果在完成文件系统作业之前完成此作业，
 * 请注意Nachos文件系统有一个存根实现，该实现
 * 可用于测试这些例程。
 */
 
/* 已打开的Nachos文件的唯一标识符 */
typedef int OpenFileId;	

/* 当地址空间启动时，它有两个打开的文件，代表
 * 键盘输入和显示输出（在UNIX术语中，stdin和stdout）。
 * 可以直接在这些文件上使用Read和Write，而无需先打开
 * 控制台设备。
 */

#define ConsoleInput	0  // 控制台输入设备标识符
#define ConsoleOutput	1  // 控制台输出设备标识符
 
/* 使用"name"创建一个Nachos文件 */
void Create(char *name);

/* 打开Nachos文件"name"，并返回一个"OpenFileId"，
 * 可用于读取和写入文件。
 */
OpenFileId Open(char *name);

/* 从"buffer"向打开的文件写入"size"字节 */
void Write(char *buffer, int size, OpenFileId id);

/* 从打开的文件读取"size"字节到"buffer"中。
 * 返回实际读取的字节数 -- 如果打开的文件不够长，
 * 或者如果是I/O设备，且没有足够的字符可读，
 * 则返回可用的字符（对于I/O设备，
 * 您应始终等待直到可以返回至少一个字符）。
 */
int Read(char *buffer, int size, OpenFileId id);

/* 关闭文件，我们已完成对其的读写操作 */
void Close(OpenFileId id);



/* 用户级线程操作：Fork和Yield。允许在用户程序中
 * 运行多个线程。
 */

/* Fork一个线程以在与当前线程*相同*的地址空间中
 * 运行过程("func")。
 */
void Fork(void (*func)());

/* 将CPU让给另一个可运行的线程，无论是否在此地址空间中 */
void Yield();		

#endif /* IN_ASM */

#endif /* SYSCALL_H */