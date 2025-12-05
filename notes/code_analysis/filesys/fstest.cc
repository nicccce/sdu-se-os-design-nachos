// fstest.cc 
//	文件系统的简单测试例程。
//
//	我们实现：
//	   Copy -- 将文件从 UNIX 复制到 Nachos
//	   Print -- cat Nachos 文件的内容
//	   Perftest -- Nachos 文件系统的压力测试
//		以小块方式读写一个非常大的文件
//		（在基线系统上不起作用！）
//
// Copyright (c) 1992-1993 The Regents of University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

// 数据传输块大小，设置为10字节以增加测试难度
// 小块传输更能测试文件系统的稳定性和正确性
#define TransferSize 	10

//----------------------------------------------------------------------
// Copy
// 	将 UNIX 文件 "from" 的内容复制到 Nachos 文件 "to"
//
//	实现逻辑：
//	1. 打开源UNIX文件
//	2. 获取源文件大小
//	3. 在Nachos中创建同大小的目标文件
//	4. 以小块方式读取源文件并写入目标文件
//	5. 关闭所有文件
//
//	关键点：
//	- 使用小块传输(TransferSize=10)增加测试难度
//	- 错误处理：文件打开失败、创建失败等
//	- 资源管理：确保文件正确关闭
//
//	参数：
//	- from: 源UNIX文件路径
//	- to: 目标Nachos文件名
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;					// 源UNIX文件指针
    OpenFile* openFile;				// 目标Nachos文件对象
    int amountRead, fileLength;			// 读取字节数，文件长度
    char *buffer;				// 数据传输缓冲区

    // 第一步：打开源UNIX文件
    if ((fp = fopen(from, "r")) == NULL) {	
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

    // 第二步：获取源文件长度
    fseek(fp, 0, 2);		// 定位到文件末尾
    fileLength = ftell(fp);	// 获取文件大小
    fseek(fp, 0, 0);		// 回到文件开头

    // 第三步：在Nachos中创建目标文件
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // 创建指定大小的Nachos文件
	printf("Copy: couldn't create output file %s\n", to);
	fclose(fp);
	return;
    }
    
    // 第四步：打开创建的Nachos文件
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);	// 确保文件打开成功
    
    // 第五步：以小块方式复制数据
    buffer = new char[TransferSize];	// 分配传输缓冲区
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0) {
	// 从源文件读取数据，写入目标文件
	openFile->Write(buffer, amountRead);	
    }
    delete [] buffer;	// 释放缓冲区

    // 第六步：关闭所有文件
    delete openFile;	// 关闭Nachos文件
    fclose(fp);		// 关闭UNIX文件
}

//----------------------------------------------------------------------
// Print
// 	打印 Nachos 文件 "name" 的内容。
//
//	实现逻辑：
//	1. 打开指定的Nachos文件
//	2. 以小块方式读取文件内容
//	3. 逐字符打印到控制台
//	4. 关闭文件
//
//	关键点：
//	- 使用小块读取(TransferSize=10)增加测试难度
//	- 逐字符输出，显示可打印字符
//	- 错误处理：文件不存在或打开失败
//
//	参数：
//	- name: 要打印的Nachos文件名
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;		// 打开的Nachos文件对象    
    int i, amountRead;		// 循环变量，读取字节数
    char *buffer;		// 数据读取缓冲区

    // 第一步：打开指定的Nachos文件
    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    // 第二步：分配读取缓冲区
    buffer = new char[TransferSize];
    
    // 第三步：循环读取并打印文件内容
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0) {
	// 逐字符打印读取到的数据
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    }
    delete [] buffer;	// 释放缓冲区

    delete openFile;	// 关闭Nachos文件
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	通过创建一个大文件，一次写出一小部分，一次读回一小部分，
//	然后删除文件来给 Nachos 文件系统施加压力。
//
//	实现为三个单独的例程：
//	  FileWrite -- 写入文件
//	  FileRead -- 读取文件
//	  PerformanceTest -- 整体控制，并打印性能数字
//
//	测试参数：
//	- FileName: 测试文件名 "TestFile"
//	- Contents: 写入内容 "1234567890" (10字节)
//	- ContentSize: 内容大小 10字节
//	- FileSize: 文件总大小 50000字节 (10*5000)
//
//	测试目的：
//	- 验证大文件操作的正确性
//	- 检查小块读写的一致性
//	- 评估文件系统性能
//	- 测试文件创建、读写、删除的完整性
//----------------------------------------------------------------------

// 测试文件名和内容定义
#define FileName 	(char*)"TestFile"		// 测试文件名
#define Contents 	(char*)"1234567890"		// 测试写入内容
#define ContentSize (int)strlen(Contents)	// 内容大小(10字节)
#define FileSize 	((int)(ContentSize * 5000))	// 文件总大小(50000字节)

// 内部函数：向测试文件写入数据
// 逐块写入相同内容，创建大文件
static void 
FileWrite()
{
    OpenFile *openFile;		// 打开的文件对象
    int i, numBytes;		// 循环变量，实际写入字节数

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
    
    // 创建测试文件(大小为0，实际大小在写入时确定)
    if (!fileSystem->Create(FileName, 0)) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    
    // 打开创建的文件
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
	printf("Perf test: unable to open %s\n", FileName);
	return;
    }
    
    // 循环写入数据，每次写入ContentSize字节
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
	// 检查写入是否成功
	if (numBytes < 10) {
	    printf("Perf test: unable to write %s\n", FileName);
	    delete openFile;
	    return;
	}
    }
    delete openFile;	// 关闭文件
}

// 内部函数：从测试文件读取数据
// 逐块读取并验证内容的正确性
static void 
FileRead()
{
    OpenFile *openFile;		// 打开的文件对象
    char *buffer = new char[ContentSize];	// 读取缓冲区
    int i, numBytes;		// 循环变量，实际读取字节数

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);

    // 打开要读取的文件
    if ((openFile = fileSystem->Open(FileName)) == NULL) {
	printf("Perf test: unable to open %s\n", FileName);
	delete [] buffer;
	return;
    }
    
    // 循环读取数据并验证
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
	// 检查读取是否成功且内容正确
	if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
	    printf("Perf test: unable to read %s\n", FileName);
	    delete openFile;
	    delete [] buffer;
	    return;
	}
    }
    delete [] buffer;	// 释放缓冲区
    delete openFile;	// 关闭文件
}

// 主性能测试函数
// 执行完整的写入-读取-删除测试流程，并输出性能统计
void
PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();	// 打印初始性能统计
    
    FileWrite();	// 执行写入测试
    FileRead();		// 执行读取测试
    
    // 清理：删除测试文件
    if (!fileSystem->Remove(FileName)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }
    stats->Print();	// 打印最终性能统计，对比差异
}